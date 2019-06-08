#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>

#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>
#include <json/json.h>

#include "agentsim/agentsim.h"

namespace utility {
void GenerateLocalUUID(std::string& uuid)
{
	// Doesn't matter if time is seeded or not at this point
	//  just need relativley unique local identifiers for runtime
	char strUuid[128];
	sprintf(strUuid, "%x%x-%x-%x-%x-%x%x%x",
	rand(), rand(),                 // Generates a 64-bit Hex number
	rand(),                         // Generates a 32-bit Hex number
	((rand() & 0x0fff) | 0x4000),   // Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
	rand() % 0x3fff + 0x8000,       // Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
	rand(), rand(), rand());        // Generates a 96-bit Hex number

	uuid = strUuid;
}
};

#define HEART_BEAT_INTERVAL_SECONDS 15
#define MAX_MISSED_HEARTBEATS_BEFORE_TIMEOUT 4
#define MAX_NUM_FRAMES std::numeric_limits<std::size_t>::max()
#define NO_CLIENT_TIMEOUT_SECONDS 30

typedef websocketpp::server<websocketpp::config::asio> server;
using namespace aics::agentsim;

struct NetMessage {
	std::string sender_uid;
	std::string msg_str;
};

std::vector<NetMessage> net_messages;
std::string latest_conn_uid = "";

enum ClientPlayState
{
	Playing = 0,
	Paused = 1,
	Stopped
};

struct NetState
{
  std::size_t frame_no = 0;
  bool is_finished = false;
	ClientPlayState play_state = ClientPlayState::Stopped;
};

// The following are mapped using a local uid
std::unordered_map<std::string, NetState> net_states;
std::unordered_map<std::string, websocketpp::connection_hdl> net_connections;
std::unordered_map<std::string, int> missed_net_heartbeats;

std::mutex net_msg_mtx;
std::atomic<bool> has_simulation_model { false };
std::atomic<bool> has_unhandled_new_connection { false };
std::string most_recent_model = "";
std::vector<std::string> param_cache;

bool use_readdy = true;
bool use_cytosim = !use_readdy;
int run_mode = 0; // live simulation

std::string trajectory_file_directory = "trajectory/";

enum {
  id_undefined_web_request = 0,
  id_vis_data_arrive = 1,
  id_vis_data_request,
  id_vis_data_finish,
  id_vis_data_pause,
  id_vis_data_resume,
  id_vis_data_abort,
  id_update_time_step,
  id_update_rate_param,
  id_model_definition,
  id_heartbeat_ping,
  id_heartbeat_pong,
  id_play_cache
};

enum {
  id_live_simulation = 0,
  id_pre_run_simulation = 1,
  id_traj_file_playback = 2
};



template <typename T, typename U>
inline bool equals(const std::weak_ptr<T>& t, const std::weak_ptr<U>& u)
{
    return !t.owner_before(u) && !u.owner_before(t);
}

void on_message(websocketpp::connection_hdl hd1, server::message_ptr msg) {
	NetMessage nm;
	nm.msg_str = msg->get_payload();

	//@TODO: Not every message needs the uid of the sender
	//	verify if this does not have a significant impact on perf
	for(auto& entry : net_connections)
	{
		auto& uid = entry.first;
		auto& conn = entry.second;

		if(equals<void,void>(conn, hd1))
		{
			nm.sender_uid = uid;
			break;
		}
	}

  net_messages.push_back(nm);
}

std::vector<std::string> uids_to_delete;
void on_close(websocketpp::connection_hdl hd1) {
  for(auto& entry : net_connections)
  {
    auto& uid = entry.first;
    auto& conn = entry.second;

    if(conn.expired()
    || equals<void,void>(conn,hd1))
    {
      std::cout << "Marking network connection for closure.\n";
      uids_to_delete.push_back(uid);
    }
  }
}

void on_open(websocketpp::connection_hdl hd1) {
  std::cout << "Incoming connection accepted.\n";
  has_unhandled_new_connection = true;

  std::string uid;
  utility::GenerateLocalUUID(uid);

  net_states[uid] = NetState();
  missed_net_heartbeats[uid] = 0;
  net_connections[uid] = hd1;
  latest_conn_uid = uid;
}

int main() {
  server sim_server;
  sim_server.set_reuse_addr(true);
  std::atomic<bool> isServerRunning { true };

  auto ws_thread = std::thread([&] {
    sim_server.set_message_handler(&on_message);
    sim_server.set_close_handler(&on_close);
    sim_server.set_open_handler(&on_open);
    sim_server.set_access_channels(websocketpp::log::alevel::none);
    sim_server.set_error_channels(websocketpp::log::elevel::none);

    sim_server.init_asio();
    sim_server.listen(9002);
    sim_server.start_accept();

    sim_server.run();
  });

  auto sim_thread = std::thread([&] {
    // Simulation thread/timing variables
    bool isSimulationSetup = false;
    auto start = std::chrono::steady_clock::now();

    float time_step = 1e-12; // seconds
    std::size_t n_time_steps = 1;
    std::string traj_file_name = "";

    // Json cpp setup
    Json::StreamWriterBuilder json_stream_writer;

    Json::CharReaderBuilder json_read_builder;
    std::unique_ptr<Json::CharReader> const json_reader(json_read_builder.newCharReader());

    // Simulation setup
    std::vector<std::shared_ptr<SimPkg>> simulators;

    if(use_readdy) {
      ReaDDyPkg* readdySimPkg = new ReaDDyPkg();
      std::shared_ptr<SimPkg> readdyPkg;
      readdyPkg.reset(readdySimPkg);
      simulators.push_back(readdyPkg);
    }

    if(use_cytosim) {
      CytosimPkg* cytosimSimPkg = new CytosimPkg();
      std::shared_ptr<SimPkg> cytosimPkg;
      cytosimPkg.reset(cytosimSimPkg);
      simulators.push_back(cytosimPkg);
    }

    std::vector<std::shared_ptr<Agent>> agents;
  	Simulation simulation(simulators, agents);
    //simulation.Reset();

    // Runtime loop
    while(isServerRunning)
    {

      /**
      * Remove expired network connections
      */
      for(std::size_t i = 0; i < uids_to_delete.size(); ++i)
      {
        std::cout << "Removing closed network connection.\n";
        net_connections.erase(uids_to_delete[i]);
        missed_net_heartbeats.erase(uids_to_delete[i]);
        net_states.erase(uids_to_delete[i]);
      }
      uids_to_delete.clear();

			/**
			* Send data to new connections
			*/
			if(has_unhandled_new_connection && has_simulation_model)
			{
				std::cout << "Sending current model to most recent client.\n";
				try {
					sim_server.send(net_connections[latest_conn_uid],
					 most_recent_model, websocketpp::frame::opcode::text);

					has_unhandled_new_connection = false;

					for(std::size_t i = 0; i < param_cache.size(); ++i)
					{
						sim_server.send(net_connections[latest_conn_uid],
							param_cache[i], websocketpp::frame::opcode::text);
					}
				}
				catch(...)
				{
					std::cout << "Ignoring failed websocket send" << std::endl;
				}
			}

			/**
			*	Check current state of the simulation
			*/
			bool isRunningSimulation = false;
			bool isSimulationPaused = true;

			for(auto& entry : net_states)
			{
				auto& net_state = entry.second;
				if(net_state.play_state == ClientPlayState::Playing)
				{
					isRunningSimulation = true;
					isSimulationPaused = false;
				}
			}

      /**
      * Handle net messages
      */
      if(net_messages.size() > 0)
      {
        net_msg_mtx.lock();
        for(std::size_t i = 0; i < net_messages.size(); ++i)
        {
					std::string sender_uid = net_messages[i].sender_uid;
          std::string msg_str = net_messages[i].msg_str;
          std::string errs;
          Json::Value json_msg;
          json_reader->parse(msg_str.c_str(), msg_str.c_str() + msg_str.length(),
                              &json_msg, &errs);

          int msg_type = json_msg["msg_type"].asInt();
          switch(msg_type)
          {
            case id_undefined_web_request:
            {
              std::cout << "undefined web request received." <<
              " There may be a typo in the js client. \n";
            } break;
            case id_vis_data_arrive:
            {
              std::cout << "vis data received\n";
            } break;
            case id_vis_data_request:
            {
              std::cout << "data request received\n";

							// If a simulation is already in progress, don't allow a new client to
							//	change the simulation, unless there is only one client connected
							if(!isRunningSimulation || net_connections.size() == 1)
							{
								isSimulationSetup = false;
	              run_mode = json_msg["mode"].asInt();

	              switch(run_mode)
	              {
	                case id_live_simulation:
	                {
	                  std::cout << "Running live simulation" << std::endl;
	                  simulation.SetPlaybackMode(id_live_simulation);
	                } break;
	                case id_pre_run_simulation:
	                {
	                  time_step = json_msg["time-step"].asFloat();
	                  n_time_steps = json_msg["num-time-steps"].asInt();
	                  std::cout << "Running pre-run simulation" << std::endl;
	                  simulation.SetPlaybackMode(id_pre_run_simulation);
	                } break;
	                case id_traj_file_playback:
	                {
	                  traj_file_name = json_msg["file-name"].asString();
	                  std::cout << "Playing back trajectory file" << std::endl;
	                  simulation.SetPlaybackMode(id_traj_file_playback);
	                } break;
	              }

								simulation.Reset();
							}

							net_states[sender_uid].play_state = ClientPlayState::Playing;
							net_states[sender_uid].frame_no = 0;
            } break;
            case id_vis_data_finish:
            {
              std::cout << "data finish received\n";
            } break;
            case id_vis_data_pause:
            {
              std::cout << "pause command received\n";
              net_states[sender_uid].play_state = ClientPlayState::Paused;
            } break;
            case id_vis_data_resume:
            {
              std::cout << "resume command received\n";
              net_states[sender_uid].play_state = ClientPlayState::Playing;
            } break;
            case id_vis_data_abort:
            {
              std::cout << "abort command received\n";
              net_states[sender_uid].play_state = ClientPlayState::Stopped;
            } break;
            case id_update_time_step:
            {
              time_step = json_msg["time_step"].asFloat();
              std::cout << "time step updated to " << time_step << "\n";

              for(auto& entry : net_connections)
              {
                auto conn = entry.second;

                auto sptr = conn.lock();
                if(sptr.get() == nullptr) { continue; }

                std::cout << "Sending timestep update to client " << i << "\n";

                try {
                  sim_server.send(conn, msg_str, websocketpp::frame::opcode::text);
                }
                catch(...)
                {
                  std::cout << "Ignoring failed websocket send" << std::endl;
                }
              }
            } break;
            case id_update_rate_param:
            {
              std::string param_name = json_msg["param_name"].asString();
              float param_value = json_msg["param_value"].asFloat();
              simulation.UpdateParameter(param_name, param_value);

              std::cout << "rate param " << param_name <<
              " updated to " << param_value << "\n";

              param_cache.push_back(msg_str);

              // Update all listening client front-ends
              for(auto& entry : net_connections)
              {
                auto conn = entry.second;

                auto sptr = conn.lock();
                if(sptr.get() == nullptr) { continue; }

                std::cout << "Sending param update to client " << i << "\n";

                try {
                  sim_server.send(conn, msg_str, websocketpp::frame::opcode::text);
                }
                catch(...)
                {
                  std::cout << "Ignoring failed websocket send" << std::endl;
                }
              }
            } break;
            case id_model_definition:
            {
              std::cout << "model definition arrived\n";
              aics::agentsim::Model sim_model;
              parse_model(json_msg, sim_model);
              print_model(sim_model);
              simulation.SetModel(sim_model);

              has_simulation_model = true;
              most_recent_model = msg_str;

              time_step = sim_model.max_time_step;
              std::cout << "Set timestep to " << time_step << "\n";

              param_cache.clear();

              // Update all listening client front-ends
              for(auto& entry : net_connections)
              {
                auto conn = entry.second;

                auto sptr = conn.lock();
                if(sptr.get() == nullptr) { continue; }

                std::cout << "Sending model update to client " << i << "\n";

                try {
                  sim_server.send(conn, msg_str, websocketpp::frame::opcode::text);
                }
                catch(...)
                {
                  std::cout << "Ignoring failed websocket send" << std::endl;
                }
              }
            } break;
            case id_heartbeat_ping:
            {
              std::cout << "heartbeat ping arrived\n";
            } break;
            case id_heartbeat_pong:
            {
              auto conn_id = json_msg["conn_id"].asString();
              std::cout << "heartbeat pong arrived from client " << conn_id << "\n";

              missed_net_heartbeats[conn_id] = 0;
            } break;
            case id_play_cache:
            {
              auto frame_no = json_msg["frame-num"].asInt();
              std::cout << "request to play cached from frame "
                << frame_no << " arrived from client " << sender_uid << std::endl;

              net_states[sender_uid].frame_no = frame_no;
              net_states[sender_uid].play_state = ClientPlayState::Playing;
            } break;
            default:
            {
              std::cout << "Received unrecognized message of type " << msg_type << "\n";
            } break;
          }
        }

        net_messages.clear();
        net_msg_mtx.unlock();
      }

			/**
			*	If simulation isn't running, no need to continue
			*/
			if(!isRunningSimulation || isSimulationPaused)
			{
				start = std::chrono::steady_clock::now();
				continue;
			}

      /**
      * Run simulation setup
      *
      * Either run the simulation to completion, load a trajectory file,
      * or do nothing in the case of 'live' simulation
      *
      */

      if(!isSimulationSetup)
      {
        if(run_mode == id_pre_run_simulation)
        {
          simulation.RunAndSaveFrames(time_step, n_time_steps);
        }

        if(run_mode == id_traj_file_playback)
        {
          simulation.LoadTrajectoryFile(trajectory_file_directory + traj_file_name);
        }

				if(run_mode == id_pre_run_simulation
					|| run_mode == id_traj_file_playback)
				{
	        // Load the first hundred simulation frames into a runtime cache
	        std::cout << "Loading trajectory file into runtime cache" << std::endl;
	        std::size_t fn = 0;
	        while(!simulation.HasLoadedAllFrames() && fn < 100)
	        {
	          std::cout << "Loading frame " << ++fn << std::endl;
	          simulation.LoadNextFrame();
	        }
	        std::cout << "Finished loading trajectory for now" << std::endl;
				}

        isSimulationSetup = true;
      }

      /**
      * Run simulation timestep ever 66 milliseconds
      */
      auto now = std::chrono::steady_clock::now();
      auto diff = now - start;

      if(diff >= std::chrono::milliseconds(66))
      {
        /**
        * Run simulation timestep
        */
        start = now;
				auto nframes = simulation.GetNumFrames();

        // Cache playback
        bool all_clients_playing_from_cache = true;
        for(auto& entry : net_states)
        {
          auto& net_id = entry.first;
          auto& net_state = entry.second;

					if(net_state.play_state != ClientPlayState::Playing)
					{
						continue;
					}

          if(net_state.is_finished)
          {
            continue;
          }

          if(net_state.frame_no >= nframes - 1)
          {
            if(net_state.frame_no != MAX_NUM_FRAMES)
            {
              std::cout << "End of simulation cache reached for client " << net_id << std::endl;
              net_state.frame_no = MAX_NUM_FRAMES;
            }

            if(simulation.HasLoadedAllFrames())
            {
              std::cout << "Simulation finished for client " << net_id << std::endl;
              net_state.is_finished = true;
            }
            else
            {
              all_clients_playing_from_cache = false;
            }
          }

					if(nframes == 0)
					{
						all_clients_playing_from_cache = false;
					}
        }

        if(all_clients_playing_from_cache)
        {
          // nothing to do at the moment
        }
        else if(run_mode == id_live_simulation)
        {
          simulation.RunTimeStep(time_step);
        }
        else if(run_mode == id_pre_run_simulation
          || run_mode == id_traj_file_playback)
        {
          if(!simulation.HasLoadedAllFrames())
          {
						simulation.LoadNextFrame();
          }
        }

        /**
        * JSON Net Serialization
        */
        for(auto& entry : net_states)
        {
          auto& net_id = entry.first;
          auto& net_state = entry.second;

					if(net_state.play_state != ClientPlayState::Playing)
					{
						continue;
					}

          if(net_state.is_finished)
          {
            continue;
          }

          AgentDataFrame simData;

          // This state is currently being used to demark the 'latest' frame
          if(net_state.frame_no == MAX_NUM_FRAMES)
          {
            simData = simulation.GetDataFrame(simulation.GetNumFrames() - 1);
          }
          else
          {
            simData = simulation.GetDataFrame(net_state.frame_no++);
          }


          Json::Value net_agent_data_frame;
          std::vector<float> vals;

          net_agent_data_frame["msg_type"] = id_vis_data_arrive;
          for(std::size_t i = 0; i < simData.size(); ++i)
          {
            auto agentData = simData[i];

            Json::Value agent;
            vals.push_back(agentData.vis_type);
            vals.push_back(agentData.type);
            vals.push_back(agentData.x);
            vals.push_back(agentData.y);
            vals.push_back(agentData.z);
            vals.push_back(agentData.xrot);
            vals.push_back(agentData.yrot);
            vals.push_back(agentData.zrot);
            vals.push_back(agentData.collision_radius);
            vals.push_back(agentData.subpoints.size());

            for(std::size_t j = 0; j < agentData.subpoints.size(); ++j)
            {
              vals.push_back(agentData.subpoints[j]);
            }
          }

          /**
          * Copy values to json data array
          */
          auto json_data_arr = Json::Value(Json::arrayValue);
          for(std::size_t j = 0; j < vals.size(); ++j)
          {
            int nd_index = static_cast<int>(j);
            json_data_arr[nd_index] = vals[nd_index];
          }

          net_agent_data_frame["data"] = json_data_arr;

          /**
          * Send data over the network
          */
          std::string msg = Json::writeString(json_stream_writer, net_agent_data_frame);

          // validate net connection
          auto sptr = net_connections[net_id].lock();
          if(sptr.get() == nullptr) { continue; }

          // try to send, failure to send is harmless, and likely a result of threading
          try {
            sim_server.send(net_connections[net_id], msg, websocketpp::frame::opcode::text);
          }
          catch(...)
          {
            std::cout << "Ignoring failed websocket send" << std::endl;
          }
        }

      }
    }
  });

  auto heartbeat_thread = std::thread([&] {
    auto start = std::chrono::steady_clock::now();
    auto now = start;
    auto diff = now - start;
		auto no_client_timer = std::chrono::steady_clock::now();

    Json::StreamWriterBuilder json_stream_writer;

    Json::CharReaderBuilder json_read_builder;
    std::unique_ptr<Json::CharReader> const json_reader(json_read_builder.newCharReader());

    while(isServerRunning)
    {
      now = std::chrono::steady_clock::now();
      diff = now - start;

			if(net_connections.size() == 0)
			{
				auto now = std::chrono::steady_clock::now();
	      auto diff = now - no_client_timer;

	      if(diff >= std::chrono::seconds(NO_CLIENT_TIMEOUT_SECONDS)) {
					std::cout << "No clients connected for " << NO_CLIENT_TIMEOUT_SECONDS << " seconds, exiting server ... " << std::endl;
					std::raise(SIGINT);
				}
			}
			else
			{
				no_client_timer = std::chrono::steady_clock::now();
			}

      if(diff >= std::chrono::seconds(HEART_BEAT_INTERVAL_SECONDS))
      {
        if(net_messages.size() > 0)
        {
          net_msg_mtx.lock();
          for(std::size_t i = 0; i < net_messages.size(); ++i)
          {
            std::string msg_str = net_messages[i].msg_str;
            std::string errs;
            Json::Value json_msg;
            json_reader->parse(msg_str.c_str(), msg_str.c_str() + msg_str.length(),
                                &json_msg, &errs);

            int msg_type = json_msg["msg_type"].asInt();
            switch(msg_type)
            {
              case id_heartbeat_ping:
              {
                std::cout << "heartbeat ping arrived\n";
              } break;
              case id_heartbeat_pong:
              {
                auto conn_id = json_msg["conn_id"].asString();
                std::cout << "heartbeat pong arrived from client " << conn_id << "\n";

                missed_net_heartbeats[conn_id] = 0;
                net_messages.erase(net_messages.begin() + i);
              } break;
              default:
              {
                // Don't care in this thread
              } break;
            }
          }
          net_msg_mtx.unlock();
        }

        start = now;

        Json::Value ping_data;
        ping_data["msg_type"] = id_heartbeat_ping;

        for(auto& entry : net_connections)
        {
          auto& current_uid = entry.first;
          auto conn = entry.second;

          missed_net_heartbeats[current_uid]++;

          auto sptr = net_connections[current_uid].lock();
          if(sptr.get() == nullptr) { continue; }

          if(missed_net_heartbeats[current_uid] > MAX_MISSED_HEARTBEATS_BEFORE_TIMEOUT)
          {
            std::cout << "Removing unresponsive network connection.\n";
            sim_server.pause_reading(conn);
            sim_server.close(conn, 0, "");

            net_connections.erase(current_uid);
            missed_net_heartbeats.erase(current_uid);
          }
          else
          {
            std::cout << "Sending ping heartbeat to connection " << current_uid << ".\n";
            ping_data["conn_id"] = current_uid;
            std::string msg = Json::writeString(json_stream_writer, ping_data);

            try {
              sim_server.send(conn, msg, websocketpp::frame::opcode::text);
            }
            catch(...)
            {
              std::cout << "Ignoring failed websocket send" << std::endl;
            }
          }
        }
      }
    }
  });

  std::string input;
  std::cout << "Enter 'quit' to exit\n";
  while(isServerRunning && std::getline(std::cin, input, '\n'))
  {
    if(input == "quit")
    {
      isServerRunning = false;
    }
    else
    {
      input = "";
    }
  }

  std::cout << "Exiting Server...\n";
  sim_thread.join();
  heartbeat_thread.join();
  ws_thread.detach();
}
