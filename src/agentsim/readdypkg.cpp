
#include "agentsim/simpkg/readdypkg.h"
#include "agentsim/agents/agent.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>

namespace aics {
namespace agentsim {

void ReaDDyPkg::Setup()
{

}

void ReaDDyPkg::Shutdown()
{

}

void ReaDDyPkg::RunTimeStep(
	float timeStep, std::vector<std::shared_ptr<Agent>>& agents)
{
	this->m_simulation.run(1, timeStep);

	agents.clear();
	std::vector<std::string> pTypes = { "core", "end", "monomer"};

	for(std::size_t i = 0; i < pTypes.size(); ++i)
	{
		std::vector<readdy::Vec3> positions = this->m_simulation.getParticlePositions(pTypes[i]);
		for(std::size_t j = 0; j < positions.size(); ++j)
		{
			readdy::Vec3 v = positions[j];
			std::shared_ptr<Agent> newAgent;
			newAgent.reset(new Agent());
			newAgent->SetName(pTypes[i]);
			newAgent->SetTypeID(i);
			newAgent->SetLocation(Eigen::Vector3d(v[0], v[1], v[2]));
			agents.push_back(newAgent);
		}
	}
}

void ReaDDyPkg::InitParticles()
{
	this->m_simulation.setKernel("SingleCPU");
	this->m_simulation.setKBT(300);

	float boxSize = 100.f;
	this->m_simulation.currentContext().boxSize()[0] = boxSize;
	this->m_simulation.currentContext().boxSize()[1] = boxSize;
	this->m_simulation.currentContext().boxSize()[2] = boxSize;

	this->m_simulation.registerParticleType("monomer", 6.25e6);
	this->m_simulation.registerParticleType(
		"end", 6.25e6, readdy::model::particleflavor::TOPOLOGY);
	this->m_simulation.registerParticleType(
		"core", 6.25e6, readdy::model::particleflavor::TOPOLOGY);
	this->m_simulation.registerTopologyType("filament");

	this->m_simulation.configureTopologyBondPotential(
		"end", "core", 10, 0.5);

	this->m_simulation.configureTopologyBondPotential(
		"core", "core", 10, 0.5);

	this->m_simulation.configureTopologyAnglePotential(
		"core", "core", "core", 1000, 3.14);

	this->m_simulation.registerHarmonicRepulsionPotential(
		"core","core", 10,0.5);

	this->m_simulation.registerSpatialTopologyReaction(
		"Bind: filament(end) + (monomer) -> filament(core--end)", 3.3e3, 50
	);

	std::size_t monomerCount = 500;
	for(std::size_t i = 0; i < monomerCount; ++i)
	{
		float x,y,z;
		int boxSize = 100.f;
		x = rand() % boxSize - boxSize / 2;
		y = rand() % boxSize - boxSize / 2;
		z = rand() % boxSize - boxSize / 2;
		this->m_simulation.addParticle("monomer", x, y, z);
	}

	this->m_simulation.addParticle("monomer", 0, 0, 0);

	std::vector<readdy::model::TopologyParticle> tp;
	tp.push_back(this->m_simulation.createTopologyParticle("end", readdy::Vec3(1,0,0)));
	tp.push_back(this->m_simulation.createTopologyParticle("core", readdy::Vec3(0,0,0)));
	tp.push_back(this->m_simulation.createTopologyParticle("end", readdy::Vec3(-1,0,0)));
	auto tp_inst = this->m_simulation.addTopology("filament", tp);
	tp_inst->graph().addEdgeBetweenParticles(0,1);
	tp_inst->graph().addEdgeBetweenParticles(1,2);
}

void ReaDDyPkg::InitReactions()
{

}

} // namespace agentsim
} // namespace aics
