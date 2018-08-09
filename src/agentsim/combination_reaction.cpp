#include "agentsim/interactions/combination_reaction.h"
#include "agentsim/pattern/agent_pattern.h"
#include "agentsim/agents/agent.h"
#include "agentsim/common/logger.h"
#include <unordered_map>

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
	std::unordered_map<std::size_t, std::shared_ptr<Agent>> reactantChildren;
	for(std::size_t i = 0; i < this->m_bondChanges.reactant_patterns.size(); ++i)
	{
		std::size_t r1_start = 0;
		std::size_t r2_start = this->m_bondChanges.end_reactant_1;
		AgentPattern ap = this->m_bondChanges.reactant_patterns[i];
		Agent* outptr = nullptr;

		/*
		*	Assumption: reactions happen between two reactants
		*	Assumption:	multiple level-1 agent reactants must be bound
		*	and owned by a level-2 agent
		*
		*	Find the first level-1 agent by searching the level-2 agent
		*	Find subsequent agents by search the first agent's bound partners
		*/
		if(i == r1_start || i == r2_start)
		{
			Agent* tosearch = i < r2_start ? a.get() : b.get();
			if(!tosearch->FindSubAgent(ap, outptr))
			{
				PRINT_ERROR("Combination_Reaction.cpp: could not find a sub agent needed for bond formation.\n")
				return false;
			}
		}
		else
		{
			Agent* tosearch = i < r2_start ? reactants[r1_start] : reactants[r2_start];
			if(!tosearch->FindBoundPartner(ap, outptr))
			{
				PRINT_ERROR("Combination_Reaction.cpp: could not find a bound sub agent needed for bond formation.\n")
				return false;
			}
		}

		reactants.push_back(outptr);
		for(std::size_t j = 0; j < ap.ChildAgents.size(); ++j)
		{
			std::shared_ptr<Agent> childoutptr = nullptr;
			if(!outptr->FindChildAgent(ap.ChildAgents[j], childoutptr))
			{
				PRINT_ERROR("Combination_Reaction.cpp: could not find a sub agent child needed for bond formation.\n")
				return false;
			}

			// assuming < 100 child agents
			reactantChildren[100 * i + j] = childoutptr;
		}
	}

	for(std::size_t i = 0; i < this->m_bondChanges.bond_indices.size(); i+= 2)
	{
		Eigen::Vector2i v1 = this->m_bondChanges.bond_indices[i];
		Eigen::Vector2i v2 = this->m_bondChanges.bond_indices[i+1];

		std::shared_ptr<Agent> ab1 = reactantChildren[v1[0] * 100 + v1[1]];
		std::shared_ptr<Agent> ab2 = reactantChildren[v2[0] * 100 + v2[1]];;
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
			++i;
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
		std::shared_ptr<Agent> product(new Agent());
		product->AddChildAgent(a);
		product->AddChildAgent(b);
		this->m_lastProduct = product;
	}

	this->m_lastProduct->SetName("Macromolecule");
	return true;
}

std::shared_ptr<Agent> CombinationReaction::GetProduct()
{
	return this->m_lastProduct;
}

std::string CombinationReaction::GetKey()
{
	return this->m_reactants[0].Name + this->m_reactants[1].Name;
}

} // namespace agentsim
} // namespace aics
