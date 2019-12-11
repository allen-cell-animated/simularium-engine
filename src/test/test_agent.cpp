#include "test/agents/test_agent.h"
#include "agentsim/agents/agent.h"
#include <time.h>

namespace aics {
namespace agentsim {
namespace test {

    TEST_F(AgentTest, BasicParenting)
    {
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

        Eigen::Vector3d v(0, 150, 0);
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
