#ifndef AICS_SIMULATION_H
#define AICS_SIMULATION_H

#include <memory>
#include <vector>
#include "agentsim/agents/agent.h"

namespace aics {
namespace agentsim {

class Agent;
class MovementSimulator;
class ReactionSimulator;

class Simulation
{
public:
		Simulation(
			std::shared_ptr<MovementSimulator> ms_shared_ptr,
			std::shared_ptr<ReactionSimulator> rs_shared_ptr);

		void RunTimeStep(float timeStep);

private:
		std::vector<Agent*> m_agents;

		std::shared_ptr<MovementSimulator> m_MovementSimulator;
		std::shared_ptr<ReactionSimulator> m_ReactionSimulator;
};

}
}

#endif // AICS_SIMULATION_H
