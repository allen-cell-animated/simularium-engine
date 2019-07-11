#include "gtest/gtest.h"
#include "agentsim/network/cli_client.h"
#include "agentsim/network/net_message_ids.h"

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

        TEST_F(ClientServerTests, SingleClient)
        {

        }

        TEST_F(ClientServerTests, HundredClient)
        {

        }

    } // namespace test
} // namespace agentsim
} // namespace aics
