#ifndef AICS_UNIMOLECULAR_REACTION_H
#define AICS_UNIMOLECULAR_REACTION_H

#include "agentsim/interactions/reaction.h"
#include "agentsim/pattern/agent_pattern.h"

namespace aics {
namespace agentsim {

class StateChangeReaction : public Reaction
{
public:
	virtual bool RegisterReactant(AgentPattern& ap) override;
	virtual bool IsReactant(Agent* a) override;

private:
	AgentPattern m_reactant;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_UNIMOLECULAR_REACTION_H
