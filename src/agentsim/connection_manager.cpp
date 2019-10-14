#include "agentsim/network/connection_manager.h"
#include "agentsim/network/net_message_ids.h"
#include "agentsim/network/trajectory_properties.h"

template <typename T, typename U>
inline bool equals(const std::weak_ptr<T>& t, const std::weak_ptr<U>& u)
{
    return !t.owner_before(u) && !u.owner_before(t);
}

namespace aics {
namespace agentsim {

    ConnectionManager::ConnectionManager()
    {
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
            ctx->use_certificate_chain_file(certFilePath);
            ctx->use_private_key_file(certFilePath, asio::ssl::context::pem);

            // Example method of generating this file:
            // `openssl dhparam -out dh.pem 2048`
            // Mozilla Intermediate suggests 1024 as the minimum size to use
            // Mozilla Modern suggests 2048 as the minimum size to use
            ctx->use_tmp_dh_file("dh.pem");

            std::string ciphers;
            if(mode == MOZILLA_MODERN) {
                ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!3DES:!MD5:!PSK";
            } else {
                ciphers = "ECDHE-RSA-AES128-GCM-SHA256:ECDHE-ECDSA-AES128-GCM-SHA256:ECDHE-RSA-AES256-GCM-SHA384:ECDHE-ECDSA-AES256-GCM-SHA384:DHE-RSA-AES128-GCM-SHA256:DHE-DSS-AES128-GCM-SHA256:kEDH+AESGCM:ECDHE-RSA-AES128-SHA256:ECDHE-ECDSA-AES128-SHA256:ECDHE-RSA-AES128-SHA:ECDHE-ECDSA-AES128-SHA:ECDHE-RSA-AES256-SHA384:ECDHE-ECDSA-AES256-SHA384:ECDHE-RSA-AES256-SHA:ECDHE-ECDSA-AES256-SHA:DHE-RSA-AES128-SHA256:DHE-RSA-AES128-SHA:DHE-DSS-AES128-SHA256:DHE-RSA-AES256-SHA256:DHE-DSS-AES256-SHA:DHE-RSA-AES256-SHA:AES128-GCM-SHA256:AES256-GCM-SHA384:AES128-SHA256:AES256-SHA256:AES128-SHA:AES256-SHA:AES:CAMELLIA:DES-CBC3-SHA:!aNULL:!eNULL:!EXPORT:!DES:!RC4:!MD5:!PSK:!aECDH:!EDH-DSS-DES-CBC3-SHA:!EDH-RSA-DES-CBC3-SHA:!KRB5-DES-CBC3-SHA";
            }

            if(SSL_CTX_set_cipher_list(ctx->native_handle(), ciphers.c_str()) != 1) {
                std::cout << "Error setting cipher list" << std::endl;
            }
        } catch(std::exception& e)
        {
            std::cout << "Exception: " << e.what() << std::endl;
        }

        return ctx;
    }

    void ConnectionManager::ListenAsync()
    {
        this->m_listeningThread = std::thread([&] {
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

                std::size_t numberOfLoadedFrames = simulation.GetNumFrames();
                std::size_t totalNumberOfFrames = this->m_trajectoryFileProperties.numberOfFrames;

                this->CheckForFinishedClients(numberOfLoadedFrames, totalNumberOfFrames);
                this->SendDataToClients(simulation);
                this->AdvanceClients();
            }
        });
    }

    void ConnectionManager::StartHeartbeatAsync(std::atomic<bool>& isRunning)
    {
        this->m_heartbeatThread = std::thread([&isRunning, this] {
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
        std::cout << "Incoming connection accepted: " << newUid << std::endl;
    }

    void ConnectionManager::RemoveConnection(std::string connectionUID)
    {
        std::cout << "Removing closed network connection.\n";
        this->m_netConnections.erase(connectionUID);
        this->m_missedHeartbeats.erase(connectionUID);
        this->m_netStates.erase(connectionUID);
    }

    void ConnectionManager::CloseConnection(std::string connectionUID)
    {
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
            std::cout << "Removing unresponsive network connection.\n";
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

    void ConnectionManager::CheckForFinishedClient(
        std::size_t numberOfLoadedFrames,
        std::size_t totalNumberOfFrames,
        std::string connectionUID,
        NetState& netState)
    {
        auto currentFrame = netState.frame_no;
        auto currentState = netState.play_state;

        // Invalid frame, set to last frame
        if(currentFrame >= totalNumberOfFrames)
        {
            std::cout << "Client " << connectionUID
                << " finished streaming" << std::endl;
            this->SetClientFrame(connectionUID, totalNumberOfFrames - 1);
            this->SetClientState(connectionUID, ClientPlayState::Finished);
        }

        // Frame is valid but not loaded yet
        if(currentState == ClientPlayState::Playing &&
            currentFrame >= numberOfLoadedFrames &&
            currentFrame < totalNumberOfFrames)
        {
            std::cout << "Client " << connectionUID
                << " set to wait for frame " << currentFrame << std::endl;
            this->SetClientState(connectionUID, ClientPlayState::Waiting);
        }

        // If the waited for frame has been loaded
        if(currentFrame < numberOfLoadedFrames && currentState == ClientPlayState::Waiting)
        {
            std::cout << "Client " << connectionUID
                << " done waiting for frame " << currentFrame << std::endl;
            this->SetClientState(connectionUID, ClientPlayState::Playing);
        }
    }

    void ConnectionManager::CheckForFinishedClients(
        std::size_t numberOfLoadedFrames,
        std::size_t totalNumberOfFrames
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
                numberOfLoadedFrames,
                totalNumberOfFrames,
                connectionUID,
                netState
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
            std::cout << "Ignoring failed websocket send to " << connectionUID << std::endl;
        }
    }

    void ConnectionManager::SendWebsocketMessageToAll(
        Json::Value jsonMessage, std::string description)
    {
        std::cout << "Sending message to all clients: " << description << std::endl;
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

            this->SendDataToClient(simulation, uid, netState.frame_no);
        }
    }

    void ConnectionManager::SetNoTimeoutArg(bool val)
    {
        this->m_argNoTimeout = val;
    }

    bool ConnectionManager::CheckNoClientTimeout()
    {
        if (this->m_argNoTimeout) {
            return false;
        }

        if (this->NumberOfClients() == 0) {
            auto now = std::chrono::system_clock::now();
            auto diff = now - this->m_noClientTimer;

            if (diff >= std::chrono::seconds(this->kNoClientTimeoutSeconds)) {
                std::cout << "No clients connected for " << this->kNoClientTimeoutSeconds
                          << " seconds, server timeout ... " << std::endl;
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
        std::size_t frameNumber,
        bool force // set to true for one-off sends
    )
    {
        auto numberOfFrames = this->m_trajectoryFileProperties.numberOfFrames;
        auto& netState = this->m_netStates.at(connectionUID);

        if(!force)
        {
            if (netState.play_state != ClientPlayState::Playing) {
                return;
            }
        }

        AgentDataFrame simData;
        netState.frame_no = std::min(frameNumber, numberOfFrames - 1);
        simData = simulation.GetDataFrame(netState.frame_no);

        Json::Value net_agent_data_frame;
        std::vector<float> vals;

        net_agent_data_frame["msgType"] = id_vis_data_arrive;
        for (std::size_t i = 0; i < simData.size(); ++i) {
            auto agentData = simData[i];
            vals.push_back(agentData.vis_type);
            vals.push_back(agentData.type);
            vals.push_back(agentData.x);
            vals.push_back(agentData.y);
            vals.push_back(agentData.z);
            vals.push_back(agentData.xrot);
            vals.push_back(agentData.yrot);
            vals.push_back(agentData.zrot);
            vals.push_back(agentData.collision_radius);
            vals.push_back(agentData.subpoints.size());

            for (std::size_t j = 0; j < agentData.subpoints.size(); ++j) {
                vals.push_back(agentData.subpoints[j]);
            }
        }

        // Copy values to json data array
        auto json_data_arr = Json::Value(Json::arrayValue);
        for (std::size_t j = 0; j < vals.size(); ++j) {
            int nd_index = static_cast<int>(j);
            json_data_arr[nd_index] = vals[nd_index];
        }

        net_agent_data_frame["data"] = json_data_arr;
        net_agent_data_frame["frameNumber"] = static_cast<int>(netState.frame_no);
        net_agent_data_frame["time"] = simulation.GetSimulationTimeAtFrame(netState.frame_no);
        this->SendWebsocketMessage(connectionUID, net_agent_data_frame);
    }

    void ConnectionManager::HandleMessage(NetMessage nm)
    {
        auto msgType = nm.jsonMessage["msgType"].asInt();

        if (msgType >= 0 && msgType < WebRequestNames.size()) {
            std::cout << "[" << nm.senderUid << "] Web socket message arrived: "
                      << WebRequestNames[msgType] << std::endl;

            if (msgType == WebRequestTypes::id_heartbeat_pong) {
                this->RegisterHeartBeat(nm.senderUid);
            } else {
                this->m_simThreadMessages.push_back(nm);
            }
        } else {
            std::cout << "Websocket message arrived: UNRECOGNIZED of type " << msgType << std::endl;
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
                    // If a simulation is already in progress, don't allow a new client to
                    //	change the simulation, unless there is only one client connected
                    if (!this->HasActiveClient()
                        || this->NumberOfClients() == 1) {
                        auto runMode = static_cast<SimulationMode>(jsonMsg["mode"].asInt());

                        switch (runMode) {
                        case SimulationMode::id_live_simulation: {
                            std::cout << "Running live simulation" << std::endl;
                            simulation.SetPlaybackMode(runMode);
                            simulation.Reset();
                        } break;
                        case SimulationMode::id_pre_run_simulation: {
                            timeStep = jsonMsg["timeStep"].asFloat();
                            auto numberOfTimeSteps = jsonMsg["numTimeSteps"].asInt();
                            std::cout << "Running pre-run simulation" << std::endl;

                            simulation.SetPlaybackMode(runMode);
                            simulation.Reset();
                            simulation.RunAndSaveFrames(timeStep, numberOfTimeSteps);

                            this->m_trajectoryFileProperties.numberOfFrames = numberOfTimeSteps;
                            this->m_trajectoryFileProperties.numberOfFrames = timeStep;
                            this->SetupRuntimeCacheAsync(simulation, 500);
                        } break;
                        case SimulationMode::id_traj_file_playback: {
                            simulation.SetPlaybackMode(runMode);
                            auto trajectoryFileName = jsonMsg["file-name"].asString();
                            std::cout << "Playing back trajectory file" << std::endl;
                            this->InitializeTrajectoryFile(simulation, senderUid, trajectoryFileName);
                        } break;
                        }
                    }

                    if(jsonMsg.isMember("frameNumber"))
                    {
                        int frameNumber = jsonMsg["frameNumber"].asInt();
                        frameNumber = std::max(frameNumber, 0);

                        std::cout << "[" << senderUid << "] Data request for frame "
                        << frameNumber << std::endl;

                        this->SendDataToClient(
                            simulation,
                            senderUid,
                            frameNumber,
                            true // force
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
                    std::cout << "time step updated to " << timeStep << "\n";
                    this->SendWebsocketMessageToAll(jsonMsg, "time step update");
                } break;
                case WebRequestTypes::id_update_rate_param: {
                    std::string paramName = jsonMsg["paramName"].asString();
                    float paramValue = jsonMsg["paramValue"].asFloat();
                    std::cout << "rate param " << paramName << " updated to " << paramValue << "\n";

                    simulation.UpdateParameter(paramName, paramValue);
                    this->BroadcastParameterUpdate(jsonMsg);
                } break;
                case WebRequestTypes::id_model_definition: {
                    aics::agentsim::Model sim_model;
                    parse_model(jsonMsg, sim_model);
                    print_model(sim_model);
                    simulation.SetModel(sim_model);

                    timeStep = sim_model.max_time_step;
                    std::cout << "Set timestep to " << timeStep << "\n";

                    this->BroadcastModelDefinition(jsonMsg);
                } break;
                case WebRequestTypes::id_play_cache: {
                    if(jsonMsg.isMember("time"))
                    {
                        double timeNs = jsonMsg["time"].asDouble();
                        std::size_t frameNumber = simulation.GetClosestFrameNumberForTime(timeNs);
                        double closestTime = simulation.GetSimulationTimeAtFrame(frameNumber);

                        std::cout << "request to play cached from time "
                        << timeNs << " (frame " << frameNumber << " with time " << closestTime
                        << ") arrived from client " << senderUid << std::endl;

                        this->SetClientFrame(senderUid, frameNumber);
                        this->SetClientState(senderUid, ClientPlayState::Playing);
                    }
                    else if (jsonMsg.isMember("frame-num"))
                    {
                        auto frameNumber = jsonMsg["frame-num"].asInt();
                        std::cout << "request to play cached from frame "
                        << frameNumber << " arrived from client " << senderUid << std::endl;

                        this->SetClientFrame(senderUid, frameNumber);
                        this->SetClientState(senderUid, ClientPlayState::Playing);
                    }

                } break;
                case WebRequestTypes::id_goto_simulation_time: {
                    double timeNs = jsonMsg["time"].asDouble();
                    std::size_t frameNumber = simulation.GetClosestFrameNumberForTime(timeNs);
                    double closestTime = simulation.GetSimulationTimeAtFrame(frameNumber);

                    std::cout << "Client " << senderUid <<
                        " set to frame " << frameNumber << std::endl;

                    this->SetClientFrame(senderUid, frameNumber);
                    this->SetClientState(senderUid, ClientPlayState::Paused);
                    this->SendDataToClient(simulation, senderUid, frameNumber, true);
                } break;
                case WebRequestTypes::id_init_trajectory_file: {
                    std::string fileName = jsonMsg["fileName"].asString();
                    this->InitializeTrajectoryFile(simulation, senderUid, fileName);
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
            std::cout << "Trajectory file not specified, ignoring request" << std::endl;
            return;
        }

        // the relative directory for trajectory files on S3
        //  local downloads mirror the S3 directory structure
        std::string trajectoryFileDirectory = "trajectory/";

        std::string lastLoaded = this->m_trajectoryFileProperties.fileName;
        if(fileName.compare(lastLoaded) == 0)
        {
            std::cout << "Using previously loaded file" << std::endl;
        }
        else {
            std::string filePath = trajectoryFileDirectory + fileName;
            if(simulation.DownloadRuntimeCache(filePath))
            {
                simulation.PreprocessRuntimeCache();
                // We found an already processed run-time cache for this trajectory
                this->m_trajectoryFileProperties.fileName = fileName;
                this->m_trajectoryFileProperties.numberOfFrames = simulation.NumberOfCachedFrames();
            }
            else {
                this->m_trajectoryFileProperties.fileName = fileName;
                simulation.SetPlaybackMode(id_traj_file_playback);
                simulation.Reset();
                simulation.LoadTrajectoryFile(
                    filePath,
                    this->m_trajectoryFileProperties
                );
                this->SetupRuntimeCacheAsync(simulation, 500);
            }
        }

        // Send Trajectory File Properties
        std::cout << "Trajectory File Props " << std::endl
        << "Number of Frames: "
        << this->m_trajectoryFileProperties.numberOfFrames << std::endl
        << "Time step size : "
        << this->m_trajectoryFileProperties.timeStepSize << std::endl;

        Json::Value fprops;
        fprops["msgType"] = WebRequestTypes::id_trajectory_file_info;
        fprops["totalDuration"] =
            this->m_trajectoryFileProperties.numberOfFrames *
            this->m_trajectoryFileProperties.timeStepSize;
        fprops["timeStepSize"] = this->m_trajectoryFileProperties.timeStepSize;

        Json::Value nameMapping;
        for(auto entry : this->m_trajectoryFileProperties.typeMapping)
        {
            std::string id = std::to_string(entry.first);
            std::string name = entry.second;

            nameMapping[id] = name;
        }
        fprops["nameMapping"] = nameMapping;

        this->SendWebsocketMessage(connectionUID, fprops);
        this->SendDataToClient(simulation, connectionUID, 0, true);
    }

    void ConnectionManager::SetupRuntimeCacheAsync(
        Simulation& simulation,
        std::size_t waitTimeMs
    ) {
        if (this->m_fileIoThread.joinable()) {
            this->m_fileIoThread.join();
        }

        this->m_fileIoThread = std::thread([&simulation] {
            // Load the first hundred simulation frames into a runtime cache
            std::cout << "Loading trajectory file into runtime cache" << std::endl;
            std::size_t fn = 0;

            while (!simulation.HasLoadedAllFrames()) {
                simulation.LoadNextFrame();
            }
            std::cout << "Finished loading trajectory into runtime cache" << std::endl;

            // Save the result so it doesn't need to be calculated again
            if(simulation.IsPlayingTrajectory())
            {
                simulation.UploadRuntimeCache();
            }
        });

        std::this_thread::sleep_for(std::chrono::milliseconds(waitTimeMs));
    }

} // namespace agentsim
} // namespace aics
