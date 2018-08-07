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

typedef websocketpp::server<websocketpp::config::asio> server;

std::mutex mtx;
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
  id_update_rate_param
};

void on_message(websocketpp::connection_hdl, server::message_ptr msg) {
  mtx.lock();
  net_messages.push_back(msg->get_payload());
  mtx.unlock();
}

void on_close(websocketpp::connection_hdl hd1) {
  std::string msg = " ";
  msg[0] = (char)id_vis_data_abort;
  net_messages.push_back(msg);

  for(std::size_t i = 0; i < net_connections.size(); ++i)
  {
    auto spt = net_connections[i].lock();
    auto spt2 = hd1.lock();

    if(spt.get() == spt2.get())
    {
      net_connections.erase(net_connections.begin()+i);
      break;
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
    bool isRunningSimulation = false;
    bool isSimulationPaused = false;
    auto start = std::chrono::steady_clock::now();

    Json::StreamWriterBuilder json_stream_writer;

    while(1)
    {
      if(net_messages.size() > 0)
      {
        mtx.lock();
        for(std::size_t i = 0; i < net_messages.size(); ++i)
        {
          std::string msg_str = net_messages[i];
          int msg_type = msg_str[0];
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
              std::cout << "time step updated\n";
            } break;
            case id_update_rate_param:
            {
              std::cout << "rate param updated\n";
            } break;
            default:
            {
              std::cout << "Received unrecognized message of type " << msg_type << "\n";
            } break;
          }
        }

        net_messages.clear();
        mtx.unlock();
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
        for(std::size_t i = 0; i < 5; ++i)
        {
          Json::Value agent;
          agent["type"] = 0;
          agent["x"] = i;
          agent["y"] = i;
          agent["z"] = i;
          agents[std::to_string(i)] = agent;
        }

        mtx.lock();
        std::string msg = Json::writeString(json_stream_writer, agents);
        sim_server.send(net_connections[0], msg, websocketpp::frame::opcode::text);
        mtx.unlock();
      }
    }
  });

  while(1)
  {

  }

  ws_thread.join();
  sim_thread.join();
}
