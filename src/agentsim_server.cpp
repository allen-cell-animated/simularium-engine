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
    std::string senderUid;
    Json::Value jsonMessage;
};

std::vector<NetMessage> simThreadMessages;

bool use_readdy = true;
bool use_cytosim = !use_readdy;

std::string trajectory_file_directory = "trajectory/";

// Web Socket handlers
void OnMessage(websocketpp::connection_hdl hd1, server::message_ptr msg);
void OnClose(websocketpp::connection_hdl hd1);
void OnOpen(websocketpp::connection_hdl hd1);

// Arg List:
//  --no-exit  don't use the no client timeout
void ParseArguments(int argc, char* argv[]);

// Enacts web-socket commands in the sim thread
// e.g. changing parameters, time-step, starting, stopping, etc.
void HandleNetMessages(Simulation& simulation, float& timeStep);

int main(int argc, char* argv[])
{
    ParseArguments(argc, argv);

    // A synchronized variable that tells all the threads to exit
    std::atomic<bool> isServerRunning { true };

    auto websocketThread = std::thread([&] {
        auto server = connectionManager.GetServer();
        if (server != nullptr) {
            server->set_message_handler(OnMessage);
            server->set_close_handler(OnClose);
            server->set_open_handler(OnOpen);
            server->set_access_channels(websocketpp::log::alevel::none);
            server->set_error_channels(websocketpp::log::elevel::none);

            server->init_asio();
            server->listen(9002);
            server->start_accept();

            server->run();
        } else {
            std::cout << "Connection Manager has no server!" << std::endl;
            isServerRunning = false;
        }
    });

    auto simulationThread = std::thread([&] {
        float timeStep = 1e-12; // seconds

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

            connectionManager.RemoveExpiredConnections();
            connectionManager.UpdateNewConections();

            HandleNetMessages(simulation, timeStep);

            if (!connectionManager.HasActiveClient()) {
                continue;
            }

            // Run simulation time-step
            if (simulation.IsRunningLive()) {
                simulation.RunTimeStep(timeStep);
            } else {
                if (!simulation.HasLoadedAllFrames()) {
                    simulation.LoadNextFrame();
                }
            }

            std::size_t numberOfFrames = simulation.GetNumFrames();
            bool hasFinishedLoading = simulation.HasLoadedAllFrames();

            connectionManager.CheckForFinishedClients(numberOfFrames, hasFinishedLoading);
            connectionManager.SendDataToClients(simulation);
            connectionManager.AdvanceClients();
        }
    });

    auto heartbeatThread = std::thread([&] {
        while (isServerRunning) {
            std::this_thread::sleep_for(std::chrono::seconds(HEART_BEAT_INTERVAL_SECONDS));
            if (connectionManager.CheckNoClientTimeout()) {
                isServerRunning = false;
            }

            if (connectionManager.NumberOfClients() > 0) {
                connectionManager.RemoveUnresponsiveClients();
                connectionManager.PingAllClients();
            }
        }
    });

    auto ioThread = std::thread([&] {
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
    simulationThread.join();
    heartbeatThread.join();

    // These threads are detached since they block for IO
    //  under the assumption that these threads will be terminated
    //  when the process terminates
    ioThread.detach();
    websocketThread.detach();
}

void OnMessage(websocketpp::connection_hdl hd1, server::message_ptr msg)
{
    Json::CharReaderBuilder jsonReadBuilder;
    std::unique_ptr<Json::CharReader> const jsonReader(jsonReadBuilder.newCharReader());

    NetMessage nm;
    nm.senderUid = connectionManager.GetUid(hd1);
    std::string message = msg->get_payload();
    std::string errs;

    jsonReader->parse(message.c_str(), message.c_str() + message.length(),
        &(nm.jsonMessage), &errs);

    auto msgType = nm.jsonMessage["msg_type"].asInt();

    if (msgType >= 0 && msgType < WebRequestNames.size()) {
        std::cout << "[" << nm.senderUid << "] Web socket message arrived: " << WebRequestNames[msgType] << std::endl;

        if (msgType == WebRequestTypes::id_heartbeat_pong) {
            connectionManager.RegisterHeartBeat(nm.senderUid);
        } else {
            simThreadMessages.push_back(nm);
        }
    } else {
        std::cout << "Websocket message arrived: UNRECOGNIZED of type " << msgType << std::endl;
    }
}

void OnClose(websocketpp::connection_hdl hd1)
{
    connectionManager.MarkConnectionExpired(hd1);
}

void OnOpen(websocketpp::connection_hdl hd1)
{
    connectionManager.AddConnection(hd1);
}

void ParseArguments(int argc, char* argv[])
{
    // The first argument is the program running
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg.compare("--no-exit") == 0) {
            std::cout << "Argument : --no-exit; ignoring no-client timeout" << std::endl;
            connectionManager.SetNoTimeoutArg(true);
        } else {
            std::cout << "Unrecognized argument " << arg << " ignored" << std::endl;
        }
    }
}

void HandleNetMessages(Simulation& simulation, float& timeStep)
{
    // handle net messages
    if (simThreadMessages.size() > 0) {
        for (std::size_t i = 0; i < simThreadMessages.size(); ++i) {
            std::string senderUid = simThreadMessages[i].senderUid;
            Json::Value jsonMsg = simThreadMessages[i].jsonMessage;

            int msg_type = jsonMsg["msg_type"].asInt();
            switch (msg_type) {
            case WebRequestTypes::id_vis_data_request: {
                // If a simulation is already in progress, don't allow a new client to
                //	change the simulation, unless there is only one client connected
                if (!connectionManager.HasActiveClient()
                    || connectionManager.NumberOfClients() == 1) {
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

                connectionManager.SetClientState(senderUid, ClientPlayState::Playing);
                connectionManager.SetClientFrame(senderUid, 0);
            } break;
            case WebRequestTypes::id_vis_data_pause: {
                connectionManager.SetClientState(senderUid, ClientPlayState::Paused);
            } break;
            case WebRequestTypes::id_vis_data_resume: {
                connectionManager.SetClientState(senderUid, ClientPlayState::Playing);
            } break;
            case WebRequestTypes::id_vis_data_abort: {
                connectionManager.SetClientState(senderUid, ClientPlayState::Stopped);
            } break;
            case WebRequestTypes::id_update_time_step: {
                timeStep = jsonMsg["time_step"].asFloat();
                std::cout << "time step updated to " << timeStep << "\n";
                connectionManager.SendWebsocketMessageToAll(jsonMsg, "time-step update");
            } break;
            case WebRequestTypes::id_update_rate_param: {
                std::string paramName = jsonMsg["param_name"].asString();
                float paramValue = jsonMsg["param_value"].asFloat();
                std::cout << "rate param " << paramName << " updated to " << paramValue << "\n";

                simulation.UpdateParameter(paramName, paramValue);
                connectionManager.BroadcastParameterUpdate(jsonMsg);
            } break;
            case WebRequestTypes::id_model_definition: {
                aics::agentsim::Model sim_model;
                parse_model(jsonMsg, sim_model);
                print_model(sim_model);
                simulation.SetModel(sim_model);

                timeStep = sim_model.max_time_step;
                std::cout << "Set timestep to " << timeStep << "\n";

                connectionManager.BroadcastModelDefinition(jsonMsg);
            } break;
            case WebRequestTypes::id_play_cache: {
                auto frameNumber = jsonMsg["frame-num"].asInt();
                std::cout << "request to play cached from frame "
                          << frameNumber << " arrived from client " << senderUid << std::endl;

                connectionManager.SetClientFrame(senderUid, frameNumber);
                connectionManager.SetClientState(senderUid, ClientPlayState::Playing);
            } break;
            default: {
            } break;
            }
        }

        simThreadMessages.clear();
    }
}
