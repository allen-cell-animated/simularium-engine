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

#define HEART_BEAT_INTERVAL_SECONDS 15
#define SERVER_TICK_INTERVAL_MILLISECONDS 200

using namespace aics::agentsim;

aics::agentsim::ConnectionManager connectionManager;

struct NetMessage {
    std::string sender_uid;
    Json::Value jsonMessage;
};

std::vector<NetMessage> simThreadMessages;
std::vector<NetMessage> heartBeatMessages;

bool use_readdy = true;
bool use_cytosim = !use_readdy;

std::string trajectory_file_directory = "trajectory/";

Json::CharReaderBuilder json_read_builder;
std::unique_ptr<Json::CharReader> const json_reader(json_read_builder.newCharReader());

// Web Socket handlers
void on_message(websocketpp::connection_hdl hd1, server::message_ptr msg)
{
    NetMessage nm;
    nm.sender_uid = connectionManager.getUid(hd1);
    std::string message = msg->get_payload();
    std::string errs;

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

void on_close(websocketpp::connection_hdl hd1)
{
    connectionManager.markConnectionExpired(hd1);
}

void on_open(websocketpp::connection_hdl hd1)
{
    connectionManager.addConnection(hd1);
}

int main(int argc, char* argv[])
{
    for (int i = 0; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg.compare("--no-exit") == 0) {
            connectionManager.setNoTimeoutArg(true);
        }
    }

    std::atomic<bool> isServerRunning { true };

    auto ws_thread = std::thread([&] {
        auto server = connectionManager.getServer();
        if(server != nullptr)
        {
            server->set_message_handler(on_message);
            server->set_close_handler(on_close);
            server->set_open_handler(on_open);
            server->set_access_channels(websocketpp::log::alevel::none);
            server->set_error_channels(websocketpp::log::elevel::none);

            server->init_asio();
            server->listen(9002);
            server->start_accept();

            server->run();
        }
    });

    auto sim_thread = std::thread([&] {
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

        // Runtime loop
        while (isServerRunning) {
            std::this_thread::sleep_for(std::chrono::milliseconds(SERVER_TICK_INTERVAL_MILLISECONDS));

            connectionManager.removeExpiredConnections();
            connectionManager.updateNewConections();

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
                        if (!connectionManager.hasActiveClient()
                            || connectionManager.numberOfClients() == 1) {
                            auto runMode = json_msg["mode"].asInt();

                            switch (runMode) {
                            case id_live_simulation: {
                                std::cout << "Running live simulation" << std::endl;
                                simulation.SetPlaybackMode(runMode);
                                simulation.Reset();
                            } break;
                            case id_pre_run_simulation: {
                                time_step = json_msg["time-step"].asFloat();
                                n_time_steps = json_msg["num-time-steps"].asInt();
                                std::cout << "Running pre-run simulation" << std::endl;

                                simulation.SetPlaybackMode(runMode);
                                simulation.Reset();
                                simulation.RunAndSaveFrames(time_step, n_time_steps);
                            } break;
                            case id_traj_file_playback: {
                                traj_file_name = json_msg["file-name"].asString();
                                std::cout << "Playing back trajectory file" << std::endl;
                                simulation.SetPlaybackMode(runMode);
                                simulation.Reset();
                                simulation.LoadTrajectoryFile(trajectory_file_directory + traj_file_name);
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

                        connectionManager.setClientState(sender_uid, ClientPlayState::Playing);
                        connectionManager.setClientFrame(sender_uid, 0);
                    } break;
                    case id_vis_data_pause: { connectionManager.setClientState(sender_uid, ClientPlayState::Paused); } break;
                    case id_vis_data_resume: { connectionManager.setClientState(sender_uid, ClientPlayState::Playing); } break;
                    case id_vis_data_abort: { connectionManager.setClientState(sender_uid, ClientPlayState::Stopped); } break;
                    case id_update_time_step: {
                        time_step = json_msg["time_step"].asFloat();
                        std::cout << "time step updated to " << time_step << "\n";
                        connectionManager.sendWebsocketMessageToAll(json_msg, "time-step update");
                    } break;
                    case id_update_rate_param: {
                        std::string param_name = json_msg["param_name"].asString();
                        float param_value = json_msg["param_value"].asFloat();
                        std::cout << "rate param " << param_name << " updated to " << param_value << "\n";

                        simulation.UpdateParameter(param_name, param_value);
                        connectionManager.broadcastParameterUpdate(json_msg);
                    } break;
                    case id_model_definition: {
                        aics::agentsim::Model sim_model;
                        parse_model(json_msg, sim_model);
                        print_model(sim_model);
                        simulation.SetModel(sim_model);

                        time_step = sim_model.max_time_step;
                        std::cout << "Set timestep to " << time_step << "\n";

                        connectionManager.broadcastModelDefinition(json_msg);
                    } break;
                    case id_play_cache: {
                        auto frame_no = json_msg["frame-num"].asInt();
                        std::cout << "request to play cached from frame "
                                  << frame_no << " arrived from client " << sender_uid << std::endl;

                        connectionManager.setClientFrame(sender_uid, frame_no);
                        connectionManager.setClientState(sender_uid, ClientPlayState::Playing);
                    } break;
                    default: { } break;
                    }
                }

                simThreadMessages.clear();
            }

            /**
			*	If simulation isn't running, no need to continue
			*/
            if(!connectionManager.hasActiveClient())
            {
                continue;
            }

            // Run simulation time-step
            if (simulation.IsRunningLive()) {
                simulation.RunTimeStep(time_step);
            }
            else {
                if(!simulation.HasLoadedAllFrames()) {
                    simulation.LoadNextFrame();
                }
            }

            connectionManager.sendDataToClients(simulation);
            connectionManager.advanceClients(
                simulation.GetNumFrames(),simulation.HasLoadedAllFrames());
        }
    });

    auto heartbeat_thread = std::thread([&] {
        auto no_client_timer = std::chrono::system_clock::now();

        while (isServerRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(HEART_BEAT_INTERVAL_SECONDS));
            if(connectionManager.checkNoClientTimeout())
            {
                isServerRunning = false;
                std::raise(SIGKILL);
            }

            if (heartBeatMessages.size() > 0) {
                for (std::size_t i = 0; i < heartBeatMessages.size(); ++i) {
                    Json::Value json_msg = heartBeatMessages[i].jsonMessage;

                    int msg_type = json_msg["msg_type"].asInt();
                    if(msg_type == id_heartbeat_pong)
                    {
                        auto conn_id = json_msg["conn_id"].asString();
                        connectionManager.registerHeartBeat(conn_id);
                    }
                }

                heartBeatMessages.clear();
            }

            connectionManager.removeUnresponsiveClients();
            connectionManager.pingAllClients();
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
