
#include "agentsim/simpkg/readdypkg.h"
#include "agentsim/agents/agent.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>

namespace aics {
namespace agentsim {

void ReaDDyPkg::Setup()
{
	this->m_simulation = new readdy::Simulation("SingleCPU");

	int boxSize = 1000;
	this->m_simulation->context().boxSize()[0] = boxSize;
	this->m_simulation->context().boxSize()[1] = boxSize;
	this->m_simulation->context().boxSize()[2] = boxSize;

	auto &particles = this->m_simulation->context().particleTypes();
	particles.add("monomer", 6.25e14);
	particles.addTopologyType("end", 6.25e9);
	particles.addTopologyType("core", 6.25e9);

	readdy::api::Bond bond;
	bond.forceConstant = 1;
	bond.length = 10;

	readdy::api::Angle angle;
	angle.forceConstant = 5;
	angle.equilibriumAngle = 3.14;

	auto &topologies = this->m_simulation->context().topologyRegistry();
	topologies.addType("filament");
	topologies.configureBondPotential("end","core", bond);
	topologies.configureBondPotential("core","core", bond);
	topologies.configureAnglePotential("core","core","core", angle);
	topologies.addSpatialReaction(
		"Bind: filament(end) + (monomer) -> filament(core--end)", 7, 50
	);

	auto &potentials = this->m_simulation->context().potentials();
	potentials.addHarmonicRepulsion("core", "core", 5, 0.5);

	std::size_t monomerCount = 500;
	for(std::size_t i = 0; i < monomerCount; ++i)
	{
		float x,y,z;
		x = rand() % boxSize - boxSize / 2;
		y = rand() % boxSize - boxSize / 2;
		z = rand() % boxSize - boxSize / 2;
		this->m_simulation->addParticle("monomer", x, y, z);
	}

	this->m_simulation->addParticle("monomer", 0, 0, 0);

	std::size_t filamentCount = 5;
	for(std::size_t i = 0; i < filamentCount; ++i)
	{
		float x,y,z;
		x = rand() % boxSize - boxSize / 2;
		y = rand() % boxSize - boxSize / 2;
		z = rand() % boxSize - boxSize / 2;

		std::vector<readdy::model::TopologyParticle> tp;
		tp.push_back(this->m_simulation->createTopologyParticle(
			"end", readdy::Vec3(1,0,0) + readdy::Vec3(x / 2,y / 2,z / 2)));
		tp.push_back(this->m_simulation->createTopologyParticle(
			"core", readdy::Vec3(0,0,0) + readdy::Vec3(x / 2,y / 2,z / 2)));
		tp.push_back(this->m_simulation->createTopologyParticle(
			"end", readdy::Vec3(-1,0,0) + readdy::Vec3(x / 2,y / 2,z / 2)));
		auto tp_inst = this->m_simulation->addTopology("filament", tp);
		tp_inst->graph().addEdgeBetweenParticles(0,1);
		tp_inst->graph().addEdgeBetweenParticles(1,2);
	}
}

void ReaDDyPkg::Shutdown()
{
	delete this->m_simulation;
	this->m_simulation = nullptr;
}

void ReaDDyPkg::RunTimeStep(
	float timeStep, std::vector<std::shared_ptr<Agent>>& agents)
{
	auto &topologies = this->m_simulation->context().topologyRegistry();
	auto &bindrx = topologies.spatialReactionByName("Bind");
	printf("bind rate: %lu\n", (long unsigned int)bindrx.rate());
	this->m_simulation->run(1, timeStep);

	agents.clear();
	std::vector<std::string> pTypes = { "core", "end", "monomer"};

	for(std::size_t i = 0; i < pTypes.size(); ++i)
	{
		std::vector<readdy::Vec3> positions = this->m_simulation->getParticlePositions(pTypes[i]);
		for(std::size_t j = 0; j < positions.size(); ++j)
		{
			readdy::Vec3 v = positions[j];
			std::shared_ptr<Agent> newAgent;
			newAgent.reset(new Agent());
			newAgent->SetName(pTypes[i]);
			newAgent->SetTypeID(i);
			newAgent->SetLocation(Eigen::Vector3d(v[0], v[1], v[2]));

			// Purely for visual effect; ReaDDy doesn't have a concept of rotations
			float rotMultiplier = 1;

			if(pTypes[i] == "monomer")
			{
				rotMultiplier = 6e14;
			}
			else
			{
				rotMultiplier = 6e9;
			}

			float xrot = rand() % 360;
			float yrot = rand() % 360;
			float zrot = rand() % 360;
			Eigen::Vector3d newRot = newAgent->GetRotation() +
				Eigen::Vector3d(xrot,yrot,zrot);
			newAgent->SetRotation(rotMultiplier * timeStep * newRot);

			agents.push_back(newAgent);
		}
	}
}

void ReaDDyPkg::UpdateParameter(std::string param_name, float param_value)
{
	std::string recognized_params[2] = {"NucleationRate", "GrowthRate"};
	std::size_t paramIndex = 252; // assuming less than 252 parameters

	for(std::size_t i = 0; i < 2; ++i)
	{
		if(recognized_params[i] == param_name)
		{
			paramIndex = i;
			break;
		}
	}

	if(paramIndex == 252)
	{
		printf("Unrecognized parameter %s passed into ReaddyPkg.\n", param_name.c_str());
		return;
	}

	switch(paramIndex)
	{
		case 0: // NucleationRate
		{

		} break;
		case 1: // GrowthRate
		{
			auto &topologies = this->m_simulation->context().topologyRegistry();
			auto &bindrx = topologies.spatialReactionByName("Bind");
			bindrx.setRate(param_value);
		} break;
		default:
		{
			printf("Recognized but unimplemented parameter %s in ReaDDy SimPkg.\n",
				param_name.c_str());
			return;
		} break;
	}

}

} // namespace agentsim
} // namespace aics
