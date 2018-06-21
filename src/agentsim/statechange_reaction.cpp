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

void StateChangeReaction::RegisterStateChange(ReactionStateChange rsc)
{
	this->m_stateChanges.push_back(rsc);
}

bool StateChangeReaction::IsReactant(Agent* a)
{
	Agent* outptr = nullptr;
	bool isReactant = a->FindSubAgent(this->m_reactant, outptr);
	return isReactant;
}

bool StateChangeReaction::React(Agent* a)
{
	for(std::size_t i = 0; i < this->m_stateChanges.size(); ++i)
	{
		Agent* outptr = nullptr;
		if(!(a->FindSubAgent(this->m_stateChanges[i].before, outptr)))
		{
			PRINT_ERROR("StateChange_Reaction.cpp: could not find a sub agent for a reaction center.\n")
			return false;
		}
		outptr->CopyState(
			this->m_stateChanges[i].before,
			this->m_stateChanges[i].after);
	}
	return true;
}

bool StateChangeReaction::React(Agent* a, Agent* b)
{
	PRINT_ERROR("StateChange_Reaction.cpp: this reaction type only has one reactant.\n")
	return false;
}

} // namespace agentsim
} // namespace aics
