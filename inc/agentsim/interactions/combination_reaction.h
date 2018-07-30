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
	virtual bool IsReactant(Agent* a) override;
	virtual bool RegisterBondChange(ReactionBondChange rbc);
	virtual bool React(std::shared_ptr<Agent>, std::shared_ptr<Agent>) override;
	virtual std::shared_ptr<Agent> GetProduct() override;
	virtual std::string GetKey() override;

private:
	std::vector<AgentPattern> m_reactants;
	ReactionBondChange m_bondChanges;
	std::shared_ptr<Agent> m_lastProduct;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_COMBINATION_REACTION_H
