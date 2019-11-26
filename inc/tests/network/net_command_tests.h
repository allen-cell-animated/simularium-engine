#include "agentsim/agentsim.h"
#include "agentsim/network/cli_client.h"
#include "agentsim/network/net_message_ids.h"
#include "gtest/gtest.h"
#include <memory>

namespace aics {
namespace agentsim {
    namespace test {

        class NetCommandTests : public ::testing::Test {
        protected:
            // You can remove any or all of the following functions if its body
            // is empty.

            NetCommandTests()
            {
                // You can do set-up work for each test here.
            }

            virtual ~NetCommandTests()
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

        TEST_F(NetCommandTests, PlayPause)
        {
            // Disabling STD OUT until a logging library is setup
            //  otherwise, the cli clients would be noisy for this test
            std::cout.setstate(std::ios_base::failbit);

            // Wait specified amount of time
            // send play/pause/resume command
            std::size_t numCommands = 100;

            std::atomic<bool> isRunning = true;
            float timeStep = 1e-12;
            std::string uri = "wss://localhost:9002";
            std::string simFileName = "actin19.h5";

            std::vector<std::shared_ptr<SimPkg>> simulators;
            std::vector<std::shared_ptr<Agent>> agents;
            Simulation simulation(simulators, agents);

            ConnectionManager connectionManager;
            connectionManager.ListenAsync();
            connectionManager.StartSimAsync(isRunning, simulation, timeStep);

            CliClient controller(uri);
            controller.Parse("start trajectory " + simFileName);

            std::vector<std::string> commands = {
                "pause",
                "resume",
                "stop",
            };

            for(std::size_t i = 0; i < numCommands; i++)
            {
                controller.Parse(commands.at(rand() % commands.size()));
                std::this_thread::sleep_for(std::chrono::milliseconds(5));
            }

            controller.Parse("quit");
            isRunning = false;
            connectionManager.CloseServer();
            std::cout.clear();
        }

    } // namespace test
} // namespace agentsim
} // namespace aics
