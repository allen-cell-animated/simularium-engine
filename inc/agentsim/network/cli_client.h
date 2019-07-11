#ifndef AICS_CLI_CLIENT_H
#define AICS_CLI_CLIENT_H

#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include <json/json.h>
#include <string>
#include <thread>
#include <unordered_map>

typedef websocketpp::client<websocketpp::config::asio_client> client;

namespace aics {
namespace agentsim {

    // A websocket client for command line
    //  can issue commands to the agentsim server
    class CliClient {
    public:
        CliClient(std::string uri);
        void Parse(std::string);

        void Help();
        void Connect(std::string uri);
        void Disconnect();
        bool IsConnected();
        void Listen();

        void Resume();
        void Pause();
        void Stop();
        void StartLive();
        void StartPrecache(std::size_t numberOfSteps, float timeStepSize);
        void StartTrajectory(std::size_t fileName);
        void Set(std::string name, float value);

        void Load(std::string fileName);

        void OnMessage(websocketpp::connection_hdl, client::message_ptr msg);
        void OnOpen(websocketpp::connection_hdl hdl);
        void OnClose(websocketpp::connection_hdl hdl);

        void SendMessage(Json::Value message);
        void OnFail();

    private:
        std::unordered_map<std::string, std::string> m_commandList = {
            { "load", "send request to load a trajectory json file" },
            { "start", "run a remote simluation" },
            { "stop", "end the remote simulation" },
            { "pause", "pause the remote simulation" },
            { "resume", "sets the client state to playing" },
            { "set", "set a parameter value" },
            { "quit", "disconnect and exit client" }
        };

        client m_webSocketClient;
        websocketpp::connection_hdl m_serverConnection;
        Json::StreamWriterBuilder m_jsonStreamWriter;

        std::thread m_listeningThread;
    };

} // namespace agentsim
} // namespace aics

#endif // AICS_CLI_CLIENT_H
