#include <iostream>
#include <chrono>
#include <thread>
#include <mutex>
#include <vector>

#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

typedef websocketpp::server<websocketpp::config::asio> server;

std::mutex mtx;
std::vector<std::string> net_messages;

enum {
  id_vis_data_arrive = 0,
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

int main() {
  auto ws_thread = std::thread([&] {
    server print_server;

    print_server.set_message_handler(&on_message);
    print_server.set_access_channels(websocketpp::log::alevel::all);
    print_server.set_error_channels(websocketpp::log::elevel::all);

    print_server.init_asio();
    print_server.listen(9002);
    print_server.start_accept();

    print_server.run();
  });

  auto sim_thread = std::thread([&] {
    bool isRunningSimulation = false;
    auto start = std::chrono::steady_clock::now();

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
            } break;
            case id_vis_data_resume:
            {
              std::cout << "resume command received\n";
            } break;
            case id_vis_data_abort:
            {
              std::cout << "abort command received\n";
              isRunningSimulation = false;
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

      if(!isRunningSimulation)
      {auto start = std::chrono::steady_clock::now();
        continue;
      }

      auto now = std::chrono::steady_clock::now();
      auto diff = now - start;

      if(diff >= std::chrono::milliseconds(66))
      {
        start = now;
        std::cout << "Running Simulation?\n";
      }
    }
  });

  while(1)
  {

  }

  ws_thread.join();
  sim_thread.join();
}
