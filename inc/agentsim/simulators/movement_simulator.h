#ifndef AICS_MOVEMENT_SIMULATOR_H
#define AICS_MOVEMENT_SIMULATOR_H

#include <vector>

namespace aics {
namespace agentsim {

class Agent;

class MovementSimulator
{
public:
		virtual void RunTimeStep(float timestep, std::vector<Agent*> agents);
};

} // namespace agentsim
} // namespace aics

#endif // AICS_MOVEMENT_SIMULATOR_H
