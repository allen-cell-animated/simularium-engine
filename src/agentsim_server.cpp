#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>

#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <json/json.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "agentsim/agentsim.h"

namespace utility {
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
};

#define HEART_BEAT_INTERVAL_SECONDS 15
#define MAX_MISSED_HEARTBEATS_BEFORE_TIMEOUT 4
#define MAX_NUM_FRAMES std::numeric_limits<std::size_t>::max()
#define NO_CLIENT_TIMEOUT_SECONDS 30
#define SERVER_TICK_INTERVAL_MILLISECONDS 200

typedef websocketpp::server<websocketpp::config::asio> server;
using namespace aics::agentsim;

struct NetMessage {
    std::string sender_uid;
    Json::Value jsonMessage;
};

std::vector<NetMessage> simThreadMessages;
std::vector<NetMessage> heartBeatMessages;
std::string latest_conn_uid = "";

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

// The following are mapped using a local uid
std::unordered_map<std::string, NetState> net_states;
std::unordered_map<std::string, websocketpp::connection_hdl> net_connections;
std::unordered_map<std::string, int> missed_net_heartbeats;

std::mutex net_msg_mtx;
std::atomic<bool> has_simulation_model { false };
std::atomic<bool> has_unhandled_new_connection { false };
Json::Value most_recent_model = "";
std::vector<Json::Value> param_cache;

bool use_readdy = true;
bool use_cytosim = !use_readdy;

std::string trajectory_file_directory = "trajectory/";

Json::StreamWriterBuilder jsonStreamWriter;
Json::CharReaderBuilder json_read_builder;
std::unique_ptr<Json::CharReader> const json_reader(json_read_builder.newCharReader());

enum {
    id_undefined_web_request = 0,
    id_vis_data_arrive = 1,
    id_vis_data_request,
    id_vis_data_finish,
    id_vis_data_pause,
    id_vis_data_resume,
    id_vis_data_abort,
    id_update_time_step,
    id_update_rate_param,
    id_model_definition,
    id_heartbeat_ping,
    id_heartbeat_pong,
    id_play_cache
};

std::vector<std::string> webRequestNames {
    "undefined",
    "stream data",
    "stream request",
    "stream finish",
    "pause",
    "resume",
    "abort",
    "update time step",
    "update rate param",
    "model definition",
    "heartbeat ping",
    "heartbeat pong",
    "play cache"
};

enum {
    id_live_simulation = 0,
    id_pre_run_simulation = 1,
    id_traj_file_playback = 2
};

template <typename T, typename U>
inline bool equals(const std::weak_ptr<T>& t, const std::weak_ptr<U>& u)
{
    return !t.owner_before(u) && !u.owner_before(t);
}

void on_message(websocketpp::connection_hdl hd1, server::message_ptr msg)
{
    NetMessage nm;
    std::string message = msg->get_payload();
    std::string errs;

    //@TODO: Not every message needs the uid of the sender
    //	verify if this does not have a significant impact on perf
    for (auto& entry : net_connections) {
        auto& uid = entry.first;
        auto& conn = entry.second;

        if (equals<void, void>(conn, hd1)) {
            nm.sender_uid = uid;
            break;
        }
    }

    json_reader->parse(message.c_str(), message.c_str() + message.length(),
        &(nm.jsonMessage), &errs);

    auto msgType = nm.jsonMessage["msg_type"].asInt();

    if(msgType >= 0 && msgType <= webRequestNames.size())
    {
        std::cout << "[" << nm.sender_uid << "] Web socket message arrived: " << webRequestNames[msgType] << std::endl;

        if(msgType == id_heartbeat_pong) { heartBeatMessages.push_back(nm); }
        else { simThreadMessages.push_back(nm); }
    }
    else
    {
        std::cout << "Websocket message arrived: UNRECOGNIZED of type " << msgType << std::endl;
    }
}

std::vector<std::string> uids_to_delete;
void on_close(websocketpp::connection_hdl hd1)
{
    for (auto& entry : net_connections) {
        auto& uid = entry.first;
        auto& conn = entry.second;

        if (conn.expired()
            || equals<void, void>(conn, hd1)) {
            std::cout << "Marking network connection for closure.\n";
            uids_to_delete.push_back(uid);
        }
    }
}

void on_open(websocketpp::connection_hdl hd1)
{
    std::cout << "Incoming connection accepted.\n";
    has_unhandled_new_connection = true;

    std::string uid;
    utility::GenerateLocalUUID(uid);

    net_states[uid] = NetState();
    missed_net_heartbeats[uid] = 0;
    net_connections[uid] = hd1;
    latest_conn_uid = uid;
}

void sendWebsocketMessage(server* webSocketServer, std::string connectionUID, Json::Value jsonMessage)
{
    if(webSocketServer == nullptr)
    {
        std::cout << "Trying to send a messge with a null/invalid server" << std::endl;
        return;
    }

    jsonMessage["conn_id"] = connectionUID;
    std::string message = Json::writeString(jsonStreamWriter, jsonMessage);

    try {
        webSocketServer->send(net_connections[connectionUID], message, websocketpp::frame::opcode::text);
    }
    catch(...)
    {
        std::cout << "Ignoring failed websocket send" << std::endl;
    }
}

void sendWebsocketMessageToAll(server* webSocketServer, Json::Value jsonMessage, std::string description)
{
    if(webSocketServer == nullptr)
    {
        std::cout << "Trying to send a messge with a null/invalid server" << std::endl;
        return;
    }

    std::cout << "Sending message to all clients: " << description << std::endl;
    for(auto& entry : net_connections)
    {
        auto uid = entry.first;
        sendWebsocketMessage(webSocketServer, uid, jsonMessage);
    }
}

// Unresponsive Clients

// Does not close connection, removes it from data structures
void removeConnection(std::string connectionUID) {
    std::cout << "Removing closed network connection.\n";
    net_connections.erase(connectionUID);
    missed_net_heartbeats.erase(connectionUID);
    net_states.erase(connectionUID);
}

// Close a connection and remove it from data structures
void closeConnection(server* webSocketServer, std::string connectionUID) {
    if(webSocketServer == nullptr)
    {
        std::cout << "Trying to send a messge with a null/invalid server" << std::endl;
        return;
    }

    auto& conn = net_connections[connectionUID];
    webSocketServer->pause_reading(conn);
    webSocketServer->close(conn, 0, "");

    removeConnection(connectionUID);
}

void removeUnresponsiveClients(server* webSocketServer) {
    if(webSocketServer == nullptr) { return; }

    for (auto& entry : net_connections) {
        auto& current_uid = entry.first;
        missed_net_heartbeats[current_uid]++;

        if (missed_net_heartbeats[current_uid] > MAX_MISSED_HEARTBEATS_BEFORE_TIMEOUT) {
            std::cout << "Removing unresponsive network connection.\n";
            closeConnection(webSocketServer, current_uid);
        }
    }
}

std::size_t numberOfClients() { return net_connections.size(); }

void registerHeartBeat(std::string connectionUID) {
    missed_net_heartbeats[connectionUID] = 0;
}

bool hasActiveClient() {
    for (auto& entry : net_states) {
        auto& net_state = entry.second;
        if (net_state.play_state == ClientPlayState::Playing) {
            return true;
        }
    }

    return false;
}

void setClientState(std::string connectionUID, ClientPlayState state)
{
    net_states[connectionUID].play_state = state;
}

void setClientFrame(std::string connectionUID, std::size_t frameNumber)
{
    net_states[connectionUID].frame_no = frameNumber;
}

bool allClientsArePlayingFromCache(std::size_t numberOfFrames, bool allFramesLoaded)
{
    bool returnValue = true;
    for (auto& entry : net_states) {
        auto& connectionUID = entry.first;
        auto& netState = entry.second;

        if (netState.play_state != ClientPlayState::Playing) {
            continue;
        }

        if (netState.frame_no >= numberOfFrames - 1) {
            if (netState.frame_no != MAX_NUM_FRAMES) {
                std::cout << "End of simulation cache reached for client " << connectionUID << std::endl;
                netState.frame_no = MAX_NUM_FRAMES;
            }

            if (allFramesLoaded) {
                std::cout << "Simulation finished for client " << connectionUID << std::endl;
                setClientState(connectionUID, ClientPlayState::Finished);
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

int main(int argc, char* argv[])
{
    bool argNoTimeout = false;
    for (int i = 0; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg.compare("--no-exit") == 0) {
            argNoTimeout = true;
        }
    }

    server sim_server;
    sim_server.set_reuse_addr(true);
    std::atomic<bool> isServerRunning { true };

    auto ws_thread = std::thread([&] {
        sim_server.set_message_handler(&on_message);
        sim_server.set_close_handler(&on_close);
        sim_server.set_open_handler(&on_open);
        sim_server.set_access_channels(websocketpp::log::alevel::none);
        sim_server.set_error_channels(websocketpp::log::elevel::none);

        sim_server.init_asio();
        sim_server.listen(9002);
        sim_server.start_accept();

        sim_server.run();
    });

    auto sim_thread = std::thread([&] {
        // Simulation thread/timing variables
        bool isSimulationSetup = false;
        int run_mode = 0; // live simulation

        float time_step = 1e-12; // seconds
        std::size_t n_time_steps = 1;
        std::string traj_file_name = "";

        // Simulation setup
        std::vector<std::shared_ptr<SimPkg>> simulators;

        if (use_readdy) {
            ReaDDyPkg* readdySimPkg = new ReaDDyPkg();
            std::shared_ptr<SimPkg> readdyPkg;
            readdyPkg.reset(readdySimPkg);
            simulators.push_back(readdyPkg);
        }

        if (use_cytosim) {
            CytosimPkg* cytosimSimPkg = new CytosimPkg();
            std::shared_ptr<SimPkg> cytosimPkg;
            cytosimPkg.reset(cytosimSimPkg);
            simulators.push_back(cytosimPkg);
        }

        std::vector<std::shared_ptr<Agent>> agents;
        Simulation simulation(simulators, agents);
        //simulation.Reset();

        // Runtime loop
        while (isServerRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SERVER_TICK_INTERVAL_MILLISECONDS));

            // Remove expired network connections
            for (std::size_t i = 0; i < uids_to_delete.size(); ++i) {
                auto uid = uids_to_delete[i];
                removeConnection(uid);
            }
            uids_to_delete.clear();

            /**
			* Send data to new connections
			*/
            if (has_unhandled_new_connection && has_simulation_model) {
                std::cout << "Sending current model to most recent client.\n";
                sendWebsocketMessage(&sim_server, latest_conn_uid, most_recent_model);
                has_unhandled_new_connection = false;
            }

            /**
			*	Check current state of the simulation
			*/
            bool isRunningSimulation = false;
            bool isSimulationPaused = true;

            if(hasActiveClient())
            {
                isRunningSimulation = true;
                isSimulationPaused = false;
            }

            // handle net messages
            if (simThreadMessages.size() > 0) {
                for (std::size_t i = 0; i < simThreadMessages.size(); ++i) {
                    std::string sender_uid = simThreadMessages[i].sender_uid;
                    Json::Value json_msg = simThreadMessages[i].jsonMessage;

                    int msg_type = json_msg["msg_type"].asInt();
                    switch (msg_type) {
                    case id_vis_data_request: {
                        // If a simulation is already in progress, don't allow a new client to
                        //	change the simulation, unless there is only one client connected
                        if (!isRunningSimulation || numberOfClients() == 1) {
                            isSimulationSetup = false;
                            run_mode = json_msg["mode"].asInt();

                            switch (run_mode) {
                            case id_live_simulation: {
                                std::cout << "Running live simulation" << std::endl;
                                simulation.SetPlaybackMode(id_live_simulation);
                            } break;
                            case id_pre_run_simulation: {
                                time_step = json_msg["time-step"].asFloat();
                                n_time_steps = json_msg["num-time-steps"].asInt();
                                std::cout << "Running pre-run simulation" << std::endl;
                                simulation.SetPlaybackMode(id_pre_run_simulation);
                            } break;
                            case id_traj_file_playback: {
                                traj_file_name = json_msg["file-name"].asString();
                                std::cout << "Playing back trajectory file" << std::endl;
                                simulation.SetPlaybackMode(id_traj_file_playback);
                            } break;
                            }

                            simulation.Reset();
                        }

                        setClientState(sender_uid, ClientPlayState::Playing);
                        setClientFrame(sender_uid, 0);
                    } break;
                    case id_vis_data_pause: { setClientState(sender_uid, ClientPlayState::Paused); } break;
                    case id_vis_data_resume: { setClientState(sender_uid, ClientPlayState::Playing); } break;
                    case id_vis_data_abort: { setClientState(sender_uid, ClientPlayState::Stopped); } break;
                    case id_update_time_step: {
                        time_step = json_msg["time_step"].asFloat();
                        std::cout << "time step updated to " << time_step << "\n";
                        sendWebsocketMessageToAll(&sim_server, json_msg, "time-step update");
                    } break;
                    case id_update_rate_param: {
                        std::string param_name = json_msg["param_name"].asString();
                        float param_value = json_msg["param_value"].asFloat();
                        simulation.UpdateParameter(param_name, param_value);

                        std::cout << "rate param " << param_name << " updated to " << param_value << "\n";
                        param_cache.push_back(json_msg);
                        sendWebsocketMessageToAll(&sim_server, json_msg, "rate-parameter update");
                    } break;
                    case id_model_definition: {
                        aics::agentsim::Model sim_model;
                        parse_model(json_msg, sim_model);
                        print_model(sim_model);
                        simulation.SetModel(sim_model);

                        has_simulation_model = true;
                        most_recent_model = json_msg;

                        time_step = sim_model.max_time_step;
                        std::cout << "Set timestep to " << time_step << "\n";

                        param_cache.clear();
                        sendWebsocketMessageToAll(&sim_server, json_msg, "model definition");
                    } break;
                    case id_play_cache: {
                        auto frame_no = json_msg["frame-num"].asInt();
                        std::cout << "request to play cached from frame "
                                  << frame_no << " arrived from client " << sender_uid << std::endl;

                        setClientFrame(sender_uid, frame_no);
                        setClientState(sender_uid, ClientPlayState::Playing);
                    } break;
                    default: { } break;
                    }
                }

                simThreadMessages.clear();
            }

            /**
			*	If simulation isn't running, no need to continue
			*/
            if (!isRunningSimulation || isSimulationPaused) {
                continue;
            }

            /**
      * Run simulation setup
      *
      * Either run the simulation to completion, load a trajectory file,
      * or do nothing in the case of 'live' simulation
      *
      */

            if (!isSimulationSetup) {
                if (run_mode == id_pre_run_simulation) {
                    simulation.RunAndSaveFrames(time_step, n_time_steps);
                }

                if (run_mode == id_traj_file_playback) {
                    simulation.LoadTrajectoryFile(trajectory_file_directory + traj_file_name);
                }

                if (run_mode == id_pre_run_simulation
                    || run_mode == id_traj_file_playback) {
                    // Load the first hundred simulation frames into a runtime cache
                    std::cout << "Loading trajectory file into runtime cache" << std::endl;
                    std::size_t fn = 0;
                    while (!simulation.HasLoadedAllFrames() && fn < 100) {
                        std::cout << "Loading frame " << ++fn << std::endl;
                        simulation.LoadNextFrame();
                    }
                    std::cout << "Finished loading trajectory for now" << std::endl;
                }

                isSimulationSetup = true;
            }

            // Run simulation timestep every 66 milliseconds

            // Run simulation time-step
            if (!allClientsArePlayingFromCache(simulation.GetNumFrames(), simulation.HasLoadedAllFrames())) {
                if (run_mode == id_live_simulation) {
                    simulation.RunTimeStep(time_step);
                }
                else if (run_mode == id_pre_run_simulation || run_mode == id_traj_file_playback) {
                    if (!simulation.HasLoadedAllFrames()) {
                        simulation.LoadNextFrame();
                    }
                }
            }

            /**
      * JSON Net Serialization
      */
            for (auto& entry : net_states) {
                auto& net_id = entry.first;
                auto& net_state = entry.second;

                if (net_state.play_state != ClientPlayState::Playing) {
                    continue;
                }

                AgentDataFrame simData;

                // This state is currently being used to demark the 'latest' frame
                if (net_state.frame_no == MAX_NUM_FRAMES) {
                    simData = simulation.GetDataFrame(simulation.GetNumFrames() - 1);
                } else {
                    simData = simulation.GetDataFrame(net_state.frame_no++);
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
                sendWebsocketMessage(&sim_server, net_id, net_agent_data_frame);
            }
        }
    });

    auto heartbeat_thread = std::thread([&] {
        auto no_client_timer = std::chrono::system_clock::now();

        while (isServerRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(HEART_BEAT_INTERVAL_SECONDS));

            if (!argNoTimeout) {
                if (numberOfClients() == 0) {
                    auto now = std::chrono::system_clock::now();
                    auto diff = now - no_client_timer;

                    if (diff >= std::chrono::seconds(NO_CLIENT_TIMEOUT_SECONDS)) {
                        std::cout << "No clients connected for " << NO_CLIENT_TIMEOUT_SECONDS << " seconds, exiting server ... " << std::endl;
                        isServerRunning = false;
                        std::raise(SIGKILL);
                    }

                    // If there are no clients, no need to continue
                    continue;
                } else {
                    no_client_timer = std::chrono::system_clock::now();
                }
            }

            if (heartBeatMessages.size() > 0) {
                for (std::size_t i = 0; i < heartBeatMessages.size(); ++i) {
                    Json::Value json_msg = heartBeatMessages[i].jsonMessage;

                    int msg_type = json_msg["msg_type"].asInt();
                    if(msg_type == id_heartbeat_pong)
                    {
                        auto conn_id = json_msg["conn_id"].asString();
                        registerHeartBeat(conn_id);
                    }
                }

                heartBeatMessages.clear();
            }

            Json::Value pingJsonMessage;
            pingJsonMessage["msg_type"] = id_heartbeat_ping;

            removeUnresponsiveClients(&sim_server);
            sendWebsocketMessageToAll(&sim_server, pingJsonMessage, "Heartbeat ping");
        }
    });

    auto io_thread = std::thread([&] {
        std::string input;
        std::cout << "Enter 'quit' to exit\n";
        while (isServerRunning && std::getline(std::cin, input, '\n')) {
            if (input == "quit") {
                isServerRunning = false;
            } else {
                input = "";
            }
        }
    });

    while (isServerRunning) {
        std::this_thread::sleep_for(std::chrono::seconds(1));
    }

    std::cout << "Exiting Server...\n";
    sim_thread.join();
    heartbeat_thread.join();
    io_thread.detach();
    ws_thread.detach();
}
