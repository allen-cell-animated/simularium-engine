#ifndef AICS_REACTION_STATE_CHANGE_H
#define AICS_REACTION_STATE_CHANGE_H

#include "agentsim/pattern/agent_pattern.h"

namespace aics {
namespace agentsim {

class Agent;

/*
*	Reaction State Change
*
*	A reaction state change describes the before and after states
* of a single reaction participant
*
*	@TODO: verify that before and after are infact the
*	"same" molecule, (same agent tree depth, same children, etc.)
*/
struct ReactionStateChange
{
	AgentPattern before;
	AgentPattern after;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_REACTION_STATE_CHANGE_H
