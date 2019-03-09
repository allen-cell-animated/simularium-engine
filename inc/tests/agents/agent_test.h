#include "gtest/gtest.h"
#include <time.h>
#include "agentsim/agents/agent.h"

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

TEST_F(AgentTest, BasicParenting) {
	std::shared_ptr<Agent> a1(new Agent());
	std::shared_ptr<Agent> a2(new Agent());

	Eigen::Vector3d v;
	v << 1, 5, 7;
	a1->SetLocation(v);
	a1->AddChildAgent(a2);

	EXPECT_EQ(a2->GetGlobalTransform(), a1->GetGlobalTransform());
	a2->SetLocation(v);
	EXPECT_NE(a2->GetGlobalTransform(), a1->GetGlobalTransform());
	EXPECT_EQ(a2->GetGlobalTransform(), a1->GetGlobalTransform() * a2->GetTransform());
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

TEST_F(AgentTest, CannotParentToSelf)
{
	std::shared_ptr<Agent> a1(new Agent());
	ASSERT_FALSE(a1->AddChildAgent(a1));
}

} // namespace test
} // namespace agentsim
} // namespace aics
