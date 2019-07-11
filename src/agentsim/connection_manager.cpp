#include "agentsim/network/connection_manager.h"
#include "agentsim/network/net_message_ids.h"

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
        this->m_server.stop_listening();
        this->m_server.stop();
        this->m_listeningThread.detach();
    }

    void ConnectionManager::Listen()
    {
        this->m_listeningThread = std::thread([&] {
            this->m_server.set_reuse_addr(true);
            this->m_server.set_message_handler(
                std::bind(
                    &ConnectionManager::OnMessage,
                    this,
                    std::placeholders::_1,
                    std::placeholders::_2)
                );
                this->m_server.set_close_handler(
                    std::bind(
                        &ConnectionManager::MarkConnectionExpired,
                        this,
                        std::placeholders::_1)
                    );
                    this->m_server.set_open_handler(
                        std::bind(
                            &ConnectionManager::AddConnection,
                            this,
                            std::placeholders::_1)
                        );
                        this->m_server.set_access_channels(websocketpp::log::alevel::none);
                        this->m_server.set_error_channels(websocketpp::log::elevel::none);

                        this->m_server.init_asio();
                        this->m_server.listen(9002);
                        this->m_server.start_accept();

                        this->m_server.run();
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
        std::cout << "Incoming connection accepted." << std::endl;
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

        for(auto& uid : toRemove)
        {
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

    void ConnectionManager::CheckForFinishedClients(
        std::size_t numberOfFrames, bool allFramesLoaded)
    {
        for (auto& entry : this->m_netStates) {
            auto& connectionUID = entry.first;
            auto& netState = entry.second;

            if (netState.play_state != ClientPlayState::Playing) {
                continue;
            }

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
        jsonMessage["conn_id"] = connectionUID;
        std::string message = Json::writeString(this->m_jsonStreamWriter, jsonMessage);

        try {
            this->m_server.send(
                this->m_netConnections[connectionUID],
                message, websocketpp::frame::opcode::text);
        } catch (...) {
            std::cout << "Ignoring failed websocket send" << std::endl;
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

            if (netState.play_state != ClientPlayState::Playing) {
                continue;
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

            net_agent_data_frame["msg_type"] = id_vis_data_arrive;
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
            this->SendWebsocketMessage(uid, net_agent_data_frame);
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
        pingJsonMessage["msg_type"] = id_heartbeat_ping;

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

    void ConnectionManager::HandleMessage(NetMessage nm)
    {
        auto msgType = nm.jsonMessage["msg_type"].asInt();

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

                int msg_type = jsonMsg["msg_type"].asInt();
                switch (msg_type) {
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
                            timeStep = jsonMsg["time-step"].asFloat();
                            auto n_time_steps = jsonMsg["num-time-steps"].asInt();
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

                            simulation.SetPlaybackMode(runMode);
                            simulation.Reset();
                            simulation.LoadTrajectoryFile(trajectory_file_directory + trajectoryFileName);
                        } break;
                        }

                        if (runMode == id_pre_run_simulation
                            || runMode == id_traj_file_playback) {
                            // Load the first hundred simulation frames into a runtime cache
                            std::cout << "Loading trajectory file into runtime cache" << std::endl;
                            std::size_t fn = 0;
                            while (!simulation.HasLoadedAllFrames() && fn < 100) {
                                std::cout << "Loading frame " << ++fn << std::endl;
                                simulation.LoadNextFrame();
                            }
                            std::cout << "Finished loading trajectory for now" << std::endl;
                        }
                    }

                    this->SetClientState(senderUid, ClientPlayState::Playing);
                    this->SetClientFrame(senderUid, 0);
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
                    timeStep = jsonMsg["time_step"].asFloat();
                    std::cout << "time step updated to " << timeStep << "\n";
                    this->SendWebsocketMessageToAll(jsonMsg, "time-step update");
                } break;
                case WebRequestTypes::id_update_rate_param: {
                    std::string paramName = jsonMsg["param_name"].asString();
                    float paramValue = jsonMsg["param_value"].asFloat();
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
                    auto frameNumber = jsonMsg["frame-num"].asInt();
                    std::cout << "request to play cached from frame "
                              << frameNumber << " arrived from client " << senderUid << std::endl;

                    this->SetClientFrame(senderUid, frameNumber);
                    this->SetClientState(senderUid, ClientPlayState::Playing);
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
