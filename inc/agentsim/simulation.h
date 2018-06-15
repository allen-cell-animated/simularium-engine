#ifndef AICS_SIMULATION_H
#define AICS_SIMULATION_H

#include <memory>
#include <vector>

namespace aics {
namespace agentsim {

class Agent;
class SimPkg;

class Simulation
{
public:
		Simulation(std::vector<std::shared_ptr<SimPkg>> simPkgs);
		~Simulation();

		void RunTimeStep(float timeStep);

private:
		std::vector<Agent*> m_agents;

		std::vector<std::shared_ptr<SimPkg>> m_SimPkgs;
};

}
}

#endif // AICS_SIMULATION_H
