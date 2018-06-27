#include "agentsim/agentsim.h"

using namespace aics::agentsim;

int main(int argc, char **argv) {
		std::shared_ptr<SimPkg> simpleMove;
		simpleMove.reset(new SimpleMove);

		std::vector<std::shared_ptr<SimPkg>> simulators;
		simulators.push_back(simpleMove);

		std::size_t numAgents = 1000;
		std::vector<std::shared_ptr<Agent>> agents;
		for(std::size_t i = 0; i < numAgents; ++i)
		{
			std::shared_ptr<Agent> a;
			a.reset(new Agent());

			agents.push_back(a);
		}

		Simulation simulation(simulators, agents);
		simulation.RunTimeStep(1.0f);
}
