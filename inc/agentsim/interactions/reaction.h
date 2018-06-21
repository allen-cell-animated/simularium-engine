#ifndef AICS_REACTION_H
#define AICS_REACTION_H

#include "agentsim/interactions/reaction_state_change.h"
#include "agentsim/interactions/reaction_bond_change.h"

namespace aics {
namespace agentsim {

class Agent;
class AgentPattern;

class Reaction
{
public:
	virtual ~Reaction() {}
	virtual bool RegisterReactant(AgentPattern& ap) = 0;
	virtual bool IsReactant(Agent* a) = 0;
	virtual bool React(Agent* a) = 0;
	virtual bool React(Agent* a, Agent* b) = 0;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_REACTION_H
