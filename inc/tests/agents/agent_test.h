#include "gtest/gtest.h"
#include "agentsim/agents/agent.h"
#include <time.h>

namespace aics {
namespace agentsim {
namespace test {

class AgentTest : public ::testing::Test
{
		protected:
		// You can remove any or all of the following functions if its body
		// is empty.

		AgentTest() {
			// You can do set-up work for each test here.
		}

		virtual ~AgentTest() {
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

TEST_F(AgentTest, ConstructorSetLocation) {
  Agent a;

	Eigen::Vector3d v;
	v << 0, 0, 0;

  EXPECT_EQ(v, a.GetLocation());
}

TEST_F(AgentTest, DoesNotInteractWithSelf)
{
		Agent a1;
		ASSERT_FALSE(a1.CanInteractWith(a1));
}


TEST_F(AgentTest, InteractionDistCheck)
{
		Agent a1, a2;

		a1.SetInteractionDistance(100);
		a2.SetInteractionDistance(100);

		Eigen::Vector3d v(0,150,0);
		a2.SetLocation(v);

		ASSERT_TRUE(a1.CanInteractWith(a2));
}

TEST_F(AgentTest, InteractionDistCheckFail)
{
		Agent a1, a2;

		a1.SetInteractionDistance(100);
		a2.SetInteractionDistance(100);

		Eigen::Vector3d v;

		v << 0, 300, 0;
		a2.SetLocation(v);

		ASSERT_FALSE(a1.CanInteractWith(a2));
}

TEST_F(AgentTest, CollisionRadius)
{
		Agent a1, a2;
		a1.SetCollisionRadius(50);
		a2.SetCollisionRadius(50);

		Eigen::Vector3d v;
		v << 0, 98, 0;
		a2.SetLocation(v);

		ASSERT_TRUE(a1.IsCollidingWith(a2));
}

} // namespace test
} // namespace agentsim
} // namespace aics
