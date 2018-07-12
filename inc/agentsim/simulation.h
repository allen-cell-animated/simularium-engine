#ifndef AICS_SIMULATION_H
#define AICS_SIMULATION_H

#include <memory>
#include <vector>

namespace aics {
namespace agentsim {

class Agent;
class SimPkg;

struct AgentData
{
	float type;
	float x, y, z;
	float xrot, yrot, zrot;
};

class Simulation
{
public:
	Simulation(
		std::vector<std::shared_ptr<SimPkg>> simPkgs,
		std::vector<std::shared_ptr<Agent>> agents
	);
	~Simulation();

	void RunTimeStep(float timeStep);
	std::vector<AgentData> GetData();

	void Reset();

private:
	std::vector<std::shared_ptr<Agent>> m_agents;

	std::vector<std::shared_ptr<SimPkg>> m_SimPkgs;
};

}
}

#endif // AICS_SIMULATION_H
