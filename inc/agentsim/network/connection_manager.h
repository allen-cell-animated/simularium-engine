#ifndef AICS_CONNECTION_MANAGER_H
#define AICS_CONNECTION_MANAGER_H

#include <chrono>
#include <cstdlib>
#include <iostream>
#include <string>
#include <thread>
#include <unordered_map>
#include <vector>
#include <mutex>
#include <queue>

#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <json/json.h>
#include <websocketpp/config/asio.hpp>
#include <websocketpp/server.hpp>

#include "agentsim/network/net_message_ids.h"
#include "agentsim/network/trajectory_properties.h"
#include "agentsim/simulation.h"

typedef websocketpp::server<websocketpp::config::asio_tls> server;
typedef websocketpp::lib::shared_ptr<websocketpp::lib::asio::ssl::context> context_ptr;

namespace aics {
namespace agentsim {

    enum ClientPlayState {
        Playing = 0,
        Paused = 1,
        Waiting,
        Stopped,
        Finished
    };

    struct NetState {
        std::size_t frame_no = 0;
        ClientPlayState play_state = ClientPlayState::Stopped;
        std::string sim_identifier = "runtime";
    };

    struct NetMessage {
        std::string senderUid;
        Json::Value jsonMessage;
    };

    struct FileRequest {
        std::string fileName;
        std::string senderUid;
        int frameNumber = -1;
    };

    class ConnectionManager {
    public:
        ConnectionManager();
        void ListenAsync();
        void StartHeartbeatAsync(std::atomic<bool>& isRunning);
        void StartSimAsync(
            std::atomic<bool>& isRunning,
            Simulation& simulation,
            float& timeStep);
        void StartFileIOAsync(
            std::atomic<bool>& isRunning,
            Simulation& simulation);

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
        void SetClientSimId(std::string connectionUID, std::string simId);

        void SendWebsocketMessage(std::string connectionUID, Json::Value jsonMessage);
        void SendWebsocketMessageToAll(Json::Value jsonMessage, std::string description);

        void CheckForFinishedClients(
            Simulation& simulation
        );
        void AdvanceClients();
        void SendDataToClients(Simulation& simulation);

        void SetNoUploadArg(bool val) { this->m_argNoUpload = val; }
        void SetForceInitArg(bool val) { this->m_argForceInit = val; }
        void SetNoTimeoutArg(bool val) { this->m_argNoTimeout = val; }

        bool CheckNoClientTimeout();
        void RemoveUnresponsiveClients();
        void RegisterHeartBeat(std::string connectionUID);
        void PingAllClients();

        void BroadcastParameterUpdate(Json::Value updateMessage);
        void BroadcastModelDefinition(Json::Value modelDefinition);

        void UpdateNewConections();
        void OnMessage(websocketpp::connection_hdl hd1, server::message_ptr msg);

        std::vector<NetMessage>& GetMessages() { return this->m_simThreadMessages; }
        void HandleMessage(NetMessage nm);

        // Enacts web-socket commands in the sim thread
        // e.g. changing parameters, time-step, starting, stopping, etc.
        void HandleNetMessages(Simulation& simulation, float& timeStep);
        void CloseServer();

        enum TLS_MODE {
            MOZILLA_INTERMEDIATE = 1,
            MOZILLA_MODERN = 2
        };

        context_ptr OnTLSConnect(TLS_MODE mode, websocketpp::connection_hdl hdl);

    private:
        void GenerateLocalUUID(std::string& uuid);

        void SendSingleFrameToClient(
            Simulation& simulation,
            std::string connectionUID,
            std::size_t frameNumber
        ) { this->SendDataToClient(simulation, connectionUID, frameNumber, 1, true); }

        void SendDataToClient(
            Simulation& simulation,
            std::string connectionUID,
            std::size_t startingFrame,
            std::size_t numberOfFrames,
            bool force = false // ignore play state & all conditions, just send
        );

        void CheckForFinishedClient(
            Simulation& simulation,
            std::string connectionUID
        );

        void InitializeTrajectoryFile(
            Simulation& simulation,
            std::string connectionUID,
            std::string fileName
        );

        /**
        * SetupRuntimeCacheAsync
        *
        *   @param simulation: the simulation object used by the other functions
        *   in this class; responsible for loading trajectories, running simulations
        *   and keeping the run-time cache updated
        */
        void SetupRuntimeCache(
            Simulation& simulation
        );

        /**
        *   TLS Functions
        */
        std::string GetPassword() {
            return std::getenv("TLS_PASSWORD") ? std::getenv("TLS_PASSWORD") : "";
        }
        std::string GetCertificateFilepath() {
            return std::getenv("TLS_CERT_PATH") ? std::getenv("TLS_CERT_PATH") : "";
        }
        std::string GetKeyFilepath() {
            return std::getenv("TLS_KEY_PATH") ? std::getenv("TLS_KEY_PATH") : "";
        }

        void LogClientEvent(std::string uid, std::string msg);

        std::unordered_map<std::string, NetState> m_netStates;
        std::unordered_map<std::string, websocketpp::connection_hdl> m_netConnections;
        std::unordered_map<std::string, std::size_t> m_missedHeartbeats;
        server m_server;
        std::vector<std::string> m_uidsToDelete;

        Json::StreamWriterBuilder m_jsonStreamWriter;
        const std::size_t kMaxMissedHeartBeats = 4;
        const std::size_t kHeartBeatIntervalSeconds = 15;
        const std::size_t kNoClientTimeoutSeconds = 30;
        const std::size_t kServerTickIntervalMilliSeconds = 200;
        const std::size_t kNumberOfFramesToBulkBroadcast = 100;
        const std::size_t kFileIoCheckIntervalMilliSeconds = 100;

        bool m_argNoTimeout = false;
        bool m_argForceInit = false;
        bool m_argNoUpload = false;

        std::chrono::time_point<std::chrono::system_clock>
            m_noClientTimer = std::chrono::system_clock::now();

        std::vector<Json::Value> m_paramCache;
        bool m_hasNewConnection = false;
        Json::Value m_mostRecentModel;
        std::string m_latestConnectionUid;
        bool m_hasModel = false;

        std::vector<NetMessage> m_simThreadMessages;
        std::queue<FileRequest> m_fileRequests;
        std::thread m_listeningThread;
        std::thread m_heartbeatThread;
        std::thread m_simThread;
        std::thread m_fileIoThread;
        std::mutex m_fileMutex;
    };

} // namespace agentsim
} // namespace aics

#endif // AICS_CONNECTION_MANAGER_H
