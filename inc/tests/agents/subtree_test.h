#include "gtest/gtest.h"
#include "agentsim/interactions/statechange_reaction.h"
#include <time.h>
#include "agentsim/agents/agent.h"
#include "agentsim/pattern/agent_pattern.h"
#include <iostream>

namespace aics {
namespace agentsim {
namespace test {

class AgentSubtreeTest : public ::testing::Test
{
	protected:
	// You can remove any or all of the following functions if its body
	// is empty.

	AgentSubtreeTest() {
		// You can do set-up work for each test here.
		parent_ap.Name = "parent";
		child1_ap.Name = "child1";
		child2_ap.Name = "child2";
		subchild_1_1_ap.Name = "sc11";
		subchild_1_2_ap.Name = "sc12";
		subchild_2_1_ap.Name = "sc21";
		subchild_2_2_ap.Name = "sc22";

		child1_ap.ChildAgents.push_back(subchild_1_1_ap);
		child1_ap.ChildAgents.push_back(subchild_1_2_ap);
		child2_ap.ChildAgents.push_back(subchild_2_1_ap);
		child2_ap.ChildAgents.push_back(subchild_2_2_ap);

		parent_ap.ChildAgents.push_back(child1_ap);
		parent_ap.ChildAgents.push_back(child2_ap);

		std::shared_ptr<Agent> parent(new Agent());
		std::shared_ptr<Agent> child1(new Agent());
		std::shared_ptr<Agent> child2(new Agent());
		std::shared_ptr<Agent> subchild_1_1(new Agent());
		std::shared_ptr<Agent> subchild_1_2(new Agent());
		std::shared_ptr<Agent> subchild_2_1(new Agent());
		std::shared_ptr<Agent> subchild_2_2(new Agent());

		parent->SetName("parent");
		child1->SetName("child1");
		child2->SetName("child2");
		subchild_1_1->SetName("sc11");
		subchild_1_2->SetName("sc12");
		subchild_2_1->SetName("sc21");
		subchild_2_2->SetName("sc22");

		child1->AddChildAgent(subchild_1_1);
		child1->AddChildAgent(subchild_1_2);
		child2->AddChildAgent(subchild_2_1);
		child2->AddChildAgent(subchild_2_2);
		parent->AddChildAgent(child1);
		parent->AddChildAgent(child2);

		parent_agent = parent;
	}

	virtual ~AgentSubtreeTest() {
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

	std::shared_ptr<Agent> parent_agent;

	// Objects declared here can be used by all tests in the test case for Foo.
	AgentPattern parent_ap;
	AgentPattern child1_ap;
	AgentPattern child2_ap;
	AgentPattern subchild_1_1_ap;
	AgentPattern subchild_1_2_ap;
	AgentPattern subchild_2_1_ap;
	AgentPattern subchild_2_2_ap;
};

TEST_F(AgentSubtreeTest, FindSubAgent)
{
	Agent* outptr = nullptr;
	ASSERT_TRUE(parent_agent->FindSubAgent(parent_ap,outptr));
	ASSERT_TRUE(outptr->GetName() == "parent");

	ASSERT_TRUE(parent_agent->FindSubAgent(child1_ap,outptr));
	ASSERT_TRUE(outptr->GetName() == "child1");

	ASSERT_TRUE(parent_agent->FindSubAgent(child2_ap,outptr));
	ASSERT_TRUE(outptr->GetName() == "child2");

	ASSERT_TRUE(parent_agent->FindSubAgent(subchild_1_2_ap,outptr));
	ASSERT_TRUE(outptr->GetName() == "sc12");

	ASSERT_TRUE(parent_agent->FindSubAgent(subchild_2_1_ap,outptr));
	ASSERT_TRUE(outptr->GetName() == "sc21");
}

} // namespace test
} // namespace agentsim
} // namespace aics
