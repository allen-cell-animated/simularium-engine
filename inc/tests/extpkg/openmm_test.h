#include "gtest/gtest.h"
#include "OpenMM.h"

namespace aics {
namespace agentsim {
namespace test {

class OpenMMTest : public ::testing::Test
{
		protected:
		// You can remove any or all of the following functions if its body
		// is empty.

		OpenMMTest() {
			// You can do set-up work for each test here.
		}

		virtual ~OpenMMTest() {
			// You can do clean-up work that doesn't throw exceptions here.
		}

		// If the constructor and destructor are not enough for setting up
		// and cleaning up each test, you can define the following methods:

		virtual void SetUp() {
			// Code here will be called immediately after the constructor (right
			// before each test).
		}

		virtual void TearDown() {
			// Code here will be called immediately after each test (right
			// before the destructor).
		}

		// Objects declared here can be used by all tests in the test case for Foo.
};

TEST_F(OpenMMTest, ConstructorSetLocation) {
	OpenMM::System system;
  EXPECT_EQ(0, 0);
}

} // namespace test
} // namespace agentsim
} // namespace aics
