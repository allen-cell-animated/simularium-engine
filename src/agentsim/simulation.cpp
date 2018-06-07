#include "agentsim/simulation.h"
#include "agentsim/simulators/movement_simulator.h"
#include "agentsim/simulators/reaction_simulator.h"

namespace aics {
namespace agentsim {

Simulation::Simulation(
	std::shared_ptr<MovementSimulator> ms_shared_ptr,
	std::shared_ptr<ReactionSimulator> rs_shared_ptr)
{
		m_MovementSimulator = ms_shared_ptr;
		m_ReactionSimulator = rs_shared_ptr;
}

void Simulation::RunTimeStep(float timeStep)
{
		m_MovementSimulator->RunTimeStep(timeStep, this->m_agents);
		m_ReactionSimulator->RunTimeStep(timeStep, this->m_agents);
}

} // namespace agentsim
} // namespace aics
