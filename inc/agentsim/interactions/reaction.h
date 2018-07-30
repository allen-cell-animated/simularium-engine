#ifndef AICS_REACTION_H
#define AICS_REACTION_H

#include "agentsim/interactions/reaction_state_change.h"
#include "agentsim/interactions/reaction_bond_change.h"
#include <memory>
#include <string>

namespace aics {
namespace agentsim {

class Agent;
class AgentPattern;

class Reaction
{
public:
	virtual ~Reaction() {}
	virtual bool RegisterReactant(AgentPattern ap) = 0;
	virtual bool IsReactant(Agent* a) = 0;
	virtual bool React(std::shared_ptr<Agent> a, std::shared_ptr<Agent> b = nullptr) = 0;
	virtual std::shared_ptr<Agent> GetProduct() { return nullptr; };
	virtual std::string GetKey() { return ""; }
};

} // namespace agentsim
} // namespace aics

#endif // AICS_REACTION_H
