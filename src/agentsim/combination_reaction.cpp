#include "agentsim/interactions/combination_reaction.h"
#include "agentsim/pattern/agent_pattern.h"
#include "agentsim/agents/agent.h"
#include "agentsim/common/logger.h"

namespace aics {
namespace agentsim {

bool CombinationReaction::RegisterReactant(AgentPattern ap)
{
	if(this->m_reactants.size() < 2)
	{
		this->m_reactants.push_back(ap);
		return true;
	}

	PRINT_WARNING("Combination_Reaction.cpp: this reaction already has two reactants.\n")
	return false;
}

bool CombinationReaction::IsReactant(Agent* a)
{
	return a->Matches(this->m_reactants[0]) ||
		a->Matches(this->m_reactants[1]);
}

bool CombinationReaction::RegisterBondChange(ReactionBondChange rbc)
{
	this->m_bondChanges = rbc;
	return true;
}

bool CombinationReaction::React(std::shared_ptr<Agent> a, std::shared_ptr<Agent> b)
{
	if(a.get() == nullptr || b.get() == nullptr)
	{
		PRINT_ERROR("Combination_Reaction.cpp: this reaction only accepts two reactants.\n")
		return false;
	}

	if(this->m_reactants.size() != 2)
	{
		PRINT_ERROR("Combination_Reaction.cpp: this reaction does not have two reactant patterns defined.\n")
		return false;
	}

	// ensure agent a is m_reactants[0]
	if(a->Matches(m_reactants[1]))
	{
		a.swap(b);
	}

	std::vector<Agent*> reactants;
	for(std::size_t i = 0; i < this->m_bondChanges.reactant_patterns.size(); ++i)
	{
		AgentPattern ap = this->m_bondChanges.reactant_patterns[i];
		Agent* tosearch = i < this->m_bondChanges.end_reactant_1 ? a.get() : b.get();
		Agent* outptr = nullptr;
		if(!tosearch->FindSubAgent(ap, outptr))
		{
			PRINT_ERROR("Combination_Reaction.cpp: could not find a sub agent needed for bond formation.\n")
			return false;
		}

		reactants.push_back(outptr);
	}

	for(std::size_t i = 0; i < this->m_bondChanges.bond_indices.size(); i+= 2)
	{
		Eigen::Vector2i v1 = this->m_bondChanges.bond_indices[i];
		Eigen::Vector2i v2 = this->m_bondChanges.bond_indices[i+1];

		std::shared_ptr<Agent> ab1 = reactants[v1[0]]->GetChildAgent(v1[1]);
		std::shared_ptr<Agent> ab2 = reactants[v2[0]]->GetChildAgent(v2[1]);
		ab1->AddBoundPartner(ab2);
		ab2->AddBoundPartner(ab1);
	}

	int stda = a->GetSubTreeDepth();
	int stdb = b->GetSubTreeDepth();

	if(stda == 2 && stdb == 2)
	{
		int i = 0;
		while(b->GetChildAgent(i))
		{
			a->AddChildAgent(b->GetChildAgent(i));
		}

		this->m_lastProduct = a;
	}

	if(stda == 2 && stdb == 1)
	{
		a->AddChildAgent(b);
		this->m_lastProduct = a;
	}

	if(stda == 1 && stdb == 2)
	{
		b->AddChildAgent(a);
		this->m_lastProduct = b;
	}

	if(stda == 1 && stdb == 1)
	{
		std::shared_ptr<Agent> product;
		product->AddChildAgent(a);
		product->AddChildAgent(b);
		this->m_lastProduct = product;
	}

	return false;
}

std::shared_ptr<Agent> CombinationReaction::GetProduct()
{
	return this->m_lastProduct;
}

} // namespace agentsim
} // namespace aics
