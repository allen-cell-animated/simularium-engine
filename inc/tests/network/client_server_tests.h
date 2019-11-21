#include "agentsim/agentsim.h"
#include "agentsim/network/cli_client.h"
#include "agentsim/network/net_message_ids.h"
#include "gtest/gtest.h"
#include <memory>

namespace aics {
namespace agentsim {
    namespace test {

        class ClientServerTests : public ::testing::Test {
        protected:
            // You can remove any or all of the following functions if its body
            // is empty.

            ClientServerTests()
            {
                // You can do set-up work for each test here.
            }

            virtual ~ClientServerTests()
            {
                // You can do clean-up work that doesn't throw exceptions here.
            }

            // If the constructor and destructor are not enough for setting up
            // and cleaning up each test, you can define the following methods:

            virtual void SetUp()
            {
                // Code here will be called immediately after the constructor (right
                // before each test).
            }

            virtual void TearDown()
            {
                // Code here will be called immediately after each test (right
                // before the destructor).
            }

            // Objects declared here can be used by all tests in the test case for Foo.
        };

        TEST_F(ClientServerTests, HundredClient)
        {
            // Disabling STD OUT until a logging library is setup
            //  otherwise, the cli clients would be noisy for this test
            std::cout.setstate(std::ios_base::failbit);

            std::atomic<bool> isRunning = true;
            float timeStep = 1e-12;
            std::string uri = "wss://localhost:9002";

            std::vector<std::shared_ptr<SimPkg>> simulators;
            std::vector<std::shared_ptr<Agent>> agents;
            Simulation simulation(simulators, agents);

            ConnectionManager connectionManager;
            connectionManager.ListenAsync();
            connectionManager.StartSimAsync(isRunning, simulation, timeStep);

            CliClient controller(uri);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            controller.Parse("start trajectory actin5-1.h5");

            std::cout << "Waiting for simulation to load ..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(10));

            std::size_t numberOfClients = 100;
            std::vector<std::shared_ptr<CliClient>> clients;
            for (std::size_t i = 0; i < numberOfClients; ++i) {
                std::shared_ptr<CliClient> cliClient(new CliClient(uri));
                std::this_thread::sleep_for(std::chrono::milliseconds(25)); // give time to connect
                cliClient->Parse("resume");
                clients.push_back(cliClient);
            }

            std::cout << "Running server for 30 seconds" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(30));

            std::cout << "Shutting down clients" << std::endl;
            for (std::size_t i = 0; i < numberOfClients; ++i) {
                clients[i]->Parse("quit");
            }

            controller.Parse("quit");
            isRunning = false;
            connectionManager.CloseServer();
            std::cout.clear();
        }

        TEST_F(ClientServerTests, DockerConnect)
        {
            // Disabling STD OUT until a logging library is setup
            //  otherwise, the cli clients would be noisy for this test
            std::cout.setstate(std::ios_base::failbit);

            std::atomic<bool> isRunning = true;
            float timeStep = 1e-12;
            std::string uri = "wss://dev-node1-agentviz-backend.cellexplore.net:9002";

            std::vector<std::shared_ptr<SimPkg>> simulators;
            std::vector<std::shared_ptr<Agent>> agents;
            Simulation simulation(simulators, agents);

            ConnectionManager connectionManager;
            connectionManager.ListenAsync();
            connectionManager.StartSimAsync(isRunning, simulation, timeStep);

            CliClient controller(uri);
            std::this_thread::sleep_for(std::chrono::seconds(1));
            controller.Parse("start trajectory actin5-1.h5");

            std::cout << "Waiting for simulation to load ..." << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(5));

            std::size_t numberOfClients = 10;
            std::vector<std::shared_ptr<CliClient>> clients;
            for (std::size_t i = 0; i < numberOfClients; ++i) {
                std::shared_ptr<CliClient> cliClient(new CliClient(uri));
                std::this_thread::sleep_for(std::chrono::milliseconds(1000)); // give time to connect
                cliClient->Parse("resume");
                clients.push_back(cliClient);
            }

            std::cout << "Running server for 30 seconds" << std::endl;
            std::this_thread::sleep_for(std::chrono::seconds(30));

            for (std::size_t i = 0; i < numberOfClients; ++i) {
                clients[i]->Parse("quit");
            }

            controller.Parse("quit");
            isRunning = false;
            connectionManager.CloseServer();
            std::cout.clear();
        }

    } // namespace test
} // namespace agentsim
} // namespace aics
