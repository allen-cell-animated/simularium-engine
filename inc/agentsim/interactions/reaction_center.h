#ifndef AICS_REACTION_CENTER_H
#define AICS_REACTION_CENTER_H

#include "agentsim/pattern/agent_pattern.h"

namespace aics {
namespace agentsim {

class Agent;

struct ReactionCenter
{
	AgentPattern a1;
	AgentPattern a2;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_REACTION_CENTER_H
