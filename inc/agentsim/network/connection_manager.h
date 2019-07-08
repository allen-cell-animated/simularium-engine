#ifndef AICS_CONNECTION_MANAGER_H
#define AICS_CONNECTION_MANAGER_H

#include <chrono>
#include <iostream>
#include <string>
#include <unordered_map>
#include <vector>

#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <json/json.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "agentsim/network/net_message_ids.h"
#include "agentsim/simulation.h"

typedef websocketpp::server<websocketpp::config::asio> server;

namespace aics {
namespace agentsim {

    enum ClientPlayState {
        Playing = 0,
        Paused = 1,
        Stopped,
        Finished
    };

    struct NetState {
        std::size_t frame_no = 0;
        ClientPlayState play_state = ClientPlayState::Stopped;
    };

    struct NetMessage {
        std::string senderUid;
        Json::Value jsonMessage;
    };

    class ConnectionManager {
    public:
        void Listen();

        void AddConnection(websocketpp::connection_hdl hd1);
        void RemoveConnection(std::string connectionUID);
        void CloseConnection(std::string connectionUID);
        void MarkConnectionExpired(websocketpp::connection_hdl hd1);
        void RemoveExpiredConnections();
        bool HasActiveClient();
        std::size_t NumberOfClients();

        std::string GetUid(websocketpp::connection_hdl hd1);

        void SetClientState(std::string connectionUID, ClientPlayState state);
        void SetClientFrame(std::string connectionUID, std::size_t frameNumber);

        void SendWebsocketMessage(std::string connectionUID, Json::Value jsonMessage);
        void SendWebsocketMessageToAll(Json::Value jsonMessage, std::string description);

        void CheckForFinishedClients(std::size_t numberOfFrames, bool allFramesLoaded);
        void AdvanceClients();
        void SendDataToClients(Simulation& simulation);

        void SetNoTimeoutArg(bool val);
        bool CheckNoClientTimeout();
        void RemoveUnresponsiveClients();
        void RegisterHeartBeat(std::string connectionUID);
        void PingAllClients();

        void BroadcastParameterUpdate(Json::Value updateMessage);
        void BroadcastModelDefinition(Json::Value modelDefinition);

        void UpdateNewConections();
        void OnMessage(websocketpp::connection_hdl hd1, server::message_ptr msg);

        static ConnectionManager& Get() {
            static ConnectionManager connManager;
            return connManager;
        }

        std::vector<NetMessage>& GetMessages() { return this->m_simThreadMessages; }
        void HandleMessage(NetMessage nm);

    private:
        ConnectionManager();
        void GenerateLocalUUID(std::string& uuid);

        std::unordered_map<std::string, NetState> m_netStates;
        std::unordered_map<std::string, websocketpp::connection_hdl> m_netConnections;
        std::unordered_map<std::string, std::size_t> m_missedHeartbeats;
        server m_server;
        std::vector<std::string> m_uidsToDelete;

        Json::StreamWriterBuilder m_jsonStreamWriter;
        const std::size_t kLatestFrameValue = std::numeric_limits<std::size_t>::max();
        const std::size_t kMaxMissedHeartBeats = 4;
        const std::size_t kNoClientTimeoutSeconds = 30;

        bool m_argNoTimeout = false;

        std::chrono::time_point<std::chrono::system_clock>
            m_noClientTimer = std::chrono::system_clock::now();

        std::vector<Json::Value> m_paramCache;
        bool m_hasNewConnection = false;
        Json::Value m_mostRecentModel;
        std::string m_latestConnectionUid;
        bool m_hasModel = false;

        std::vector<NetMessage> m_simThreadMessages;
    };

} // namespace agentsim
} // namespace aics

#endif // AICS_CONNECTION_MANAGER_H
