#include "gtest/gtest.h"
#include "agentsim/interactions/combination_reaction.h"
#include <time.h>
#include "agentsim/agents/agent.h"
#include "agentsim/pattern/agent_pattern.h"

namespace aics {
namespace agentsim {
namespace test {

std::shared_ptr<Agent> create_actin_agent()
{
	std::shared_ptr<Agent> actin(new Agent());
	std::shared_ptr<Agent> pointed(new Agent());
	std::shared_ptr<Agent> side1(new Agent());
	std::shared_ptr<Agent> side2(new Agent());
	std::shared_ptr<Agent> barbed(new Agent());
	std::shared_ptr<Agent> nuc(new Agent());

	pointed->SetName("pointed");
	side1->SetName("side1");
	side2->SetName("side2");
	barbed->SetName("barbed");
	nuc->SetName("nuc");
	nuc->SetState("ADP/Pi");

	actin->AddChildAgent(pointed);
	actin->AddChildAgent(side1);
	actin->AddChildAgent(side2);
	actin->AddChildAgent(barbed);
	actin->AddChildAgent(nuc);
	return actin;
}

AgentPattern create_actin_agent_pattern()
{
	AgentPattern actinap;
	AgentPattern pointedap;
	AgentPattern side1ap;
	AgentPattern side2ap;
	AgentPattern barbedap;
	AgentPattern nucap;

	pointedap.Name = "pointed";
	side1ap.Name = "side1";
	side2ap.Name = "side2";
	barbedap.Name = "barbed";
	nucap.Name = "nuc";
	nucap.State = "ADP/Pi";
	actinap.ChildAgents.push_back(pointedap);
	actinap.ChildAgents.push_back(side1ap);
	actinap.ChildAgents.push_back(side2ap);
	actinap.ChildAgents.push_back(barbedap);
	actinap.ChildAgents.push_back(nucap);
	return actinap;
}

std::shared_ptr<CombinationReaction> create_actin_nucleation_rx()
{
	std::shared_ptr<CombinationReaction> rx;
	return rx;
}

class CombinationReactionTest : public ::testing::Test
{
	protected:
	// You can remove any or all of the following functions if its body
	// is empty.

	CombinationReactionTest() {
		// You can do set-up work for each test here.
	}

	virtual ~CombinationReactionTest() {
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

TEST_F(CombinationReactionTest, ActinNucleation)
{
	std::shared_ptr<Agent> actin1, actin2;
	actin1 = create_actin_agent();
	actin2 = create_actin_agent();

	AgentPattern ap = create_actin_agent_pattern();
	ReactionBondChange rb;
	rb.reactant_patterns.push_back(ap);
	rb.reactant_patterns.push_back(ap);
	rb.end_reactant_1 = 1;

	rb.bond_indices.push_back(Eigen::Vector2i(0,0));
	rb.bond_indices.push_back(Eigen::Vector2i(1,3));
}

TEST_F(CombinationReactionTest, ActinTrimer)
{

}

TEST_F(CombinationReactionTest, BarbedEndPolymerization)
{

}

} // namespace test
} // namespace agentsim
} // namespace aics
