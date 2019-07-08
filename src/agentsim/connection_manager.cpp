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
        this->m_server.set_reuse_addr(true);
    }

    server* ConnectionManager::GetServer()
    {
        return &(this->m_server);
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
        for (auto& entry : this->m_netConnections) {
            auto& current_uid = entry.first;
            this->m_missedHeartbeats[current_uid]++;

            if (this->m_missedHeartbeats[current_uid] > this->kMaxMissedHeartBeats) {
                std::cout << "Removing unresponsive network connection.\n";
                this->CloseConnection(current_uid);
            }
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

} // namespace agentsim
} // namespace aics
