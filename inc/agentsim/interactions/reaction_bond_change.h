#ifndef AICS_REACTION_BOND_CHANGE_H
#define AICS_REACTION_BOND_CHANGE_H

#include <vector>
#include "Eigen/Dense"
#include "agentsim/pattern/agent_pattern.h"

namespace aics {
namespace agentsim {

/**
*	Reaction Bond Change
*
*	A reaction bond change describes the bonds to be formed
*	reactants are indexed by thier order in the reaction definition
*	this allows the bond formation to be tracked by indices
*
*	the patterns lists below correspond to 'level 1' agents,
* meaning agents with an agent-tree depth of 1.
*	this corresponds to molecules agents, which own binding sites agents,
*	in the case of biochemical reactions
*
*	e.g. A(a,b,c).B.(a,b,c) + C(a,b,c)
*	has 3 'level 1' agents: A. B, and C
*	reactant_patterns[0] = A
*	reactant_patterns[1] = B
*	reactant_patterns[2] = C
*
*	similarly, bond_indices stores the index of the level 1 agents
*	in reactant_patterns, and the index of the child agent forming a bond
*	bonds are stored sequentially
*
*	e.g. [... [0, 1] , [1, 0], ...] indicates a bond between
* MatchFor(reactant_patterns[0])->GetChildAgent(1) and
*	MatchFor(reactant_patterns[1])->GetChildAgent(0)
*
*	the final value indicates which agent is the final agent of reactant 1
*	e.g. end_reactant_1 = 7, indicates that indices 0 - 6 belong to reactant 1,
*	in reactant_patterns
*/
struct ReactionBondChange
{
	std::vector<AgentPattern> reactant_patterns;
	std::vector<Eigen::Vector2i> bond_indices;
	std::size_t end_reactant_1;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_REACTION_STATE_CHANGE_H
