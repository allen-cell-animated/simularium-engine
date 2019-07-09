
#define ASIO_STANDALONE
#include <asio/asio.hpp>
#include <websocketpp/client.hpp>
#include <websocketpp/config/asio_no_tls_client.hpp>

#include <fstream>
#include <iostream>
#include <json/json.h>
#include <sys/stat.h>

#include "agentsim/network/net_message_ids.h"
#include "agentsim/network/cli_client.h"
using namespace aics::agentsim;

int main(int argc, char* argv[])
{
    CliClient cliClient;

    std::string uri = "ws://localhost:9002";
    std::atomic<bool> isClientRunning { true };

    auto webSocketThread = std::thread([&] {
        cliClient.Connect(uri);
    });

    std::string input;
    std::cout << "Enter 'help' for commands\n";
    std::cout << "Enter 'quit' to exit\n";

    while (isClientRunning && std::getline(std::cin, input, '\n')) {
        if (input == "quit") {
            isClientRunning = false;
        }

        cliClient.Parse(input);
    }

    std::cout << "Exiting Client... \n";
    webSocketThread.detach();
}
