#include "agentsim/network/cli_client.h"
#include "agentsim/network/net_message_ids.h"

#include <fstream>
#include <iostream>
#include <json/json.h>
#include <sys/stat.h>
#include <thread>
#include <stdexcept>

inline bool file_exists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

namespace aics {
namespace agentsim {

CliClient::CliClient(std::string uri) {
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

    if(!this->m_commandList.count(command))
    {
        std::cout << "Unrecognized command :" << command << std::endl;
    }

    if(command == "help") { this->Help(); }
    if(command == "stop") { this->Stop(); }
    if(command == "pause") { this->Pause(); }

    if(command == "load") {
        if(tokens.size() != 2)
        {
            std::cout << "Usage: 'load [filename]'" << std::endl;
            return;
        }

        this->Load(tokens[1]);
    }

    if(command == "start") {
        if(tokens[1] == "live" && tokens.size() == 2) {
            this->StartLive();
        }
        else if(tokens[1] == "precache" && tokens.size() == 4) {
            std::string::size_type sz;
            this->StartPrecache(std::stoi(tokens[2]), std::stof(tokens[3].substr(sz)));
        }
        else if(tokens[1] == "trajectory" && tokens.size() == 3) {
            this->StartLive();
        }
        else {
            std::cout << "Usage: \
            \n 'start live' \
            \n 'start precache [time step count] [time step size (seconds)]' \
            \n 'start trajectory [filename]'" << std::endl;
        }
    }

    if(command == "set")
    {
        if(tokens.size() != 3)
        {
            std::cout << "Usage: 'set [parameter name] [parameter value]'" << std::endl;
            return;
        }

        std::string::size_type sz;
        this->Set(tokens[1], std::stof(tokens[2].substr(sz)));
    }

    if(command == "connect")
    {
        if(tokens.size() != 2)
        {
            std::cout << "Usage: 'connect' [uri]" << std::endl;
            return;
        }

        this->Connect(tokens[1]);
    }

    if(command == "quit")
    {
        this->Disconnect();
    }

    if(command == "resume")
    {
        this->Resume();
    }
}

void CliClient::Help()
{
    std::cout << "\nCOMMANDS: \
    \n________________________" << std::endl;

    for(auto& entry : this->m_commandList)
    {
        auto commandName = entry.first;
        while(commandName.length() < 15)
        {
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
            std::placeholders::_2)
    );
    this->m_webSocketClient.set_open_handler(
        std::bind(
            &CliClient::OnOpen,
            this,
            std::placeholders::_1)
    );
    this->m_webSocketClient.set_close_handler(
        std::bind(
            &CliClient::OnClose,
            this,
            std::placeholders::_1)
    );
    this->m_webSocketClient.set_fail_handler(
        std::bind(
            &CliClient::OnFail,
            this)
    );

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
    try {
        this->m_webSocketClient.pause_reading(this->m_serverConnection);
        this->m_webSocketClient.close(
            this->m_serverConnection,
            websocketpp::close::status::normal,
            "Sucess");

        this->m_listeningThread.detach();
    } catch (websocketpp::exception const& e) {
        std::cout << e.what() << std::endl;
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
    jsonMsg["msg_type"] = WebRequestTypes::id_vis_data_resume;
    jsonMsg["mode"] = id_live_simulation;
    this->SendMessage(jsonMsg);
}

void CliClient::Pause()
{
    Json::Value jsonMsg;
    jsonMsg["msg_type"] = WebRequestTypes::id_vis_data_pause;
    this->SendMessage(jsonMsg);
}

void CliClient::StartLive()
{
    Json::Value jsonMsg;
    jsonMsg["msg_type"] = WebRequestTypes::id_vis_data_request;
    jsonMsg["mode"] = id_live_simulation;
    this->SendMessage(jsonMsg);
}

void CliClient::StartPrecache(std::size_t numberOfSteps, float timeStepSize)
{
    Json::Value jsonMsg;
    jsonMsg["msg_type"] = WebRequestTypes::id_vis_data_request;
    jsonMsg["mode"] = id_pre_run_simulation;
    jsonMsg["time-step"] = 1e-9f;
    jsonMsg["num-time-steps"] = 200;
    this->SendMessage(jsonMsg);
}

void CliClient::StartTrajectory(std::size_t fileName)
{
    Json::Value jsonMsg;
    jsonMsg["msg_type"] = WebRequestTypes::id_vis_data_request;
    jsonMsg["mode"] = id_traj_file_playback;
    jsonMsg["file-name"] = fileName;
    this->SendMessage(jsonMsg);
}

void CliClient::Stop() {
    Json::Value jsonMsg;
    jsonMsg["msg_type"] = WebRequestTypes::id_vis_data_abort;
    this->SendMessage(jsonMsg);
}

void CliClient::Set(std::string name, float value)
{
    Json::Value jsonMsg;
    jsonMsg["msg_type"] = WebRequestTypes::id_update_rate_param;
    jsonMsg["param_name"] = name;
    jsonMsg["param_value"] = value;
    this->SendMessage(jsonMsg);
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

    jsonMsg["msg_type"] = WebRequestTypes::id_model_definition;
    this->SendMessage(jsonMsg);
}

void CliClient::OnMessage(websocketpp::connection_hdl, client::message_ptr msg) {
    Json::CharReaderBuilder jsonReaderBuilder;
    std::unique_ptr<Json::CharReader> const jsonReader(jsonReaderBuilder.newCharReader());

    std::string msg_str = msg->get_payload();
    std::string errs;
    Json::Value jsonMsg;
    jsonReader->parse(msg_str.c_str(), msg_str.c_str() + msg_str.length(),
        &jsonMsg, &errs);

    int msgType = jsonMsg["msg_type"].asInt();

    switch (msgType) {
    case WebRequestTypes::id_heartbeat_ping: {
        jsonMsg["msg_type"] = WebRequestTypes::id_heartbeat_pong;
        this->SendMessage(jsonMsg);
    } break;
    default: { } break;
    }
}

void CliClient::OnOpen(websocketpp::connection_hdl hdl)
{
    this->m_serverConnection = hdl;
    std::cout << "Connected to server" << std::endl;
}

void CliClient::OnClose(websocketpp::connection_hdl hdl)
{

}

void CliClient::SendMessage(Json::Value message)
{
    try {
        std::string outMsg = Json::writeString(this->m_jsonStreamWriter, message);
        this->m_webSocketClient.send(this->m_serverConnection, outMsg, websocketpp::frame::opcode::text);
    }
    catch(...)
    {
        std::cout << "Websocket send failed" << std::endl;
    }
}

void CliClient::OnFail()
{
    throw std::logic_error("Web socket client requires a connection");
}

} // namespace agentsim
} // namespace aics
