#ifndef AICS_UNIMOLECULAR_REACTION_H
#define AICS_UNIMOLECULAR_REACTION_H

#include "agentsim/interactions/reaction.h"
#include <vector>
#include "agentsim/interactions/reaction_center.h"
#include "agentsim/pattern/agent_pattern.h"

namespace aics {
namespace agentsim {

class StateChangeReaction : public Reaction
{
public:
	virtual bool RegisterReactant(AgentPattern& ap) override;
	virtual bool IsReactant(Agent* a) override;
	virtual bool React(Agent* a) override;
	virtual bool React(Agent* a, Agent* b) override;

private:
	AgentPattern m_reactant;

	/**
	*	For a state change reaction,
	*	a1 is the previous state of the agent/child-agent
	* a2 is the new state of the agent/child-agent
	*/
	std::vector<ReactionCenter> m_reactionCenters;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_UNIMOLECULAR_REACTION_H
