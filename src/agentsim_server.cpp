#include <chrono>
#include <iostream>
#include <mutex>
#include <thread>
#include <vector>
#include <cstdlib>

#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <json/json.h>
#include <websocketpp/config/asio_no_tls.hpp>
#include <websocketpp/server.hpp>

#include "simularium/simularium.h"

using namespace aics::simularium;

// Arg List:
//  --no-exit  don't use the no client timeout
void ParseArguments(
    int argc,
    char* argv[],
    ConnectionManager& connectionManager);

int main(int argc, char* argv[])
{
    if(!std::getenv("TLS_CERT_PATH") || !std::getenv("TLS_KEY_PATH"))
    {
        std::cout << "Setting up local TLS environment (dev)" << std::endl;
        setenv("TLS_PASSWORD","test", false);
        setenv("TLS_CERT_PATH","localhost.pem", false);
        setenv("TLS_KEY_PATH","localhost-key.pem", false);
    }

    ConnectionManager connectionManager;
    ParseArguments(argc, argv, connectionManager);

    // A synchronized variable that tells all the threads to exit
    std::atomic<bool> isServerRunning { true };

    float timeStep = 1e-12; // seconds

    // Simulation setup
    std::vector<std::shared_ptr<SimPkg>> simulators;

    simulators.push_back(std::shared_ptr<SimPkg>(new CytosimPkg()));
    simulators.push_back(std::shared_ptr<SimPkg>(new ReaDDyPkg()));

    std::vector<std::shared_ptr<Agent>> agents;
    Simulation simulation(simulators, agents);

    connectionManager.ListenAsync();
    connectionManager.StartSimAsync(isServerRunning, simulation, timeStep);
    connectionManager.StartHeartbeatAsync(isServerRunning);
    connectionManager.StartFileIOAsync(isServerRunning, simulation);

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
    connectionManager.CloseServer();

    // The following thread(s) are detached since they block for IO
    //  under the assumption that these threads will be terminated
    //  when the process terminates
    ioThread.detach();
}

void ParseArguments(int argc, char* argv[], ConnectionManager& connectionManager)
{
    // The first argument is the program running
    for (int i = 1; i < argc; i++) {
        std::string arg(argv[i]);
        if (arg.compare("--no-exit") == 0) {
            std::cout << "Argument : --no-exit; ignoring no-client timeout" << std::endl;
            connectionManager.SetNoTimeoutArg(true);
        } else if (arg.compare("--no-upload") == 0) {
            std::cout << "Argument : --no-upload; caches will not be uploaded to S3" << std::endl;
            connectionManager.SetNoUploadArg(true);
        } else if (arg.compare("--force-init") == 0) {
            std::cout << "Argument : --force-init; no caches will be downloaded from S3" << std::endl;
            connectionManager.SetForceInitArg(true);
        } else if (arg.compare("--dev") == 0) {
            std::cout << "Argument: --dev; setting --no-exit --no-upload --force-init" << std::endl;
            connectionManager.SetNoTimeoutArg(true);
            connectionManager.SetNoUploadArg(true);
            connectionManager.SetForceInitArg(true);
        } else {
            std::cout << "Unrecognized argument " << arg << " ignored" << std::endl;
        }
    }
}
