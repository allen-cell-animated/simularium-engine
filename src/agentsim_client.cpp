#include "agentsim/network/cli_client.h"
#include "agentsim/network/net_message_ids.h"
using namespace aics::agentsim;

int main(int argc, char* argv[])
{
    CliClient cliClient("ws://localhost:9002");

    std::atomic<bool> isClientRunning { true };

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
}
