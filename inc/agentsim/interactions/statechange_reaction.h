#ifndef AICS_UNIMOLECULAR_REACTION_H
#define AICS_UNIMOLECULAR_REACTION_H

#include "agentsim/interactions/reaction.h"
#include <vector>
#include "agentsim/pattern/agent_pattern.h"

namespace aics {
namespace agentsim {

class StateChangeReaction : public Reaction
{
public:
	virtual ~StateChangeReaction() {}
	virtual bool RegisterReactant(AgentPattern& ap) override;
	virtual void RegisterStateChange(ReactionStateChange rc);
	virtual bool IsReactant(Agent* a) override;
	virtual bool React(Agent* a) override;
	virtual bool React(Agent* a, Agent* b) override;

private:
	AgentPattern m_reactant;

	std::vector<ReactionStateChange> m_stateChanges;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_UNIMOLECULAR_REACTION_H
