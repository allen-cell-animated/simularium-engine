#include "gtest/gtest.h"
#include "agentsim/interactions/combination_reaction.h"
#include <time.h>
#include "agentsim/agents/agent.h"
#include "agentsim/pattern/agent_pattern.h"

namespace aics {
namespace agentsim {
namespace test {

std::shared_ptr<Agent> create_actin_monomer()
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
	actin->SetName("actin");
	return actin;
}

std::shared_ptr<Agent> create_actin_dimer()
{
	std::shared_ptr<Agent> actin1 = create_actin_monomer();
	std::shared_ptr<Agent> actin2 = create_actin_monomer();
	std::shared_ptr<Agent> dimer(new Agent());

	std::shared_ptr<Agent> pointed = actin1->GetChildAgent(0);
	std::shared_ptr<Agent> barbed = actin2->GetChildAgent(3);

	actin1->SetName("pointed actin");
	actin2->SetName("barbed actin");

	pointed->SetName("bound pointed");
	barbed->SetName("bound barbed");

	pointed->AddBoundPartner(barbed);
	barbed->AddBoundPartner(pointed);

	actin1->AddBoundPartner(actin2);
	actin2->AddBoundPartner(actin1);

	dimer->AddChildAgent(actin1);
	dimer->AddChildAgent(actin2);

	return dimer;
}

AgentPattern create_actin_monomer_pattern()
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
	actinap.Name = "actin";
	return actinap;
}

AgentPattern create_actin_dimer_pattern()
{
	AgentPattern dimerap;
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

	actinap.Name = "pointed actin";
	dimerap.ChildAgents.push_back(actinap);
	actinap.Name = "barbed actin";
	dimerap.ChildAgents.push_back(actinap);

	AgentPattern* pointedptr = &(dimerap.ChildAgents[0].ChildAgents[0]);
	AgentPattern* barbedptr = &(dimerap.ChildAgents[1].ChildAgents[3]);

	pointedptr->Name = "bound pointed";
	barbedptr->Name = "bound barbed";

	pointedptr->BoundPartners.push_back(*(barbedptr));
	barbedptr->BoundPartners.push_back(*(pointedptr));
	//pointedptr->BoundPartners[0].BoundPartners.push_back(*(pointedptr));

	AgentPattern* actin_1_ptr = &(dimerap.ChildAgents[0]);
	AgentPattern* actin_2_ptr = &(dimerap.ChildAgents[1]);

	actin_1_ptr->BoundPartners.push_back(*(actin_2_ptr));
	actin_2_ptr->BoundPartners.push_back(*(actin_1_ptr));

	return dimerap;
}

class CombinationReactionTest : public ::testing::Test
{
	protected:
	// You can remove any or all of the following functions if its body
	// is empty.create_actin_agent

	CombinationReactionTest() {
		// You can do set-up work for each test here.
		boundBarbedap.Name = "barbed";
		boundBarbedap.IsWildCardBound = true;

		boundPointedap.Name = "pointed";
		boundPointedap.IsWildCardBound = true;
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
	AgentPattern boundPointedap;
	AgentPattern boundBarbedap;
};

TEST_F(CombinationReactionTest, ActinNucleation)
{
	std::shared_ptr<Agent> actin1, actin2;
	actin1 = create_actin_monomer();
	actin2 = create_actin_monomer();

	AgentPattern ap = create_actin_monomer_pattern();
	ReactionBondChange rb;
	rb.reactant_patterns.push_back(ap);
	rb.reactant_patterns.push_back(ap);
	rb.end_reactant_1 = 1;

	rb.bond_indices.push_back(Eigen::Vector2i(0,0));
	rb.bond_indices.push_back(Eigen::Vector2i(1,3));

	std::shared_ptr<CombinationReaction> rx(new CombinationReaction());
	rx->RegisterBondChange(rb);
	rx->RegisterReactant(ap);
	rx->RegisterReactant(ap);

	ASSERT_TRUE(rx->React(actin1, actin2));
	std::shared_ptr<Agent> product = rx->GetProduct();
	Agent* outptr = nullptr;
	ASSERT_TRUE(product->FindSubAgent(boundPointedap, outptr));
	ASSERT_TRUE(product->FindSubAgent(boundBarbedap, outptr));
}

TEST_F(CombinationReactionTest, ActinTrimer)
{
	std::shared_ptr<Agent> monomer, dimer;
	monomer = create_actin_monomer();
	dimer = create_actin_dimer();

	AgentPattern map, map_bound_pointed, map_bound_barbed, dap;
	map = create_actin_monomer_pattern();
	map_bound_barbed = create_actin_monomer_pattern();
	map_bound_pointed = create_actin_monomer_pattern();
	dap = create_actin_dimer_pattern();

	AgentPattern* pointedptr = &(map_bound_pointed.ChildAgents[0]);
	AgentPattern* barbedptr = &(map_bound_barbed.ChildAgents[3]);

	pointedptr->BoundPartners.push_back(*(barbedptr));
	barbedptr->BoundPartners.push_back(*(pointedptr));

	//ASSERT_TRUE(monomer->Matches(map));
	//ASSERT_TRUE(dimer->Matches(dap));

	ReactionBondChange rb;
	rb.reactant_patterns.push_back(map);
	rb.reactant_patterns.push_back(map_bound_pointed);
	rb.reactant_patterns.push_back(map_bound_barbed);
	rb.end_reactant_1 = 2;

	// bind #1(side2) to #2(side1)
	rb.bond_indices.push_back(Eigen::Vector2i(0,2));
	rb.bond_indices.push_back(Eigen::Vector2i(1,1));

	// bind #1(side1) to #3(side2)
	rb.bond_indices.push_back(Eigen::Vector2i(0,1));
	rb.bond_indices.push_back(Eigen::Vector2i(2,2));

	std::shared_ptr<CombinationReaction> rx(new CombinationReaction());
	rx->RegisterBondChange(rb);
	rx->RegisterReactant(map);
	rx->RegisterReactant(dap);

	ASSERT_TRUE(rx->IsReactant(monomer.get()));
	ASSERT_TRUE(rx->IsReactant(dimer.get()));
	ASSERT_TRUE(rx->React(monomer, dimer));
}

TEST_F(CombinationReactionTest, BarbedEndPolymerization)
{

}

} // namespace test
} // namespace agentsim
} // namespace aics
