#ifndef AICS_CONNECTION_MANAGER_H
#define AICS_CONNECTION_MANAGER_H

#include <string>
#include <vector>
#include <iostream>
#include <unordered_map>

#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <json/json.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "agentsim/simulation.h"
#include "agentsim/network/net_message_ids.h"

typedef websocketpp::server<websocketpp::config::asio> server;

template <typename T, typename U>
inline bool equals(const std::weak_ptr<T>& t, const std::weak_ptr<U>& u)
{
    return !t.owner_before(u) && !u.owner_before(t);
}

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

    class ConnectionManager {
    public:
        ConnectionManager() {
            this->m_server.set_reuse_addr(true);
        }

        server* getServer () {
            return &(this->m_server);
        }

        void addConnection(websocketpp::connection_hdl hd1, std::string& newUid)
        {
            this->GenerateLocalUUID(newUid);
            this->m_netStates[newUid] = NetState();
            this->m_missedHeartbeats[newUid] = 0;
            this->m_netConnections[newUid] = hd1;
            std::cout << "Incoming connection accepted." << std::endl;
        }

        void removeConnection(std::string connectionUID)
        {
            std::cout << "Removing closed network connection.\n";
            this->m_netConnections.erase(connectionUID);
            this->m_missedHeartbeats.erase(connectionUID);
            this->m_netStates.erase(connectionUID);
        }

        void closeConnection(std::string connectionUID) {
            auto& conn = this->m_netConnections[connectionUID];
            this->m_server.pause_reading(conn);
            this->m_server.close(conn, 0, "");

            removeConnection(connectionUID);
        }

        void removeUnresponsiveClients() {
            for (auto& entry : this->m_netConnections) {
                auto& current_uid = entry.first;
                this->m_missedHeartbeats[current_uid]++;

                if (this->m_missedHeartbeats[current_uid] > this->kMaxMissedHeartBeats) {
                    std::cout << "Removing unresponsive network connection.\n";
                    this->closeConnection(current_uid);
                }
            }
        }

        std::string getUid(websocketpp::connection_hdl hd1)
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

        bool hasActiveClient() {
            for (auto& entry : this->m_netStates) {
                auto& netState = entry.second;
                if (netState.play_state == ClientPlayState::Playing) {
                    return true;
                }
            }

            return false;
        }

        void setClientState(std::string connectionUID, ClientPlayState state)
        {
            this->m_netStates[connectionUID].play_state = state;
        }

        void setClientFrame(std::string connectionUID, std::size_t frameNumber)
        {
            this->m_netStates[connectionUID].frame_no = frameNumber;
        }

        bool allClientsArePlayingFromCache(std::size_t numberOfFrames, bool allFramesLoaded)
        {
            bool returnValue = true;
            for (auto& entry : this->m_netStates) {
                auto& connectionUID = entry.first;
                auto& netState = entry.second;

                if (netState.play_state != ClientPlayState::Playing) {
                    continue;
                }

                if (netState.frame_no >= numberOfFrames - 1) {
                    if (netState.frame_no != this->kLatestFrameValue) {
                        std::cout << "End of simulation cache reached for client " << connectionUID << std::endl;
                        netState.frame_no = this->kLatestFrameValue;
                    }

                    if (allFramesLoaded) {
                        std::cout << "Simulation finished for client " << connectionUID << std::endl;
                        this->setClientState(connectionUID, ClientPlayState::Finished);
                    } else {
                        returnValue = false;
                    }
                }

                if (numberOfFrames == 0) {
                    returnValue = false;
                }
            }

            return returnValue;
        }

        void markConnectionExpired(websocketpp::connection_hdl hd1)
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

        void removeExpiredConnections() {
            for(auto& uid : this->m_uidsToDelete)
            {
                this->removeConnection(uid);
            }

            this->m_uidsToDelete.clear();
        }

        // Web socket message sends
        void sendWebsocketMessage(std::string connectionUID, Json::Value jsonMessage)
        {
            jsonMessage["conn_id"] = connectionUID;
            std::string message = Json::writeString(this->m_jsonStreamWriter, jsonMessage);

            try {
                this->m_server.send(
                    this->m_netConnections[connectionUID],
                    message, websocketpp::frame::opcode::text);
            }
            catch(...)
            {
                std::cout << "Ignoring failed websocket send" << std::endl;
            }
        }

        void sendWebsocketMessageToAll(Json::Value jsonMessage, std::string description)
        {
            std::cout << "Sending message to all clients: " << description << std::endl;
            for(auto& entry : this->m_netConnections)
            {
                auto uid = entry.first;
                sendWebsocketMessage(uid, jsonMessage);
            }
        }

        std::size_t numberOfClients() { return this->m_netConnections.size(); }

        void registerHeartBeat(std::string connectionUID) {
            this->m_missedHeartbeats[connectionUID] = 0;
        }

        void sendDataToClients(Simulation& simulation) {
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
                    simData = simulation.GetDataFrame(netState.frame_no++);
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
                this->sendWebsocketMessage(uid, net_agent_data_frame);
            }
        }

    private:
        void GenerateLocalUUID(std::string& uuid)
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

        std::unordered_map<std::string, NetState> m_netStates;
        std::unordered_map<std::string, websocketpp::connection_hdl> m_netConnections;
        std::unordered_map<std::string, std::size_t> m_missedHeartbeats;
        server m_server;
        std::vector<std::string> m_uidsToDelete;

        Json::StreamWriterBuilder m_jsonStreamWriter;
        const std::size_t kLatestFrameValue = std::numeric_limits<std::size_t>::max();
        const std::size_t kMaxMissedHeartBeats = 4;
    };

} // namespace agentsim
} // namespace aics

#endif // AICS_CONNECTION_MANAGER_H
