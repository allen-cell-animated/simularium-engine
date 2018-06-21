#ifndef AICS_COMBINATION_REACTION_H
#define AICS_COMBINATION_REACTION_H

#include "agentsim/interactions/reaction.h"
#include <vector>
#include <memory>

namespace aics {
namespace agentsim {

class CombinationReaction : public Reaction
{
public:
	virtual ~CombinationReaction() {}
	virtual bool RegisterReactant(AgentPattern ap) override;
	virtual bool IsReactant(Agent* a) = 0;
	virtual bool RegisterBondChange(ReactionBondChange rbc);
	virtual bool React(std::shared_ptr<Agent>, std::shared_ptr<Agent>) = 0;
	virtual std::shared_ptr<Agent> GetProduct();

private:
	std::vector<AgentPattern> m_reactants;
	ReactionBondChange m_bondChanges;
	std::shared_ptr<Agent> m_lastProduct;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_COMBINATION_REACTION_H
