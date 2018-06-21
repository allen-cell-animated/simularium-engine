#include "agentsim/interactions/statechange_reaction.h"
#include "agentsim/pattern/agent_pattern.h"
#include "agentsim/agents/agent.h"
#include "agentsim/common/logger.h"

namespace aics {
namespace agentsim {

bool StateChangeReaction::RegisterReactant(AgentPattern ap)
{
	this->m_reactant = ap;
	return true;
}

void StateChangeReaction::RegisterStateChange(ReactionStateChange rsc)
{
	this->m_stateChanges = rsc;
}

bool StateChangeReaction::IsReactant(Agent* a)
{
	Agent* outptr = nullptr;
	bool isReactant = a->FindSubAgent(this->m_reactant, outptr);
	return isReactant;
}

bool StateChangeReaction::React(std::shared_ptr<Agent> a, std::shared_ptr<Agent> b)
{
	if(b.get() != nullptr)
	{
		PRINT_WARNING("StateChange_Reaction.cpp: this reaction only accepts one reactant.\n")
		return false;
	}

	Agent* outptr = nullptr;
	if(!(a->FindSubAgent(this->m_stateChanges.before, outptr)))
	{
		PRINT_ERROR("StateChange_Reaction.cpp: could not find a sub agent for a reaction center.\n")
		return false;
	}
	outptr->CopyState(
		this->m_stateChanges.before,
		this->m_stateChanges.after);

	return true;
}

} // namespace agentsim
} // namespace aics
