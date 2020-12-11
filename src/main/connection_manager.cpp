#include "simularium/network/connection_manager.h"
#include "simularium/network/net_message_ids.h"
#include "simularium/network/trajectory_properties.h"
#include "simularium/aws/aws_util.h"
#include "loguru/loguru.hpp"
#include <fstream>
#include <iostream>

template <typename T, typename U>
inline bool equals(const std::weak_ptr<T>& t, const std::weak_ptr<U>& u)
{
    return !t.owner_before(u) && !u.owner_before(t);
}

static const std::string LIVE_SIM_IDENTIFIER = "live";

namespace aics {
namespace simularium {

    ConnectionManager::ConnectionManager()
    {
        LOG_F(INFO, "Connection Manager Created");
    }

    void ConnectionManager::CloseServer()
    {
        if (this->m_fileIoThread.joinable()) {
            this->m_fileIoThread.join();
        }

        if (this->m_heartbeatThread.joinable()) {
            this->m_heartbeatThread.join();
        }

        if (this->m_simThread.joinable()) {
            this->m_simThread.join();
        }

        if (this->m_listeningThread.joinable()) {
            this->m_server.stop();
            this->m_listeningThread.join();
        }
    }

    void ConnectionManager::LogClientEvent(std::string uid, std::string msg) {
        LOG_F(INFO, "[%s] %s", uid.c_str(), msg.c_str());
    }

    context_ptr ConnectionManager::OnTLSConnect(
        TLS_MODE mode,
        websocketpp::connection_hdl hdl
    ) {
        namespace asio = websocketpp::lib::asio;
        context_ptr ctx =
            websocketpp::lib::make_shared<asio::ssl::context>(
                asio::ssl::context::sslv23
            );

        try {
            if(mode == TLS_MODE::MOZILLA_MODERN)
            {
                ctx->set_options(
                    asio::ssl::context::default_workarounds |
                    asio::ssl::context::no_sslv2 |
                    asio::ssl::context::no_sslv3 |
                    asio::ssl::context::no_tlsv1 |
                    asio::ssl::context::single_dh_use
                );
            } else {
                ctx->set_options(
                    asio::ssl::context::default_workarounds |
                    asio::ssl::context::no_sslv2 |
                    asio::ssl::context::no_sslv3 |
                    asio::ssl::context::single_dh_use
                );
            }
            ctx->set_password_callback(
                std::bind(
                    &ConnectionManager::GetPassword,
                    this
                ));

            auto certFilePath = this->GetCertificateFilepath();
            auto keyFilePath = this->GetKeyFilepath();
            ctx->use_certificate_chain_file(certFilePath);
            ctx->use_private_key_file(keyFilePath, asio::ssl::context::pem);

            // Example method of generating this file:
            // `openssl dhparam -out dh.pem 2048`
            // Mozilla Intermediate suggests 1024 as the minimum size to use
            // Mozilla Modern suggests 2048 as the minimum size to use
            ctx->use_tmp_dh_file("./dh.pem");

            std::string ciphers;
            if(mode == MOZILLA_MODERN) {
                ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";
            } else {
                ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";
            }

            if(SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) != 1) {
                LOG_F(ERROR, "Error setting cipher list");
            }
        } catch(std::exception& e)
        {
            LOG_F(ERROR, "Exception: %s", e.what());
            LOG_F(FATAL, "Failed to establish TLS context");
            std::raise(SIGABRT);
        }

        return ctx;
    }

    void ConnectionManager::ListenAsync()
    {
        this->m_listeningThread = std::thread([&] {
            loguru::set_thread_name("Websocket");
            this->m_server.set_reuse_addr(true);
            this->m_server.set_message_handler(
                std::bind(
                    &ConnectionManager::OnMessage,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2));
            this->m_server.set_close_handler(
                std::bind(
                    &ConnectionManager::MarkConnectionExpired,
                    this,
                    std::placeholders::_1));
            this->m_server.set_open_handler(
                std::bind(
                    &ConnectionManager::AddConnection,
                    this,
                    std::placeholders::_1));
            this->m_server.set_tls_init_handler(
                std::bind(
                    &ConnectionManager::OnTLSConnect,
                    this,
                    TLS_MODE::MOZILLA_INTERMEDIATE,
                    std::placeholders::_1
            ));

            this->m_server.set_access_channels(websocketpp::log::alevel::none);
            this->m_server.set_error_channels(websocketpp::log::elevel::none);

            this->m_server.init_asio();
            this->m_server.listen(9002);
            this->m_server.start_accept();

            this->m_server.run();
        });
    }

    void ConnectionManager::StartSimAsync(
        std::atomic<bool>& isRunning,
        Simulation& simulation,
        float& timeStep)
    {
        this->m_simThread = std::thread([&isRunning, &simulation, &timeStep, this] {
            loguru::set_thread_name("Simulation");
            while (isRunning) {
                std::this_thread::sleep_for(std::chrono::milliseconds(this->kServerTickIntervalMilliSeconds));

                this->RemoveExpiredConnections();
                this->UpdateNewConections();

                this->HandleNetMessages(simulation, timeStep);

                if (!this->HasActiveClient()) {
                    continue;
                }

                // Run simulation time step
                if (simulation.IsRunningLive()) {
                    simulation.RunTimeStep(timeStep);
                }

                this->CheckForFinishedClients(simulation);
                this->SendDataToClients(simulation);
            }
        });
    }

    void ConnectionManager::StartHeartbeatAsync(std::atomic<bool>& isRunning)
    {
        this->m_heartbeatThread = std::thread([&isRunning, this] {
            loguru::set_thread_name("Heartbeat");
            while (isRunning) {
                std::this_thread::sleep_for(std::chrono::seconds(this->kHeartBeatIntervalSeconds));

                if (this->CheckNoClientTimeout()) {
                    isRunning = false;
                }

                if (this->NumberOfClients() > 0) {
                    this->RemoveUnresponsiveClients();
                    this->PingAllClients();
                }
            }
        });
    }

    void ConnectionManager::StartFileIOAsync(
      std::atomic<bool>& isRunning,
      Simulation& simulation)
    {
        this->m_fileIoThread = std::thread([&isRunning, &simulation, this] {
            loguru::set_thread_name("File IO");
            while (isRunning) {
                while(this->m_fileRequests.size()) {
                  auto request = this->m_fileRequests.front();
                  LOG_F(INFO, "Handling request for file %s", request.fileName.c_str());

                  this->m_fileMutex.lock();
                  std::string senderUid = request.senderUid;
                  std::string fileName = request.fileName;
                  int frameNumber = request.frameNumber;

                  // Check that the client is still connected/valid
                  if(!this->m_netStates.count(senderUid)) {
                    LOG_F(ERROR, "No net state for client %s", senderUid.c_str());
                    this->m_fileRequests.pop();
                    this->m_fileMutex.unlock();
                    continue;
                  }

                  auto state = this->m_netStates.at(senderUid);
                  std::string id = state.sim_identifier;

                  // Check that the requested file hasn't changed
                  if(id != fileName) {
                    LOG_F(WARNING,
                      "Client %s has selected file %s, ignoring previous request for file %s",
                       senderUid.c_str(), id.c_str(), fileName.c_str()
                     );
                     this->m_fileRequests.pop();
                     this->m_fileMutex.unlock();
                    continue;
                  }

                  this->InitializeTrajectoryFile(
                      simulation,
                      senderUid,
                      fileName
                  );

                  // Check that the client is still connected/valid
                  if(!this->m_netStates.count(senderUid)) {
                    LOG_F(ERROR, "No net state for client %s", senderUid.c_str());
                    this->m_fileRequests.pop();
                    this->m_fileMutex.unlock();
                    continue;
                  }

                  state = this->m_netStates.at(senderUid);
                  id = state.sim_identifier;

                  // Check again that the requested file hasn't changed
                  if(id != fileName) {
                    LOG_F(WARNING, "Client %s has selected a new file, ignoring previous request", senderUid.c_str());
                    this->m_fileRequests.pop();
                    this->m_fileMutex.unlock();
                    continue;
                  }

                  if(frameNumber > 0) {
                    this->SendSingleFrameToClient(
                        simulation,
                        senderUid,
                        frameNumber
                    );
                  }

                  this->m_fileRequests.pop();
                  this->m_fileMutex.unlock();
                }

                std::this_thread::sleep_for(std::chrono::milliseconds(this->kFileIoCheckIntervalMilliSeconds));
            }
        });
    }

    void ConnectionManager::AddConnection(websocketpp::connection_hdl hd1)
    {
        std::string newUid;
        this->GenerateLocalUUID(newUid);
        this->m_netStates[newUid] = NetState();
        this->m_missedHeartbeats[newUid] = 0;
        this->m_netConnections[newUid] = hd1;
        this->m_latestConnectionUid = newUid;
        this->m_hasNewConnection = true;
        this->LogClientEvent(newUid, "Incoming connection accepted");
        LOG_F(INFO, "%zu active websocket connections", this->m_netConnections.size());
    }

    void ConnectionManager::RemoveConnection(std::string connectionUID)
    {
        this->LogClientEvent(connectionUID, "Removing closed network connection");
        this->m_netConnections.erase(connectionUID);
        this->m_missedHeartbeats.erase(connectionUID);
        this->m_netStates.erase(connectionUID);
    }

    void ConnectionManager::CloseConnection(std::string connectionUID)
    {
        this->LogClientEvent(connectionUID, "Closing network connection");
        auto& conn = this->m_netConnections[connectionUID];
        this->m_server.pause_reading(conn);
        this->m_server.close(conn, 0, "");

        this->RemoveConnection(connectionUID);
    }

    void ConnectionManager::RemoveUnresponsiveClients()
    {
        std::vector<std::string> toRemove;
        for (auto& entry : this->m_netConnections) {
            auto& current_uid = entry.first;
            this->m_missedHeartbeats[current_uid]++;

            if (this->m_missedHeartbeats[current_uid] > this->kMaxMissedHeartBeats) {
                toRemove.push_back(current_uid);
            }
        }

        for (auto& uid : toRemove) {
            this->LogClientEvent(uid, "Removing unresponsive network connection");
            this->CloseConnection(uid);
        }
    }

    std::string ConnectionManager::GetUid(websocketpp::connection_hdl hd1)
    {
        //@TODO: Not every message needs the uid of the sender
        //	verify if this does not have a significant impact on perf
        for (auto& entry : m_netConnections) {
            auto& uid = entry.first;
            auto& conn = entry.second;

            if (equals<void, void>(conn, hd1)) {
                return uid;
            }
        }

        return "";
    }

    bool ConnectionManager::HasActiveClient()
    {
        for (auto& entry : this->m_netStates) {
            auto& netState = entry.second;
            if (
                netState.play_state == ClientPlayState::Playing ||
                netState.play_state == ClientPlayState::Waiting
            ) {
                return true;
            }
        }

        return false;
    }

    void ConnectionManager::SetClientState(
        std::string connectionUID, ClientPlayState state)
    {
        this->m_netStates[connectionUID].play_state = state;
    }

    void ConnectionManager::SetClientPos(
        std::string connectionUID, std::size_t pos)
    {
        this->m_netStates[connectionUID].playback_pos = pos;
    }

    void ConnectionManager::SetClientSimId(
        std::string connectionUID, std::string simId)
    {
        this->m_netStates[connectionUID].sim_identifier = simId;
    }

    void ConnectionManager::CheckForFinishedClient(
        Simulation& simulation,
        std::string connectionUID
    ) {
        if(!this->m_netStates.count(connectionUID)) {
            LOG_F(ERROR, "No net state for client %s", connectionUID.c_str());
            return;
        }

        auto& netState = this->m_netStates.at(connectionUID);
        auto sid = netState.sim_identifier;
        if(!simulation.HasFileInCache(sid)) { return; }

        auto totalNumberOfFrames =
            simulation.GetFileProperties(sid).numberOfFrames;
        auto numberOfLoadedFrames = simulation.GetNumFrames(sid);
        auto endPos = simulation.GetEndOfStreamPos(sid);

        bool isFileFinishedProcessing =
          (totalNumberOfFrames == numberOfLoadedFrames)
          && totalNumberOfFrames > 0;
        bool isClientAtEndOfStream =
          (netState.playback_pos >= endPos)
          && endPos > 0;

        auto currentState = netState.play_state;

        // Nothing to load yet
        if(totalNumberOfFrames == 0 || numberOfLoadedFrames == 0) {
            this->SetClientState(connectionUID, ClientPlayState::Waiting);
        }
        // Invalid frame, set to last frame
        else if(isClientAtEndOfStream && isFileFinishedProcessing)
        {
            if(netState.sim_identifier == LIVE_SIM_IDENTIFIER) {
                this->SetClientState(connectionUID, ClientPlayState::Waiting);
            } else {
                this->LogClientEvent(connectionUID, "Finished Streaming");
                this->SetClientPos(connectionUID, broadcast::eos);
                this->SetClientState(connectionUID, ClientPlayState::Finished);
            }
        }
        // Frame is valid but not loaded yet
        else if(currentState == ClientPlayState::Playing &&
            isClientAtEndOfStream &&
            !isFileFinishedProcessing)
        {
            this->SetClientState(connectionUID, ClientPlayState::Waiting);
        }
        // If the waited for frame has been loaded
        else if(!isClientAtEndOfStream && currentState == ClientPlayState::Waiting)
        {
            this->SetClientState(connectionUID, ClientPlayState::Playing);
        }
    }

    void ConnectionManager::CheckForFinishedClients(
        Simulation& simulation
    )
    {
        for (auto& entry : this->m_netStates) {
            auto& connectionUID = entry.first;
            auto& netState = entry.second;

            if(netState.play_state == ClientPlayState::Finished ||
                netState.play_state == ClientPlayState::Stopped)
            {
                continue;
            }

            this->CheckForFinishedClient(
                simulation,
                connectionUID
            );
        }
    }

    void ConnectionManager::MarkConnectionExpired(websocketpp::connection_hdl hd1)
    {
        for (auto& entry : this->m_netConnections) {
            auto& uid = entry.first;
            auto& conn = entry.second;

            if (conn.expired()
                || equals<void, void>(conn, hd1)) {
                this->m_uidsToDelete.push_back(uid);
            }
        }
    }

    void ConnectionManager::RemoveExpiredConnections()
    {
        for (auto& uid : this->m_uidsToDelete) {
            this->RemoveConnection(uid);
        }

        this->m_uidsToDelete.clear();
    }

    void ConnectionManager::SendArrayBufferMessage(
      std::string connectionUID, std::vector<float> buffer
    ) {
      if(!this->m_netConnections.count(connectionUID)) {
        LOG_F(ERROR, "Ignoring message send to invalid/untracked client %s", connectionUID.c_str());
        return;
      }

      if(buffer.size() == 0) {
        LOG_F(WARNING, "Ignoring empty arraybuffer message");
        return;
      }

      try {
          this->m_server.send(
              this->m_netConnections.at(connectionUID),
              buffer.data(),
              buffer.size() * sizeof(float),
              websocketpp::frame::opcode::binary
            );
      } catch (...) {
          this->LogClientEvent(connectionUID, "Failed to send websocket message to client");
      }
    }

    void ConnectionManager::SendWebsocketMessage(
        std::string connectionUID, Json::Value jsonMessage)
    {
        if(!this->m_netConnections.count(connectionUID)) {
          LOG_F(ERROR, "Ignoring message send to invalid/untracked client %s", connectionUID.c_str());
          return;
        }

        jsonMessage["connId"] = connectionUID;
        std::string message = Json::writeString(this->m_jsonStreamWriter, jsonMessage);

        try {
            this->m_server.send(
                this->m_netConnections.at(connectionUID),
                message, websocketpp::frame::opcode::text);
        } catch (...) {
            this->LogClientEvent(connectionUID, "Failed to send websocket message to client");
        }
    }

    void ConnectionManager::SendWebsocketMessageToAll(
        Json::Value jsonMessage, std::string description)
    {
        LOG_F(INFO, "Sending message to all clients: %s", description.c_str());
        for (auto& entry : this->m_netConnections) {
            auto uid = entry.first;
            SendWebsocketMessage(uid, jsonMessage);
        }
    }

    std::size_t ConnectionManager::NumberOfClients()
    {
        return this->m_netConnections.size();
    }

    void ConnectionManager::RegisterHeartBeat(std::string connectionUID)
    {
        this->LogClientEvent(connectionUID, "Registered client heartbeat");
        this->m_missedHeartbeats[connectionUID] = 0;
    }

    void ConnectionManager::SendSingleFrameToClient(
        Simulation& simulation,
        std::string connectionUID,
        std::size_t frameNumber
    ) {
      this->SendDataToClient(simulation, connectionUID, frameNumber, 1, true);
    }

    void ConnectionManager::SendDataToClients(Simulation& simulation)
    {
        for (auto& entry : this->m_netStates) {
            auto& uid = entry.first;
            auto& netState = entry.second;

            this->SendDataToClient(
                simulation,
                uid
            );
        }
    }

    bool ConnectionManager::CheckNoClientTimeout()
    {
        if (this->m_argNoTimeout) {
            return false;
        }

        if (this->NumberOfClients() == 0) {
            auto now = std::chrono::system_clock::now();
            auto diff = now - this->m_noClientTimer;
            auto& timeOut = this->kNoClientTimeoutSeconds;

            if (diff >= std::chrono::seconds(timeOut)) {
                LOG_F(INFO, "No clients connected for %zu seconds, server timeout ...", timeOut);
                return true;
            }
        } else {
            this->m_noClientTimer = std::chrono::system_clock::now();
        }

        return false;
    }

    void ConnectionManager::PingAllClients()
    {
        Json::Value pingJsonMessage;
        pingJsonMessage["msgType"] = id_heartbeat_ping;
        this->SendWebsocketMessageToAll(pingJsonMessage, "Heartbeat ping");
    }

    void ConnectionManager::BroadcastParameterUpdate(Json::Value updateMessage)
    {
        this->m_paramCache.push_back(updateMessage);
        this->SendWebsocketMessageToAll(updateMessage, "rate-parameter update");
    }

    void ConnectionManager::BroadcastModelDefinition(Json::Value modelDefinition)
    {
        this->m_hasModel = true;
        this->m_mostRecentModel = modelDefinition;
        this->m_paramCache.clear();
        this->SendWebsocketMessageToAll(modelDefinition, "model definition");
    }

    void ConnectionManager::UpdateNewConections()
    {
        if (this->m_hasNewConnection && this->m_hasModel) {
            this->SendWebsocketMessage(this->m_latestConnectionUid, this->m_mostRecentModel);
            this->m_hasNewConnection = false;

            for (auto& update : this->m_paramCache) {
                this->SendWebsocketMessage(this->m_latestConnectionUid, update);
            }
        }
    }

    void ConnectionManager::GenerateLocalUUID(std::string& uuid)
    {
        // Doesn't matter if time is seeded or not at this point
        //  just need relativley unique local identifiers for runtime
        char strUuid[128];
        sprintf(strUuid, "%x%x-%x-%x-%x-%x%x%x",
            rand(), rand(), // Generates a 64-bit Hex number
            rand(), // Generates a 32-bit Hex number
            ((rand() & 0x0fff) | 0x4000), // Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
            rand() % 0x3fff + 0x8000, // Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
            rand(), rand(), rand()); // Generates a 96-bit Hex number

        uuid = strUuid;
    }

    void ConnectionManager::SendDataToClient(
        Simulation& simulation,
        std::string connectionUID
    ) {
        if(!this->m_netStates.count(connectionUID)) {
            LOG_F(ERROR, "No net state for client %s", connectionUID.c_str());
            return;
        }

        auto& netState = this->m_netStates.at(connectionUID);
        std::string sid = netState.sim_identifier;
        auto totalNumberOfFrames = simulation.GetNumFrames(sid);

        if(totalNumberOfFrames == 0) {
          return; // no data to send
        }

        if (netState.play_state != ClientPlayState::Playing) {
            return;
        }

        auto update = simulation.GetBroadcastUpdate(
          sid,
          netState.playback_pos,
          this->kBroadcastBufferSize
        );

        netState.playback_pos = update.new_pos;
        this->SendArrayBufferMessage(connectionUID, update.buffer);
    }

    void ConnectionManager::SendSingleFrameToClient(
        Simulation& simulation,
        std::string connectionUID,
        std::size_t frameNumber
    ) {
        if(!this->m_netStates.count(connectionUID)) {
            LOG_F(ERROR, "No net state for client %s", connectionUID.c_str());
            return;
        }

        auto& netState = this->m_netStates.at(connectionUID);
        std::string sid = netState.sim_identifier;
        auto totalNumberOfFrames = simulation.GetNumFrames(sid);

        if(totalNumberOfFrames == 0) {
          return; // no data to send
        }

        auto update = simulation.GetBroadcastFrame(
          sid, frameNumber
        );

        netState.playback_pos = update.new_pos;
        this->SendArrayBufferMessage(connectionUID, update.buffer);
    }

    void ConnectionManager::HandleMessage(NetMessage nm)
    {
        auto msgType = nm.jsonMessage["msgType"].asInt();

        if (msgType >= 0 && msgType < WebRequestNames.size()) {
            this->LogClientEvent(nm.senderUid, "Web socket message arrived: " + WebRequestNames[msgType]);
            if (msgType == WebRequestTypes::id_heartbeat_pong) {
                this->RegisterHeartBeat(nm.senderUid);
            } else {
                this->m_simThreadMessages.push_back(nm);
            }
        } else {
            LOG_F(WARNING,"Websocket message arrived: UNRECOGNIZED of type %i", msgType);
        }
    }

    void ConnectionManager::OnMessage(websocketpp::connection_hdl hd1, server::message_ptr msg)
    {
        Json::CharReaderBuilder jsonReadBuilder;
        std::unique_ptr<Json::CharReader> const jsonReader(jsonReadBuilder.newCharReader());

        NetMessage nm;
        nm.senderUid = this->GetUid(hd1);
        std::string message = msg->get_payload();
        std::string errs;

        jsonReader->parse(message.c_str(), message.c_str() + message.length(),
            &(nm.jsonMessage), &errs);

        this->HandleMessage(nm);
    }

    void ConnectionManager::HandleNetMessages(
        Simulation& simulation,
        float& timeStep)
    {
        auto& messages = this->GetMessages();

        // handle net messages
        if (messages.size() > 0) {
            for (std::size_t i = 0; i < messages.size(); ++i) {
                std::string senderUid = messages[i].senderUid;
                Json::Value jsonMsg = messages[i].jsonMessage;

                int msgType = jsonMsg["msgType"].asInt();
                switch (msgType) {
                case WebRequestTypes::id_vis_data_request: {
                    auto runMode = static_cast<SimulationMode>(jsonMsg["mode"].asInt());

                    // If a simulation is already in progress, don't allow a new client to
                    //	change the simulation, unless there is only one client connected
                    //  update: trajectory file streaming can be multi-file
                    if (!this->HasActiveClient()
                        || this->NumberOfClients() == 1
                        || runMode == SimulationMode::id_traj_file_playback
                    ) {
                        switch (runMode) {
                        case SimulationMode::id_live_simulation: {
                            this->LogClientEvent(senderUid, "Running Live Simulation");
                            this->SetClientSimId(senderUid, LIVE_SIM_IDENTIFIER);
                            simulation.SetPlaybackMode(runMode);
                            simulation.SetSimId(LIVE_SIM_IDENTIFIER);
                            simulation.Reset();
                        } break;
                        case SimulationMode::id_pre_run_simulation: {
                            timeStep = jsonMsg["timeStep"].asFloat();
                            auto numberOfTimeSteps = jsonMsg["numTimeSteps"].asInt();
                            this->LogClientEvent(senderUid, "Running pre-run simulation");

                            simulation.SetPlaybackMode(runMode);
                            simulation.Reset();
                            simulation.RunAndSaveFrames(timeStep, numberOfTimeSteps);

                            TrajectoryFileProperties tfp;
                            tfp.numberOfFrames = numberOfTimeSteps;
                            tfp.timeStepSize = timeStep;
                            simulation.SetFileProperties("prerun", tfp);
                            this->SetClientSimId(senderUid, "prerun");
                            simulation.SetSimId("prerun");
                            this->SetupRuntimeCache(simulation);
                        } break;
                        case SimulationMode::id_traj_file_playback: {
                            simulation.SetPlaybackMode(runMode);

                            if(!jsonMsg.isMember("file-name")) {
                              this->SetClientState(senderUid, ClientPlayState::Paused);
                              this->SetClientPos(senderUid, 0);
                              continue;
                            }
                            auto trajectoryFileName = jsonMsg["file-name"].asString();
                            if(trajectoryFileName.empty()) {
                              LOG_F(INFO, "Ignoring request with empty file-name");
                              continue;
                            }

                            auto trajectoryFileName = jsonMsg["file-name"].asString();
                            this->LogClientEvent(senderUid, "Playing back trajectory file: " + trajectoryFileName);
                            this->SetClientSimId(senderUid, trajectoryFileName);

                            FileRequest request;
                            request.senderUid = senderUid;
                            request.fileName = trajectoryFileName;

                            if(jsonMsg.isMember("frameNumber")) {
                              int frameNumber = jsonMsg["frameNumber"].asInt();
                              request.frameNumber = std::max(frameNumber,0);
                              this->SetClientState(senderUid, ClientPlayState::Paused);
                            } else {
                              this->SetClientState(senderUid, ClientPlayState::Playing);
                            }

                            this->m_fileRequests.push(request);
                        } break;
                        }
                    } else {
                        LOG_F(WARNING, "Ignoring request to change server mode; other clients are listening");
                    }
                } break;
                case WebRequestTypes::id_vis_data_pause: {
                    this->SetClientState(senderUid, ClientPlayState::Paused);
                } break;
                case WebRequestTypes::id_vis_data_resume: {
                    this->SetClientState(senderUid, ClientPlayState::Playing);
                } break;
                case WebRequestTypes::id_vis_data_abort: {
                    this->SetClientState(senderUid, ClientPlayState::Stopped);
                } break;
                case WebRequestTypes::id_update_time_step: {
                    timeStep = jsonMsg["timeStep"].asFloat();

                    this->LogClientEvent(
                        senderUid,
                        "Time step updated to " + std::to_string(timeStep)
                    );
                    this->SendWebsocketMessageToAll(jsonMsg, "time step update");
                } break;
                case WebRequestTypes::id_update_rate_param: {
                    std::string paramName = jsonMsg["paramName"].asString();
                    float paramValue = jsonMsg["paramValue"].asFloat();

                    this->LogClientEvent(
                        senderUid,
                        "Rate Parameter " + paramName +
                        " updated to " + std::to_string(paramValue)
                    );
                    simulation.UpdateParameter(paramName, paramValue);
                    this->BroadcastParameterUpdate(jsonMsg);
                } break;
                case WebRequestTypes::id_model_definition: {
                    this->LogClientEvent(senderUid, "New Model Arrived");
                    aics::simularium::Model sim_model;
                    parse_model(jsonMsg, sim_model);
                    print_model(sim_model);
                    simulation.SetModel(sim_model);

                    timeStep = sim_model.max_time_step;
                    this->LogClientEvent(
                        senderUid,
                        "Set Timestep to " + std::to_string(timeStep)
                    );

                    this->BroadcastModelDefinition(jsonMsg);
                } break;
                case WebRequestTypes::id_goto_simulation_time: {
                    auto& netState = this->m_netStates[senderUid];
                    double timeNs = std::stod(jsonMsg["time"].asString());
                    std::size_t frameNumber = simulation.GetClosestFrameNumberForTime(
                        netState.sim_identifier, timeNs
                    );
                    double closestTime = simulation.GetSimulationTimeAtFrame(
                        netState.sim_identifier, frameNumber
                    );

                    this->LogClientEvent(senderUid,
                        "Request for time " + std::to_string(timeNs) + " -> selected frame " +
                        std::to_string(frameNumber) + " with time " + std::to_string(closestTime)
                    );

                    this->SendSingleFrameToClient(simulation, senderUid, frameNumber);
                } break;
                case WebRequestTypes::id_init_trajectory_file: {
                    std::string trajectoryFileName = jsonMsg["fileName"].asString();
                    simulation.SetPlaybackMode(SimulationMode::id_traj_file_playback);

                    FileRequest request;
                    request.senderUid = senderUid;
                    request.fileName = trajectoryFileName;
                    request.frameNumber = -1;
                    this->m_fileRequests.push(request);
                } break;
                default: {
                } break;
                }
            }

            messages.clear();
        }
    }

    void ConnectionManager::InitializeTrajectoryFile(
        Simulation& simulation,
        std::string connectionUID,
        std::string fileName
    )
    {
        if (fileName.empty()) {
            this->LogClientEvent(connectionUID, "Trajectory file not specified, ignoring request");
            return;
        }

        if(!this->m_netStates.count(connectionUID)) {
          LOG_F(ERROR, "No net state for client %s", connectionUID.c_str());
          return;
        }

        auto state = this->m_netStates.at(connectionUID);
        std::string currentFile = state.sim_identifier;
        if(currentFile != fileName) {
          LOG_F(WARNING, "Client has changed trajectory files since this request was made");
          return;
        }

        this->SetClientFrame(connectionUID, 0);
        if(simulation.HasFileInCache(fileName))
        {
            LOG_F(INFO,"[%s] Using previously loaded file for trajectory", fileName.c_str());
            this->SendSingleFrameToClient(simulation, connectionUID, 0);
        }
        else {
            bool isSimulariumFile = fileName.substr(fileName.find_last_of(".") + 1) == "simularium";
            if(isSimulariumFile) {
              LOG_F(INFO, "File %s has a simularium file extension", fileName.c_str());
            }

            // Attempt to download an already processed runtime cache
            if(!this->m_argForceInit // this will force the server to re-download/process a trajectory
                && simulation.DownloadRuntimeCache(fileName))
            {
                simulation.PreprocessRuntimeCache(fileName);
                this->SendSingleFrameToClient(simulation, connectionUID, 0);
            } else if(simulation.FindSimulariumFile(fileName)) { // find .simularium file instead
                simulation.PreprocessRuntimeCache(fileName);
                this->SendSingleFrameToClient(simulation, connectionUID, 0);
                if(!this->m_argNoUpload) {
                  simulation.UploadRuntimeCache(fileName);
                }
            } else {
                // Reprocess a raw trajectory (if found)
                simulation.SetPlaybackMode(id_traj_file_playback);
                simulation.Reset();
                if(simulation.LoadTrajectoryFile(fileName))
                {
                    this->SetupRuntimeCache(simulation);
                    this->SendSingleFrameToClient(simulation, connectionUID, 0);
                }
            }
        }

        // Need to call this after the binary cache is processed
        //  in 'SetupRuntimeCache' above
        this->SetClientPos(connectionUID, simulation.GetFramePos(fileName, 0));

        // Send Trajectory File Properties
        TrajectoryFileProperties tfp = simulation.GetFileProperties(fileName);
        LOG_F(INFO, "%s", tfp.Str().c_str());

        Json::Value fprops;
        fprops["version"] = 1;
        fprops["msgType"] = WebRequestTypes::id_trajectory_file_info;
        fprops["totalSteps"] = tfp.numberOfFrames;
        fprops["timeStepSize"] = tfp.timeStepSize;
        fprops["spatialUnitFactorMeters"] = tfp.spatialUnitFactorMeters;

        Json::Value typeMapping;
        for(auto entry : tfp.typeMapping)
        {
            std::string id = std::to_string(entry.first);
            std::string name = entry.second;

            Json::Value typeEntry;
            typeEntry["name"] = name;

            typeMapping[id] = typeEntry;
        }

        Json::Value size;
        size["x"] = tfp.boxX;
        size["y"] = tfp.boxY;
        size["z"] = tfp.boxZ;

        fprops["typeMapping"] = typeMapping;
        fprops["size"] = size;

        this->SendWebsocketMessage(connectionUID, fprops);
    }

    void ConnectionManager::SetupRuntimeCache(
        Simulation& simulation
    ) {
        std::string fileName = simulation.GetSimId();

        LOG_F(INFO,"[%s] Loading trajectory file into runtime cache", fileName.c_str());
        std::size_t fn = 0;

        while (!simulation.HasLoadedAllFrames()) {
            simulation.LoadNextFrame();
        }
        LOG_F(INFO, "[%s] Finished loading trajectory into runtime cache", fileName.c_str());

        // Save the result so it doesn't need to be calculated again
        if(simulation.IsPlayingTrajectory() && !(this->m_argNoUpload))
        {
            simulation.UploadRuntimeCache(fileName);
        }

        simulation.CleanupTmpFiles(fileName);
    }

} // namespace simularium
} // namespace aics
