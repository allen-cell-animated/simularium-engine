
#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>
#include <websocketpp/client.hpp>

#include <sys/stat.h>
#include <iostream>
#include <fstream>
#include <json/json.h>

#include "agentsim/agentsim.h"

typedef websocketpp::client<websocketpp::config::asio_client> client;

std::vector<std::string> net_messages;
websocketpp::connection_hdl server_connection;

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
  id_heartbeat_pong
};

inline bool file_exists (const std::string& name) {
  struct stat buffer;
  return (stat (name.c_str(), &buffer) == 0);
}

void on_message(websocketpp::connection_hdl, client::message_ptr msg)
{
	net_messages.push_back(msg->get_payload());
}

void on_open(websocketpp::connection_hdl hdl){
	server_connection = hdl;
	std::cout << "Connected to Server.\n";
}

int main(int argc, char* argv[])
{
	client sim_client;
	std::string uri = "ws://localhost:9002";
	std::atomic<bool> isClientRunning { true };

	if(argc == 2) {
		uri = argv[1];
	}

	auto ws_thread = std::thread([&] {
		try {
			sim_client.set_access_channels(websocketpp::log::alevel::none);
			sim_client.clear_access_channels(websocketpp::log::alevel::frame_payload);
			sim_client.set_error_channels(websocketpp::log::elevel::none);

			sim_client.init_asio();

			sim_client.set_message_handler(&on_message);
			sim_client.set_open_handler(&on_open);

			websocketpp::lib::error_code ec;
			client::connection_ptr con = sim_client.get_connection(uri, ec);
			if(ec)
			{
				std::cout << "could not create connection because: " << ec.message() << std::endl;
				return;
			}

			sim_client.connect(con);

			sim_client.run();
		} catch (websocketpp::exception const &e) {
			std::cout << e.what() << std::endl;
		}
	});

	auto client_thread = std::thread([&] {
		// Json cpp setup
		Json::StreamWriterBuilder json_stream_writer;

		Json::CharReaderBuilder json_read_builder;
    std::unique_ptr<Json::CharReader> const json_reader(json_read_builder.newCharReader());

		while(isClientRunning)
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
            case id_heartbeat_ping:
            {
              //std::cout << "heartbeat ping arrived\n";
							json_msg["msg_type"] = id_heartbeat_pong;

							std::string out_msg = Json::writeString(json_stream_writer, json_msg);
							sim_client.send(server_connection, out_msg, websocketpp::frame::opcode::text);
            } break;
            default:
            {
              //std::cout << "Received unexpected message of type " << msg_type << "\n";
            } break;
          }
        }

        net_messages.clear();
      }
		}
	});

	std::string input;
	std::cout << "Enter 'help' for commands\n";
  std::cout << "Enter 'quit' to exit\n";
  while(isClientRunning && std::getline(std::cin, input, '\n'))
  {
		Json::StreamWriterBuilder json_stream_writer;

    std::string delimiter = " ";

    size_t pos = 0;
		std::vector<std::string> tokens;
		while ((pos = input.find(delimiter)) != std::string::npos) {
        tokens.push_back(input.substr(0, pos));
				input.erase(0, pos + delimiter.length());
		}
    tokens.push_back(input);
    input = "";

		if(tokens[0] == "help")
    {
      std::cout << "\nCOMMANDS\n"
      << "> start\n"
      << "> stop\n"
      << "> load [model_name]\n"
      << "\n";
    }
		else if (tokens[0] == "load")
		{
      if(tokens.size() == 2)
      {
  			Json::Value json_msg;
        std::string file_path = "data/models/" + tokens[1] + ".json";

        if(!file_exists(file_path))
        {
          std::cout << "File " << file_path << " not found.\n";
          continue;
        }

  			std::ifstream file(file_path);
  			file >> json_msg;

  			json_msg["msg_type"] = id_model_definition;
  			std::string out_msg = Json::writeString(json_stream_writer, json_msg);
  			sim_client.send(server_connection, out_msg, websocketpp::frame::opcode::text);
      }
      else if (tokens.size() == 1)
      {
        std::cout << "The 'load' command requires a second parameter specifying the input file.\n";
      }
      else
      {
        std::cout << "Too many parameters for load command, expected 2.\n";
      }
		}
		else if (tokens[0] == "start")
		{
			Json::Value json_msg;
			json_msg["msg_type"] = id_vis_data_request;

      if(tokens.size() == 1)
      {
        std::cout << "'start' command requires second parameter: 'live' or 'precache'" << std::endl;
        continue;
      }
      else if(tokens[1] == "live")
      {
        json_msg["live"] = true;
      }
      else if(tokens[1] == "precache")
      {
        json_msg["live"] = false;
      }
      else
      {
        std::cout << "Unrecognized second parameter for command 'start'" << std::endl;
        continue;
      }

      std::string out_msg = Json::writeString(json_stream_writer, json_msg);
			sim_client.send(server_connection, out_msg, websocketpp::frame::opcode::text);
		}
    else if (tokens[0] == "set")
    {
      if(tokens.size() != 3)
      {
        std::cout << "Expected three tokens for command 'set': set param_name param_value" << std::endl;
      }
      else
      {
        Json::Value json_msg;
  			json_msg["msg_type"] = id_update_rate_param;
  			json_msg["param_name"] = tokens[1];
        std::string::size_type sz;
  			json_msg["param_value"] = std::stof(tokens[2].substr(sz));

  			std::string out_msg = Json::writeString(json_stream_writer, json_msg);

  			sim_client.send(server_connection, out_msg, websocketpp::frame::opcode::text);
      }
    }
    else if (tokens[0] == "stop")
		{
			Json::Value json_msg;
			json_msg["msg_type"] = id_vis_data_abort;
			std::string out_msg = Json::writeString(json_stream_writer, json_msg);

			sim_client.send(server_connection, out_msg, websocketpp::frame::opcode::text);
		}
    else if(tokens[0] == "quit")
    {
      isClientRunning = false;
    }
    else
    {
      std::cout << "Unrecognized command " << tokens[0] << "\n";
    }
  }

	std::cout << "Exiting Client... \n";
	client_thread.join();
	ws_thread.detach();
}
