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
                } else {
                    if (!simulation.HasLoadedAllFrames()) {
                        simulation.LoadNextFrame();
                    }
                }

                std::size_t numberOfFrames = simulation.GetNumFrames();
                bool hasFinishedLoading = simulation.HasLoadedAllFrames();

                this->CheckForFinishedClients(numberOfFrames, hasFinishedLoading);
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
            if (netState.play_state == ClientPlayState::Playing) {
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
        std::size_t numberOfFrames,
        bool allFramesLoaded,
        std::string connectionUID,
        NetState& netState)
    {
        if (netState.frame_no >= numberOfFrames - 1) {
            if (netState.frame_no != this->kLatestFrameValue) {
                std::cout << "End of simulation cache reached for client "
                          << connectionUID << std::endl;
                netState.frame_no = this->kLatestFrameValue;
            }

            if (allFramesLoaded) {
                std::cout << "Simulation finished for client " << connectionUID << std::endl;
                this->SetClientState(connectionUID, ClientPlayState::Finished);
            }
        }
    }

    void ConnectionManager::CheckForFinishedClients(
        std::size_t numberOfFrames, bool allFramesLoaded)
    {
        for (auto& entry : this->m_netStates) {
            auto& connectionUID = entry.first;
            auto& netState = entry.second;

            if (netState.play_state != ClientPlayState::Playing) {
                continue;
            }

            this->CheckForFinishedClient(numberOfFrames, allFramesLoaded, connectionUID, netState);
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

            if (netState.play_state != ClientPlayState::Playing
                || netState.frame_no == this->kLatestFrameValue) {
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
        std::size_t start,
        std::size_t count
    )
    {
        std::size_t numberOfFrames = simulation.GetNumFrames();
        bool hasFinishedLoading = simulation.HasLoadedAllFrames();
        this->SetClientState(connectionUID, ClientPlayState::Playing);

        auto& netState = this->m_netStates.at(connectionUID);
        for(std::size_t i = 0; i < count; ++i)
        {
            this->SendDataToClient(simulation, connectionUID, start + i);
            netState.frame_no++;
            this->CheckForFinishedClient(
                numberOfFrames,
                hasFinishedLoading,
                connectionUID,
                netState);
        }

        if(netState.play_state == ClientPlayState::Playing)
        {
            this->SetClientState(connectionUID, ClientPlayState::Stopped);
        }
    }

    void ConnectionManager::SendDataToClient(
        Simulation& simulation,
        std::string connectionUID,
        std::size_t frameNumber)
    {
        auto& netState = this->m_netStates.at(connectionUID);

        if (netState.play_state != ClientPlayState::Playing) {
            return;
        }

        AgentDataFrame simData;

        // This state is currently being used to demark the 'latest' frame
        if (netState.frame_no == this->kLatestFrameValue) {
            simData = simulation.GetDataFrame(simulation.GetNumFrames() - 1);
        } else {
            simData = simulation.GetDataFrame(netState.frame_no);
        }

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
        net_agent_data_frame["time"] = simulation.GetTime(netState.frame_no);
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
        // the relative directory for trajectory files on S3
        //  local downloads mirror the S3 directory structure
        std::string trajectory_file_directory = "trajectory/";
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
                        auto runMode = jsonMsg["mode"].asInt();

                        switch (runMode) {
                        case id_live_simulation: {
                            std::cout << "Running live simulation" << std::endl;
                            simulation.SetPlaybackMode(runMode);
                            simulation.Reset();
                        } break;
                        case id_pre_run_simulation: {
                            timeStep = jsonMsg["timeStep"].asFloat();
                            auto n_time_steps = jsonMsg["numTimeSteps"].asInt();
                            std::cout << "Running pre-run simulation" << std::endl;

                            simulation.SetPlaybackMode(runMode);
                            simulation.Reset();
                            simulation.RunAndSaveFrames(timeStep, n_time_steps);
                        } break;
                        case id_traj_file_playback: {
                            auto trajectoryFileName = jsonMsg["file-name"].asString();
                            std::cout << "Playing back trajectory file" << std::endl;

                            if (trajectoryFileName.empty()) {
                                std::cout << "Trajectory file not specified, ignoring request" << std::endl;
                                continue;
                            }

                            TrajectoryFileProperties trajProps;
                            simulation.SetPlaybackMode(runMode);
                            simulation.Reset();
                            simulation.LoadTrajectoryFile(
                                trajectory_file_directory + trajectoryFileName,
                                trajProps
                            );

                            std::cout << "Trajectory File Props " << std::endl
                            << "Number of Frames: " << trajProps.numberOfFrames << std::endl
                            << "Time step size : " << trajProps.timeStepSize << std::endl;

                            Json::Value fprops;
                            fprops["msgType"] = WebRequestTypes::id_trajectory_file_info;
                            fprops["numberOfFrames"] = trajProps.numberOfFrames;
                            fprops["timeStepSize"] = trajProps.timeStepSize;
                            this->SendWebsocketMessage(senderUid, fprops);
                        } break;
                        }

                        if (runMode == id_pre_run_simulation
                            || runMode == id_traj_file_playback) {
                            // Load the first hundred simulation frames into a runtime cache
                            std::cout << "Loading trajectory file into runtime cache" << std::endl;
                            std::size_t fn = 0;
                            std::size_t count = 100;

                            if(jsonMsg.isMember("count"))
                            {
                                count = jsonMsg["count"].asInt() + 1;
                            }

                            while (!simulation.HasLoadedAllFrames() && fn < count) {
                                std::cout << "Loading frame " << ++fn << std::endl;
                                simulation.LoadNextFrame();
                            }
                            std::cout << "Finished loading trajectory for now" << std::endl;
                        }
                    }

                    if(jsonMsg.isMember("count"))
                    {
                        // this is arbitrarily capped at 5
                        //  the current expected usage is to ask for a single frame
                        std::size_t count = std::min(jsonMsg["count"].asInt(), 5);
                        auto& netState = this->m_netStates.at(senderUid);
                        netState.frame_no = jsonMsg["frameNumber"].asInt();

                        this->SendDataToClient(simulation, senderUid, netState.frame_no, count);
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
                        std::size_t frameNumber = simulation.GetFrameNumber(timeNs);
                        double closestTime = simulation.GetTime(frameNumber);

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
                default: {
                } break;
                }
            }

            messages.clear();
        }
    }

} // namespace agentsim
} // namespace aics
