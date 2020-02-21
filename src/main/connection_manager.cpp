#include "agentsim/network/connection_manager.h"
#include "agentsim/network/net_message_ids.h"
#include "agentsim/network/trajectory_properties.h"
#include "agentsim/aws/aws_util.h"
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
namespace agentsim {

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
                this->AdvanceClients();
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

    void ConnectionManager::SetClientFrame(
        std::string connectionUID, std::size_t frameNumber)
    {
        this->m_netStates[connectionUID].frame_no = frameNumber;
    }

    void ConnectionManager::SetClientSimId(
        std::string connectionUID, std::string simId)
    {
        this->m_netStates[connectionUID].sim_identifier = simId;
    }

    void ConnectionManager::CheckForFinishedClient(
        Simulation& simulation,
        std::string connectionUID
    )
    {
        auto& netState = this->m_netStates.at(connectionUID);
        auto sid = netState.sim_identifier;
        auto totalNumberOfFrames =
            simulation.GetFileProperties(sid).numberOfFrames;
        auto numberOfLoadedFrames = simulation.GetNumFrames(sid);

        auto currentFrame = netState.frame_no;
        auto currentState = netState.play_state;

        // Invalid frame, set to last frame
        if(currentFrame >= totalNumberOfFrames)
        {
            if(netState.sim_identifier == LIVE_SIM_IDENTIFIER) {
                this->SetClientState(connectionUID, ClientPlayState::Waiting);
            } else {
                this->LogClientEvent(connectionUID, "Finished Streaming");
                this->SetClientFrame(connectionUID, totalNumberOfFrames - 1);
                this->SetClientState(connectionUID, ClientPlayState::Finished);
            }
        }

        // Frame is valid but not loaded yet
        if(currentState == ClientPlayState::Playing &&
            currentFrame >= numberOfLoadedFrames &&
            currentFrame < totalNumberOfFrames)
        {
            this->LogClientEvent(connectionUID, "Waiting for frame " + std::to_string(currentFrame));
            this->SetClientState(connectionUID, ClientPlayState::Waiting);
        }

        // If the waited for frame has been loaded
        if(currentFrame < numberOfLoadedFrames && currentState == ClientPlayState::Waiting)
        {
            if(netState.sim_identifier != LIVE_SIM_IDENTIFIER) {
                this->LogClientEvent(connectionUID, "Done waiting for frame " + std::to_string(currentFrame));
            }
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

    void ConnectionManager::SendWebsocketMessage(
        std::string connectionUID, Json::Value jsonMessage)
    {
        jsonMessage["connId"] = connectionUID;
        std::string message = Json::writeString(this->m_jsonStreamWriter, jsonMessage);

        try {
            this->m_server.send(
                this->m_netConnections[connectionUID],
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

    void ConnectionManager::AdvanceClients()
    {
        for (auto& entry : this->m_netStates) {
            auto& netState = entry.second;

            if (netState.play_state != ClientPlayState::Playing) {
                continue;
            }

            netState.frame_no++;
        }
    }

    void ConnectionManager::SendDataToClients(Simulation& simulation)
    {
        for (auto& entry : this->m_netStates) {
            auto& uid = entry.first;
            auto& netState = entry.second;

            this->SendDataToClient(
                simulation,
                uid,
                netState.frame_no,
                this->kNumberOfFramesToBulkBroadcast
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
        std::string connectionUID,
        std::size_t startingFrame,
        std::size_t numberOfFrames,
        bool force // set to true for one-off sends
    )
    {
        auto& netState = this->m_netStates.at(connectionUID);
        std::string sid = netState.sim_identifier;
        auto totalNumberOfFrames = simulation.GetNumFrames(sid);

        if(!force)
        {
            if (netState.play_state != ClientPlayState::Playing) {
                return;
            }
        }

        Json::Value dataFrameMessage;
        dataFrameMessage["msgType"] = id_vis_data_arrive;
        dataFrameMessage["bundleSize"] = (int)numberOfFrames;
        dataFrameMessage["bundleStart"] = (int)startingFrame;
        Json::Value frameArr = Json::Value(Json::arrayValue);
        int endFrame = std::min(startingFrame + numberOfFrames, totalNumberOfFrames);
        int currentFrame = startingFrame;

        for(; currentFrame < endFrame; currentFrame++)
        {
            AgentDataFrame adf = simulation.GetDataFrame(sid, currentFrame);
            Json::Value frameJSON;
            frameJSON["data"] = Serialize(adf);
            frameJSON["frameNumber"] = currentFrame;
            frameJSON["time"] = simulation.GetSimulationTimeAtFrame(sid, currentFrame);

            frameArr.append(frameJSON);
        }

        netState.frame_no = currentFrame - 1;
        dataFrameMessage["bundleData"] = frameArr;
        this->SendWebsocketMessage(connectionUID, dataFrameMessage);
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
                            this->SetupRuntimeCacheAsync(simulation, 500);
                        } break;
                        case SimulationMode::id_traj_file_playback: {
                            simulation.SetPlaybackMode(runMode);
                            auto trajectoryFileName = jsonMsg["file-name"].asString();
                            this->LogClientEvent(senderUid, "Playing back trajectory file: " + trajectoryFileName);
                            this->InitializeTrajectoryFile(simulation, senderUid, trajectoryFileName);
                        } break;
                        }
                    } else {
                        LOG_F(WARNING, "Ignoring request to change server mode; other clients are listening");
                    }

                    if(jsonMsg.isMember("frameNumber"))
                    {
                        int frameNumber = jsonMsg["frameNumber"].asInt();
                        frameNumber = std::max(frameNumber, 0);

                        this->LogClientEvent(
                            senderUid,
                            "Data request for frame " + std::to_string(frameNumber)
                        );
                        this->SendSingleFrameToClient(
                            simulation,
                            senderUid,
                            frameNumber
                        );

                        this->SetClientState(senderUid, ClientPlayState::Stopped);
                    }
                    else {
                        this->SetClientState(senderUid, ClientPlayState::Playing);
                        this->SetClientFrame(senderUid, 0);
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
                    aics::agentsim::Model sim_model;
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
                case WebRequestTypes::id_play_cache: {
                    if(jsonMsg.isMember("time"))
                    {
                        auto& netState = this->m_netStates[senderUid];
                        double timeNs = jsonMsg["time"].asDouble();
                        std::size_t frameNumber = simulation.GetClosestFrameNumberForTime(
                            netState.sim_identifier, timeNs
                        );
                        double closestTime = simulation.GetSimulationTimeAtFrame(
                            netState.sim_identifier, frameNumber
                        );

                        std::string description =
                            "Request to play cached from time " + std::to_string(timeNs) +
                            " (frame " + std::to_string(frameNumber) + " with time " +
                            std::to_string(closestTime) + ")";
                        this->LogClientEvent(senderUid, description);

                        this->SetClientFrame(senderUid, frameNumber);
                        this->SetClientState(senderUid, ClientPlayState::Playing);
                    }
                    else if (jsonMsg.isMember("frame-num"))
                    {
                        auto frameNumber = jsonMsg["frame-num"].asInt();
                        this->LogClientEvent(
                            senderUid,
                            "Request to play cached from frame " + std::to_string(frameNumber)
                        );
                        this->SetClientFrame(senderUid, frameNumber);
                        this->SetClientState(senderUid, ClientPlayState::Playing);
                    }

                } break;
                case WebRequestTypes::id_goto_simulation_time: {
                    auto& netState = this->m_netStates[senderUid];
                    double timeNs = jsonMsg["time"].asDouble();
                    std::size_t frameNumber = simulation.GetClosestFrameNumberForTime(
                        netState.sim_identifier, timeNs
                    );
                    double closestTime = simulation.GetSimulationTimeAtFrame(
                        netState.sim_identifier, frameNumber
                    );

                    this->LogClientEvent(senderUid, "Set to frame " + std::to_string(frameNumber));
                    this->SetClientFrame(senderUid, frameNumber);
                    this->SendSingleFrameToClient(simulation, senderUid, frameNumber);
                } break;
                case WebRequestTypes::id_init_trajectory_file: {
                    std::string trajectoryFileName = jsonMsg["fileName"].asString();
                    this->InitializeTrajectoryFile(simulation, senderUid, trajectoryFileName);
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
        this->m_fileMutex.lock();
        LOG_F(INFO,"[%s] File mutex locked", fileName.c_str());

        if (fileName.empty()) {
            this->LogClientEvent(connectionUID, "Trajectory file not specified, ignoring request");
            this->m_fileMutex.unlock();
            LOG_F(INFO,"[%s] File mutex unlocked", fileName.c_str());
            return;
        }

        this->LogClientEvent(connectionUID, "Set to file " + fileName);
        this->SetClientSimId(connectionUID, fileName);
        this->SetClientFrame(connectionUID, 0);

        if(simulation.HasFileInCache(fileName))
        {
            LOG_F(INFO,"[%s] Using previously loaded file for trajectory", fileName.c_str());
            this->SendSingleFrameToClient(simulation, connectionUID, 0);
        }
        else {
            // Attempt to download an already processed runtime cache
            if(!this->m_argForceInit // this will force the server to re-download/process a trajectory
                && simulation.DownloadRuntimeCache(fileName))
            {
                simulation.PreprocessRuntimeCache(fileName);
                this->SendSingleFrameToClient(simulation, connectionUID, 0);
            }
            else {
                simulation.SetPlaybackMode(id_traj_file_playback);
                simulation.Reset();
                if(simulation.LoadTrajectoryFile(fileName))
                {
                    this->m_fileMutex.unlock();
                    LOG_F(INFO,"[%s] File mutex unlocked", fileName.c_str());

                    this->SetupRuntimeCacheAsync(simulation, 500);
                    this->SendSingleFrameToClient(simulation, connectionUID, 0);

                    this->m_fileMutex.lock();
                    LOG_F(INFO,"[%s] File mutex locked", fileName.c_str());
                }
            }
        }

        // Send Trajectory File Properties
        TrajectoryFileProperties tfp = simulation.GetFileProperties(fileName);
        LOG_F(INFO, "%s", tfp.Str().c_str());

        Json::Value fprops;
        fprops["msgType"] = WebRequestTypes::id_trajectory_file_info;
        fprops["totalDuration"] = (tfp.numberOfFrames - 1) * tfp.timeStepSize;
        fprops["timeStepSize"] = tfp.timeStepSize;

        Json::Value typeMapping;
        for(auto entry : tfp.typeMapping)
        {
            std::string id = std::to_string(entry.first);
            std::string name = entry.second;

            typeMapping[id] = name;
        }
        fprops["typeMapping"] = typeMapping;
        fprops["boxSizeX"] = tfp.boxX;
        fprops["boxSizeY"] = tfp.boxY;
        fprops["boxSizeZ"] = tfp.boxZ;

        this->SendWebsocketMessage(connectionUID, fprops);
        this->m_fileMutex.unlock();
        LOG_F(INFO,"[%s] File mutex unlocked", fileName.c_str());
    }

    void ConnectionManager::SetupRuntimeCacheAsync(
        Simulation& simulation,
        std::size_t waitTimeMs
    ) {
        if (this->m_fileIoThread.joinable()) {
            this->m_fileIoThread.join();
        }

        this->m_fileIoThread = std::thread([&simulation, this] {
            loguru::set_thread_name("File IO");
            std::string fileName = simulation.GetSimId();

            this->m_fileMutex.lock();
            LOG_F(INFO,"[%s] File mutex locked", fileName.c_str());

            LOG_F(INFO,"[%s] Loading trajectory file into runtime cache", fileName.c_str());
            std::size_t fn = 0;

            while (!simulation.HasLoadedAllFrames()) {
                simulation.LoadNextFrame();
            }
            LOG_F(INFO, "[%s] Finished loading trajectory into runtime cache", fileName.c_str());
            this->m_fileMutex.unlock();
            LOG_F(INFO,"[%s] File mutex unlocked", fileName.c_str());

            // Save the result so it doesn't need to be calculated again
            if(simulation.IsPlayingTrajectory() && !(this->m_argNoUpload))
            {
                simulation.UploadRuntimeCache();
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeMs));
    }

} // namespace agentsim
} // namespace aics
