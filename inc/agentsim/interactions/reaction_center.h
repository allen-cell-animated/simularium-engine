#ifndef AICS_REACTION_CENTER_H
#define AICS_REACTION_CENTER_H

#include "agentsim/pattern/agent_pattern.h"

namespace aics {
namespace agentsim {

class Agent;

/*
*	Reaction Center
*
*	A reaction center describes the before and after states
* of a single reaction participant
*/
struct ReactionCenter
{
	AgentPattern before;
	AgentPattern after;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_REACTION_CENTER_H
