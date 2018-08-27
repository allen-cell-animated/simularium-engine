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

typedef websocketpp::server<websocketpp::config::asio> server;
using namespace aics::agentsim;

std::vector<std::string> net_messages;
std::vector<websocketpp::connection_hdl> net_connections;

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
  id_model_definition
};

void on_message(websocketpp::connection_hdl, server::message_ptr msg) {
  net_messages.push_back(msg->get_payload());
}

void on_close(websocketpp::connection_hdl hd1) {
  for(std::size_t i = 0; i < net_connections.size(); ++i)
  {
    if(net_connections[i].expired())
    {
      net_connections.erase(net_connections.begin()+i);
      --i;
      continue;
    }
  }
}

void on_open(websocketpp::connection_hdl hd1) {
  net_connections.push_back(hd1);
}

int main() {
  server sim_server;

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

    // Json cpp setup
    Json::StreamWriterBuilder json_stream_writer;

    Json::CharReaderBuilder json_read_builder;
    std::unique_ptr<Json::CharReader> const json_reader(json_read_builder.newCharReader());

    // Simulation setup
    ReaDDyPkg* readdySimPkg = new ReaDDyPkg();

    std::shared_ptr<SimPkg> readdyPkg;
    readdyPkg.reset(readdySimPkg);

  	std::vector<std::shared_ptr<SimPkg>> simulators;
    simulators.push_back(readdyPkg);http://eric-pc:8000/

    std::vector<std::shared_ptr<Agent>> agents;
  	Simulation simulation(simulators, agents);

    // Runtime loop
    while(1)
    {
      if(net_messages.size() > 0)
      {
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
            } break;
            case id_update_rate_param:
            {
              std::string param_name = json_msg["param_name"].asString();
              float param_value = json_msg["param_value"].asFloat();
              simulation.UpdateParameter(param_name, param_value);

              std::cout << "rate param " << param_name <<
              " updated to " << param_value << "\n";
            } break;
            case id_model_definition:
            {
                std::cout << "model definition arrived\n";
                aics::agentsim::Model sim_model;
                parse_model(json_msg, sim_model);
                simulation.SetModel(sim_model);
            } break;
            default:
            {
              std::cout << "Received unrecognized message of type " << msg_type << "\n";
            } break;
          }
        }

        net_messages.clear();
      }

      if(!isRunningSimulation || isSimulationPaused)
      {
        auto start = std::chrono::steady_clock::now();
        continue;
      }

      auto now = std::chrono::steady_clock::now();
      auto diff = now - start;

      if(diff >= std::chrono::milliseconds(66))
      {
        start = now;
        Json::Value agents;
        agents["msg_type"] = id_vis_data_arrive;

        simulation.RunTimeStep(time_step);
        auto simData = simulation.GetData();

        for(std::size_t i = 0; i < simData.size(); ++i)
        {
          auto agentData = simData[i];

          Json::Value agent;
          agent["type"] = agentData.type;
          agent["x"] = agentData.x;
          agent["y"] = agentData.y;
          agent["z"] = agentData.z;

          agent["xrot"] = agentData.xrot;
          agent["yrot"] = agentData.yrot;
          agent["zrot"] = agentData.zrot;

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

  while(1)
  {

  }

  ws_thread.join();
  sim_thread.join();
}
