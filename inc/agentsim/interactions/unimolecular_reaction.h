#ifndef AICS_UNIMOLECULAR_REACTION_H
#define AICS_UNIMOLECULAR_REACTION_H

#include "agentsim/interactions/reaction.h"

namespace aics {
namespace agentsim {

class UnimolecularReaction : public Reaction
{
public:
	virtual bool IsReactant(Agent* a) override;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_UNIMOLECULAR_REACTION_H
