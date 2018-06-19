#ifndef AICS_REACTION_H
#define AICS_REACTION_H

namespace aics {
namespace agentsim {

class Agent;
class AgentPattern;

class Reaction
{
public:
	virtual bool RegisterReactant(AgentPattern& ap) = 0;
	virtual bool IsReactant(Agent* a) = 0;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_REACTION_H
