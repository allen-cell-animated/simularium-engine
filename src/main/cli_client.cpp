#include "simularium/network/cli_client.h"
#include "simularium/network/net_message_ids.h"

#include <fstream>
#include <iostream>
#include <json/json.h>
#include <stdexcept>
#include <sys/stat.h>
#include <thread>

inline bool file_exists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}


namespace aics {
namespace simularium {
    void printAgentData(const Json::Value& jsonMsg)
    {
        const Json::Value& data = jsonMsg["data"];
        std::size_t frameNumber = jsonMsg["frameNumber"].asInt();
        float time = jsonMsg["time"].asFloat();

        Json::ArrayIndex size = data.size();
        std::size_t agentCount = 0;
        Json::ArrayIndex i = 0;
        std::cout << std::endl << "Frame " << frameNumber << " [" << time << "]:" << std::endl;

        while(i < size)
        {
            std::cout << "Agent " << agentCount++ << ": "
            << "vis-type: " << data[i++].asFloat() << " | "
            << "type: " << data[i++].asFloat() << " | "
            << "pos: ["
                << data[i++].asFloat() << ", "
                << data[i++].asFloat() << ", "
                << data[i++].asFloat() <<"]"
                << " | "
            << "rot: ["
                << data[i++].asFloat() << ", "
                << data[i++].asFloat() << ", "
                << data[i++].asFloat() <<"]"
                << " | "
            << "col-radius: " << data[i++].asFloat()
            << std::endl;

            Json::ArrayIndex nSubPoints = data[i++].asInt();
            i += nSubPoints; // don't care right now
        }
    }

    CliClient::CliClient(std::string uri)
    {
        this->Connect(uri);
    }

    void CliClient::Parse(std::string input)
    {
        std::string delimiter = " ";
        size_t pos = 0;
        std::vector<std::string> tokens;
        while ((pos = input.find(delimiter)) != std::string::npos) {
            tokens.push_back(input.substr(0, pos));
            input.erase(0, pos + delimiter.length());
        }
        tokens.push_back(input);
        input = "";

        auto command = tokens[0];

        if (!this->m_commandList.count(command)) {
            std::cout << "Unrecognized command :" << command << std::endl;
        }

        if (command == "help") {
            this->Help();
        }
        if (command == "stop") {
            this->Stop();
        }
        if (command == "pause") {
            this->Pause();
        }

        if (command == "load") {
            if (tokens.size() != 2) {
                std::cout << "Usage: 'load [filename]'" << std::endl;
                return;
            }

            this->Load(tokens[1]);
        }

        if (command == "start") {
            if (tokens[1] == "live" && tokens.size() == 2) {
                this->StartLive();
            } else if (tokens[1] == "precache" && tokens.size() == 4) {
                std::string::size_type sz;
                this->StartPrecache(std::stoi(tokens[2]), std::stof(tokens[3].substr(sz)));
            } else if (tokens[1] == "trajectory" && tokens.size() == 3) {
                this->StartTrajectory(tokens[2]);
            } else {
                std::cout << "Usage: \
            \n 'start live' \
            \n 'start precache [time step count] [time step size (seconds)]' \
            \n 'start trajectory [filename]'"
                          << std::endl;
            }
        }

        if (command == "set") {
            if (tokens.size() != 3) {
                std::cout << "Usage: 'set [parameter name] [parameter value]'" << std::endl;
                return;
            }

            std::string::size_type sz;
            this->Set(tokens[1], std::stof(tokens[2].substr(sz)));
        }

        if (command == "connect") {
            if (tokens.size() != 2) {
                std::cout << "Usage: 'connect' [uri]" << std::endl;
                return;
            }

            this->Connect(tokens[1]);
        }

        if (command == "quit") {
            this->Disconnect();
        }

        if (command == "resume") {
            this->Resume();
        }
    }

    void CliClient::Help()
    {
        std::cout << "\nCOMMANDS: \
    \n________________________"
                  << std::endl;

        for (auto& entry : this->m_commandList) {
            auto commandName = entry.first;
            while (commandName.length() < 15) {
                commandName = commandName + " ";
            }

            std::cout << commandName << " | " << entry.second << std::endl;
        }
    }

    void CliClient::Connect(std::string uri)
    {
        this->m_webSocketClient.set_access_channels(websocketpp::log::alevel::none);
        this->m_webSocketClient.clear_access_channels(websocketpp::log::alevel::frame_payload);
        this->m_webSocketClient.set_error_channels(websocketpp::log::elevel::none);

        this->m_webSocketClient.init_asio();

        this->m_webSocketClient.set_message_handler(
            std::bind(
                &CliClient::OnMessage,
                this,
                std::placeholders::_1,
                std::placeholders::_2));
        this->m_webSocketClient.set_open_handler(
            std::bind(
                &CliClient::OnOpen,
                this,
                std::placeholders::_1));
        this->m_webSocketClient.set_close_handler(
            std::bind(
                &CliClient::OnClose,
                this,
                std::placeholders::_1));
        this->m_webSocketClient.set_fail_handler(
            std::bind(
                &CliClient::OnFail,
                this));
        this->m_webSocketClient.set_tls_init_handler(
            std::bind(
                &CliClient::OnTLSConnect,
                this
        ));

        websocketpp::lib::error_code ec;
        client::connection_ptr con = this->m_webSocketClient.get_connection(uri, ec);
        if (ec) {
            std::cout << "could not create connection because: " << ec.message() << std::endl;
            return;
        }

        this->m_webSocketClient.connect(con);
        this->m_listeningThread = std::thread([&] {
            this->Listen();
        });
    }

    void CliClient::Disconnect()
    {
        if(this->m_listeningThread.joinable())
        {
            this->m_webSocketClient.stop();
            this->m_listeningThread.detach();
        }
    }

    void CliClient::Listen()
    {
        try {
            this->m_webSocketClient.run();
        } catch (websocketpp::exception const& e) {
            std::cout << e.what() << std::endl;
        }
    }

    void CliClient::Resume()
    {
        Json::Value jsonMsg;
        jsonMsg["msgType"] = WebRequestTypes::id_vis_data_resume;
        jsonMsg["mode"] = SimulationMode::id_live_simulation;
        this->SendMessage(jsonMsg, "resume command");
    }

    void CliClient::Pause()
    {
        Json::Value jsonMsg;
        jsonMsg["msgType"] = WebRequestTypes::id_vis_data_pause;
        this->SendMessage(jsonMsg, "pause command");
    }

    void CliClient::StartLive()
    {
        Json::Value jsonMsg;
        jsonMsg["msgType"] = WebRequestTypes::id_vis_data_request;
        jsonMsg["mode"] = SimulationMode::id_live_simulation;
        this->SendMessage(jsonMsg, "start command live");
    }

    void CliClient::StartPrecache(std::size_t numberOfSteps, float timeStepSize)
    {
        Json::Value jsonMsg;
        jsonMsg["msgType"] = WebRequestTypes::id_vis_data_request;
        jsonMsg["mode"] = SimulationMode::id_pre_run_simulation;
        jsonMsg["timeStep"] = 1e-9f;
        jsonMsg["numTimeSteps"] = 200;
        this->SendMessage(jsonMsg, "start command precache");
    }

    void CliClient::StartTrajectory(std::string fileName)
    {
        Json::Value jsonMsg;
        jsonMsg["msgType"] = WebRequestTypes::id_vis_data_request;
        jsonMsg["mode"] = SimulationMode::id_traj_file_playback;
        jsonMsg["file-name"] = fileName;
        this->SendMessage(jsonMsg, "start command trajectory " + fileName);
    }

    void CliClient::Stop()
    {
        Json::Value jsonMsg;
        jsonMsg["msgType"] = WebRequestTypes::id_vis_data_abort;
        this->SendMessage(jsonMsg, "stop command");
    }

    void CliClient::Set(std::string name, float value)
    {
        Json::Value jsonMsg;
        jsonMsg["msgType"] = WebRequestTypes::id_update_rate_param;
        jsonMsg["paramName"] = name;
        jsonMsg["paramValue"] = value;
        this->SendMessage(jsonMsg, "set command " + name);
    }

    void CliClient::Load(std::string fileName)
    {
        Json::Value jsonMsg;
        std::string file_path = "data/models/" + fileName;

        if (!file_exists(file_path)) {
            std::cout << "File " << file_path << " not found.\n";
            return;
        }

        std::ifstream file(fileName);
        file >> jsonMsg;

        jsonMsg["msgType"] = WebRequestTypes::id_model_definition;
        this->SendMessage(jsonMsg, "load command " + fileName);
    }

    context_ptr CliClient::OnTLSConnect() {
        namespace asio = websocketpp::lib::asio;
        context_ptr ctx =
            websocketpp::lib::make_shared<asio::ssl::context>(
                asio::ssl::context::sslv23
            );

        try {
            ctx->set_options(
                asio::ssl::context::default_workarounds |
                asio::ssl::context::no_sslv2 |
                asio::ssl::context::no_sslv3 |
                asio::ssl::context::single_dh_use
            );
        } catch(std::exception& e) {
            std::cout << "Exception: " << e.what() << std::endl;
        }

        return ctx;
    }

    void CliClient::OnMessage(websocketpp::connection_hdl, client::message_ptr msg)
    {
        Json::CharReaderBuilder jsonReaderBuilder;
        std::unique_ptr<Json::CharReader> const jsonReader(jsonReaderBuilder.newCharReader());

        std::string msg_str = msg->get_payload();
        std::string errs;
        Json::Value jsonMsg;
        jsonReader->parse(msg_str.c_str(), msg_str.c_str() + msg_str.length(),
            &jsonMsg, &errs);

        int msgType = jsonMsg["msgType"].asInt();

        switch (msgType) {
        case WebRequestTypes::id_heartbeat_ping: {
            jsonMsg["msgType"] = WebRequestTypes::id_heartbeat_pong;
            this->SendMessage(jsonMsg, "heartbeat ping");
        } break;
        case WebRequestTypes::id_vis_data_arrive: {
            //printAgentData(jsonMsg);
        } break;
        default: {
        } break;
        }
    }

    void CliClient::OnOpen(websocketpp::connection_hdl hdl)
    {
        this->m_serverConnection = hdl;
        std::cout << "Connected to server" << std::endl;
    }

    void CliClient::OnClose(websocketpp::connection_hdl hdl)
    {
        std::cout << "Closing client" << std::endl;
    }

    void CliClient::SendMessage(Json::Value message, std::string description)
    {
        try {
            std::string outMsg = Json::writeString(this->m_jsonStreamWriter, message);
            this->m_webSocketClient.send(this->m_serverConnection, outMsg, websocketpp::frame::opcode::text);
        } catch (...) {
            std::cout << "Websocket send failed: " << description << std::endl;
        }
    }

    void CliClient::OnFail()
    {
        throw std::logic_error("Web socket client requires a connection");
    }

} // namespace simularium
} // namespace aics
