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

#define HEART_BEAT_INTERVAL_SECONDS 8
#define MAX_MISSED_HEARTBEATS_BEFORE_TIMEOUT 2

typedef websocketpp::server<websocketpp::config::asio> server;
using namespace aics::agentsim;

std::vector<std::string> net_messages;
std::vector<websocketpp::connection_hdl> net_connections;
std::vector<int> missed_net_heartbeats;

std::mutex net_msg_mtx;
std::atomic<bool> has_simulation_model { false };
std::atomic<bool> has_unhandled_new_connection { false };
std::string most_recent_model = "";
std::vector<std::string> param_cache;

bool use_readdy = true;
bool use_cytosim = !use_readdy;
int run_mode = 0; // live simulation

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

void on_message(websocketpp::connection_hdl, server::message_ptr msg) {
  net_messages.push_back(msg->get_payload());
}

void on_close(websocketpp::connection_hdl hd1) {
  for(std::size_t i = 0; i < net_connections.size(); ++i)
  {
    if(net_connections[i].expired()
      || equals<void,void>(net_connections[i],hd1))
    {
      std::cout << "Removing closed network connection.\n";
      net_connections.erase(net_connections.begin()+i);
      --i;
      missed_net_heartbeats.erase(missed_net_heartbeats.begin()+i);
    }
  }
}

void on_open(websocketpp::connection_hdl hd1) {
  std::cout << "Incoming connection accepted.\n";
  net_connections.push_back(hd1);
  missed_net_heartbeats.push_back(0);
  has_unhandled_new_connection = true;
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
    bool isRunningSimulation = false;
    bool isSimulationPaused = false;
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
      if(isRunningSimulation && net_connections.size() == 0)
      {
        isRunningSimulation = false;
        std::cout << "No clients connected. Ending simulation.\n";
      }

      if(has_unhandled_new_connection && has_simulation_model)
      {
        std::cout << "Sending current model to most recent client.\n";
        sim_server.send(net_connections[net_connections.size() - 1],
         most_recent_model, websocketpp::frame::opcode::text);

        has_unhandled_new_connection = false;

        for(std::size_t i = 0; i < param_cache.size(); ++i)
        {
          sim_server.send(net_connections[net_connections.size() - 1],
            param_cache[i], websocketpp::frame::opcode::text);
        }
      }

      if(net_messages.size() > 0)
      {
        net_msg_mtx.lock();
        for(std::size_t i = 0; i < net_messages.size(); ++i)
        {
          std::string msg_str = net_messages[i];
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
              isRunningSimulation = true;
              isSimulationPaused = false;

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
            } break;
            case id_vis_data_finish:
            {
              std::cout << "data finish received\n";
            } break;
            case id_vis_data_pause:
            {
              std::cout << "pause command received\n";
              if(isRunningSimulation)
              {
                isSimulationPaused = true;
              }
            } break;
            case id_vis_data_resume:
            {
              std::cout << "resume command received\n";
              if(isRunningSimulation)
              {
                isSimulationPaused = false;
              }
            } break;
            case id_vis_data_abort:
            {
              std::cout << "abort command received\n";
              isRunningSimulation = false;
              isSimulationPaused = false;
            } break;
            case id_update_time_step:
            {
              time_step = json_msg["time_step"].asFloat();
              std::cout << "time step updated to " << time_step << "\n";

              for(std::size_t i = 0; i < net_connections.size(); ++i)
              {
                auto sptr = net_connections[i].lock();
                if(sptr.get() == nullptr) { continue; }

                std::cout << "Sending timestep update to client " << i << "\n";
                sim_server.send(net_connections[i], msg_str, websocketpp::frame::opcode::text);
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
              for(std::size_t i = 0; i < net_connections.size(); ++i)
              {
                auto sptr = net_connections[i].lock();
                if(sptr.get() == nullptr) { continue; }

                std::cout << "Sending param update to client " << i << "\n";
                sim_server.send(net_connections[i], msg_str, websocketpp::frame::opcode::text);
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
              for(std::size_t i = 0; i < net_connections.size(); ++i)
              {
                auto sptr = net_connections[i].lock();
                if(sptr.get() == nullptr) { continue; }

                std::cout << "Sending model update to client " << i << "\n";
                sim_server.send(net_connections[i], msg_str, websocketpp::frame::opcode::text);
              }
            } break;
            case id_heartbeat_ping:
            {
              std::cout << "heartbeat ping arrived\n";
            } break;
            case id_heartbeat_pong:
            {
              auto conn_id = json_msg["conn_id"].asInt();
              std::cout << "heartbeat pong arrived from client " << conn_id << "\n";

              missed_net_heartbeats[conn_id] = 0;
            } break;
            case id_play_cache:
            {
              std::cout << "request to play cached arrived\n";
              simulation.PlayCacheFromFrame(0);
              isRunningSimulation = true;
              isSimulationPaused = false;
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

      if(!isRunningSimulation || isSimulationPaused)
      {
        start = std::chrono::steady_clock::now();
        continue;
      }

      auto now = std::chrono::steady_clock::now();
      auto diff = now - start;

      if(run_mode == id_pre_run_simulation)
      {
        simulation.RunAndSaveFrames(time_step, n_time_steps);
      }

      if(run_mode == id_traj_file_playback)
      {
        simulation.LoadTrajectoryFile("./data/traj/" + traj_file_name);
      }

      if(diff >= std::chrono::milliseconds(66))
      {
        start = now;
        Json::Value agents;
        agents["msg_type"] = id_vis_data_arrive;

        if(simulation.IsPlayingFromCache())
        {
          simulation.IncrementCacheFrame();
        }
        else if(run_mode == id_live_simulation)
        {
          simulation.RunTimeStep(time_step);
        }
        else if(run_mode == id_pre_run_simulation
          || run_mode == id_traj_file_playback)
        {
          if(simulation.HasLoadedAllFrames())
          {
            std::cout << "Simulation Finished" << std::endl;
            isRunningSimulation = false;
          }
          else
          {
            simulation.LoadNextFrame();
          }
        }

        auto simData = simulation.GetData();

        for(std::size_t i = 0; i < simData.size(); ++i)
        {
          auto agentData = simData[i];

          Json::Value agent;
          agent["type"] = agentData.type;
          agent["vis-type"] = agentData.vis_type;
          agent["x"] = agentData.x;
          agent["y"] = agentData.y;
          agent["z"] = agentData.z;

          agent["xrot"] = agentData.xrot;
          agent["yrot"] = agentData.yrot;
          agent["zrot"] = agentData.zrot;

          agent["collision-radius"] = agentData.collision_radius;

          auto subpointsData = Json::Value(Json::arrayValue);
          for(std::size_t j = 0; j < agentData.subpoints.size(); ++j)
          {
            int sp_index = static_cast<int>(j);
            subpointsData[sp_index] = agentData.subpoints[sp_index];
          }

          agent["subpoints"] = subpointsData;
          agents[std::to_string(i)] = agent;
        }

        std::string msg = Json::writeString(json_stream_writer, agents);
        for(std::size_t i = 0; i < net_connections.size(); ++i)
        {
          auto sptr = net_connections[i].lock();
          if(sptr.get() == nullptr) { continue; }

          sim_server.send(net_connections[i], msg, websocketpp::frame::opcode::text);
        }
      }
    }
  });

  auto heartbeat_thread = std::thread([&] {
    auto start = std::chrono::steady_clock::now();
    auto now = start;
    auto diff = now - start;

    Json::StreamWriterBuilder json_stream_writer;

    Json::CharReaderBuilder json_read_builder;
    std::unique_ptr<Json::CharReader> const json_reader(json_read_builder.newCharReader());

    while(isServerRunning)
    {
      now = std::chrono::steady_clock::now();
      diff = now - start;
      if(diff >= std::chrono::seconds(HEART_BEAT_INTERVAL_SECONDS))
      {
        if(net_messages.size() > 0)
        {
          net_msg_mtx.lock();
          for(std::size_t i = 0; i < net_messages.size(); ++i)
          {
            std::string msg_str = net_messages[i];
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
                auto conn_id = json_msg["conn_id"].asInt();
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

        for(std::size_t i = 0; i < net_connections.size(); ++i)
        {
          missed_net_heartbeats[i]++;

          auto sptr = net_connections[i].lock();
          if(sptr.get() == nullptr) { continue; }

          if(missed_net_heartbeats[i] > MAX_MISSED_HEARTBEATS_BEFORE_TIMEOUT)
          {
            std::cout << "Removing unresponsive network connection.\n";
            sim_server.pause_reading(net_connections[i]);
            sim_server.close(net_connections[i], 0, "");

            net_connections.erase(net_connections.begin()+i);
            --i;
            missed_net_heartbeats.erase(missed_net_heartbeats.begin()+i);
          }
          else
          {
            std::cout << "Sending ping heartbeat to connection " << i << ".\n";
            ping_data["conn_id"] = (int)i;
            std::string msg = Json::writeString(json_stream_writer, ping_data);
            sim_server.send(net_connections[i], msg, websocketpp::frame::opcode::text);
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
