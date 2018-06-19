#include "agentsim/interactions/statechange_reaction.h"
#include "agentsim/pattern/agent_pattern.h"
#include "agentsim/agents/agent.h"
#include "agentsim/common/logger.h"

namespace aics {
namespace agentsim {

bool StateChangeReaction::RegisterReactant(AgentPattern& ap)
{
	this->m_reactant = ap;
	return true;
}

bool StateChangeReaction::IsReactant(Agent* a)
{
	return a->Matches(this->m_reactant);
}

bool StateChangeReaction::React(Agent* a)
{
	std::unordered_map<std::string, bool> ignore;
	Agent* outptr = nullptr;
	for(std::size_t i = 0; i < this->m_reactionCenters.size(); ++i)
	{
		if(!a->FindChildAgent(this->m_reactionCenters[i].a1, outptr, ignore))
		{
			PRINT_ERROR("StateChange_Reaction.cpp: attempted a reaction on a non-qualified reactant")
			return false;
		}

		ignore[outptr->GetID()] = true;
		//@TODO copy state from a2
	}

	return true;
}

bool StateChangeReaction::React(Agent* a, Agent* b)
{
	PRINT_ERROR("StateChange_Reaction.cpp: this reaction type only has one reactant")
	return false;
}

} // namespace agentsim
} // namespace aics
