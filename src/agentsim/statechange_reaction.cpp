#include "agentsim/interactions/statechange_reaction.h"
#include "agentsim/pattern/agent_pattern.h"
#include "agentsim/agents/agent.h"

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

} // namespace agentsim
} // namespace aics
