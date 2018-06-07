#ifndef AICS_REACTION_SIMULATOR_H
#define AICS_REACTION_SIMULATOR_H

#include <vector>

namespace aics {
namespace agentsim {

class Agent;

class ReactionSimulator
{
public:
		virtual void RunTimeStep(float timestep, std::vector<Agent*> agents);
};

} // namespace agentsim
} // namespace aics

#endif // AICS_REACTION_SIMULATOR_H
