#ifdef MAKE_EXT_PKGS

#include "agentsim/simpkg/readdypkg.h"
#include "agentsim/agents/agent.h"
#include "readdy/readdy.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>

namespace aics {
namespace agentsim {

void ReaDDyPkg::Setup()
{
	this->m_simulation = new readdy::this->m_simulation();
	this->m_simulation->setKernel("SingleCPU");
	this->m_simulation->setKBT(300);

	this->m_context = new readdy::model::this->m_context();

	float boxSize = 100.f;
	this->m_simulation->currentContext().boxSize()[0] = boxSize;
	this->m_simulation->currentContext().boxSize()[1] = boxSize;
	this->m_simulation->currentContext().boxSize()[2] = boxSize;
}

void ReaDDyPkg::Shutdown()
{
	delete this->m_simulation;
	this->m_simulation = nullptr;
}

void ReaDDyPkg::RunTimeStep(
	float timeStep, std::vector<std::shared_ptr<Agent>>& agents)
{
	this->m_simulation->RunTimeStep(1, timeStep);
}

void ReaDDyPkg::InitParticles(std::vector<std::shared_ptr<Agent>>& agents)
{
	this->m_simulation.registerParticleType("monomer", 6.25e6);
	this->m_simulation.registerParticleType(
		"end", 6.25e6, readdy::model::particle_flavor::TOPOLOGY);
	this->m_simulation.registerParticleType(
		"core", 6.25e6, readdy::model::particle_flavor::TOPOLOGY);
	this->m_simulation->registerTopologyType("filament");

	this->m_simulation->configureTopologyBondPotential(
		"end", "core", 10, 0.5);

	this->m_simulation->configureTopologyBondPotential(
		"core", "core", 10, 0.5);

	this->m_simulation->configureTopologyAnglePotential(
		"core", "core", "core", 1000, 3.14);

	this->m_simulation->registerHarmonicRepulsionPotential(
		"core","core", 10,0.5);

	this->m_simulation->registerSpatialTopologyReaction(
		"Bind: filament(end) + (monomer) -> filament(core--end)", 3.3e3, 50
	);

	std::size_t monomerCount = 500;
	for(std::size_t i = 0; i < monomerCount; ++i)
	{
		float x,y,z;
		float boxSize = 100.f;
		x = rand() % boxSize - boxSize / 2;
		y = rand() % boxSize - boxSize / 2;
		z = rand() % boxSize - boxSize / 2;
		this->m_simulation->addParticle("monomer", x, y, z);
	}

	std::vector<readdy::model::TopologyParticle> tp;
	tp.push_back(this->m_simulation->createTopologyParticle("end", Vec3(1,0,0)));
	tp.push_back(this->m_simulation->createTopologyParticle("core", Vec3(0,0,0)));
	tp.push_back(this->m_simulation->createTopologyParticle("end", Vec3(-1,0,0)));
	this->m_simulation->addTopology("filament", tp);
}

void ReaDDyPkg::InitReactions(std::vector<ext::readdy::rxd> rxs)
{

}

} // namespace agentsim
} // namespace aics

#endif // MAKE_EXT_PKGS
