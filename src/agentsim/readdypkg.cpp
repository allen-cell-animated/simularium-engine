
#include "agentsim/simpkg/readdypkg.h"
#include "agentsim/agents/agent.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <csignal>

namespace aics {
namespace agentsim {

void ReaDDyPkg::Setup()
{
	int boxSize = 1000;
	if(!initialized)
	{
		this->m_simulation.context().boxSize()[0] = boxSize;
		this->m_simulation.context().boxSize()[1] = boxSize;
		this->m_simulation.context().boxSize()[2] = boxSize;

		auto &particles = this->m_simulation.context().particleTypes();
		particles.addTopologyType("monomer", 6.25e14);
		particles.addTopologyType("end", 6.25e9);
		particles.addTopologyType("core", 6.25e9);

		readdy::api::Bond bond;
		bond.forceConstant = 1;
		bond.length = 10;

		readdy::api::Angle angle;
		angle.forceConstant = 5;
		angle.equilibriumAngle = 3.14;

		auto &topologies = this->m_simulation.context().topologyRegistry();
		topologies.addType("filament");
		topologies.addType("free");
		topologies.configureBondPotential("end","end", bond);
		topologies.configureBondPotential("end","core", bond);
		topologies.configureBondPotential("core","core", bond);
		topologies.configureAnglePotential("core","core","core", angle);
		topologies.configureAnglePotential("end","core","core", angle);
		topologies.configureAnglePotential("end","core","end", angle);

		topologies.addSpatialReaction(
			"Growth: filament(end) + free(monomer) -> filament(core--end)", 3.7e-6, 50
		);

		topologies.addSpatialReaction(
			"Combine: filament(end) + filament(end) -> filament(core--core)", 3.7e-6, 50
		);

		topologies.addSpatialReaction(
			"Nucleate: free(monomer) + free(monomer) -> filament(end--end)", 3.7e-6, 50
		);

		auto rGrowthFunc = [&](readdy::model::top::GraphTopology &top) {
			readdy::model::top::reactions::Recipe recipe(top);

			if(top.graph().vertices().size() < 4)
			{
				return recipe;
			}

			auto v1 = top.graph().vertices().begin();
			if(v1->particleType() == 1) // end
			{
				v1 = v1->neighbors()[0];
			}

			auto v2 = v1->neighbors()[0];
			if(v2->particleType() == 1)
			{
				v2 = v1->neighbors()[1];
			}

			printf("Performing dissociation.\n");
			recipe.removeEdge(v1, v2);

			recipe.changeParticleType(v1, "end");
			recipe.changeParticleType(v2, "end");
			recipe.removeEdge(v1, v2);

			return recipe;
		};

		readdy::model::top::reactions::StructuralTopologyReaction rGrowthRx(
			rGrowthFunc, 3.7e-16
		);
		topologies.addStructuralReaction("filament", rGrowthRx);

		auto &potentials = this->m_simulation.context().potentials();
		potentials.addHarmonicRepulsion("core", "core", 5, 0.5);

		initialized = true;
	}

	std::size_t monomerCount = 500;
	for(std::size_t i = 0; i < monomerCount; ++i)
	{
		float x,y,z;
		x = rand() % boxSize - boxSize / 2;
		y = rand() % boxSize - boxSize / 2;
		z = rand() % boxSize - boxSize / 2;

		std::vector<readdy::model::TopologyParticle> tp;
		tp.push_back(this->m_simulation.createTopologyParticle(
			"monomer", readdy::Vec3(x,y,z)));
		this->m_simulation.addTopology("free", tp);
	}

	std::size_t filamentCount = 5;
	for(std::size_t i = 0; i < filamentCount; ++i)
	{
		float x,y,z;
		x = rand() % boxSize - boxSize / 2;
		y = rand() % boxSize - boxSize / 2;
		z = rand() % boxSize - boxSize / 2;

		std::vector<readdy::model::TopologyParticle> tp;
		tp.push_back(this->m_simulation.createTopologyParticle(
			"end", readdy::Vec3(1,0,0) + readdy::Vec3(x / 2,y / 2,z / 2)));
		tp.push_back(this->m_simulation.createTopologyParticle(
			"core", readdy::Vec3(0,0,0) + readdy::Vec3(x / 2,y / 2,z / 2)));
		tp.push_back(this->m_simulation.createTopologyParticle(
			"end", readdy::Vec3(-1,0,0) + readdy::Vec3(x / 2,y / 2,z / 2)));
		auto tp_inst = this->m_simulation.addTopology("filament", tp);
		tp_inst->graph().addEdgeBetweenParticles(0,1);
		tp_inst->graph().addEdgeBetweenParticles(1,2);
	}
}

void ReaDDyPkg::Shutdown()
{
	// @TODO: I haven't figured out how to restart this without crashing yet
}

void ReaDDyPkg::RunTimeStep(
	float timeStep, std::vector<std::shared_ptr<Agent>>& agents)
{
	this->m_simulation.run(1, timeStep);

	agents.clear();
	std::vector<std::string> pTypes = { "core", "end", "monomer" };

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
	static const int num_params = 3;
	std::string recognized_params[num_params] = {
		"Nucleation Rate", "Growth Rate", "Dissociation Rate" };
	std::size_t paramIndex = 252; // assuming less than 252 parameters

	for(std::size_t i = 0; i < num_params; ++i)
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
			auto &topologies = this->m_simulation.context().topologyRegistry();
			auto &nucrx = topologies.spatialReactionByName("Nucleate");
			nucrx.setRate(param_value);
		} break;
		case 1: // GrowthRate
		{
			auto &topologies = this->m_simulation.context().topologyRegistry();
			auto &growthrx = topologies.spatialReactionByName("Growth");
			growthrx.setRate(param_value);

			auto& combinerx = topologies.spatialReactionByName("Combine");
			combinerx.setRate(param_value);
		} break;
		case 2: // DissociationRate
		{
			auto &topologies = this->m_simulation.context().topologyRegistry();
			auto &srxs = topologies.getStructuralReactionsOf("filament");

			for(std::size_t i = 0; i < srxs.size(); ++i)
			{
				srxs[i].setRate(param_value);
			}

			auto tops = this->m_simulation.currentTopologies();
			for(auto&& top : tops)
			{
				if(top->type() == 0)
				{
					top->updateReactionRates(topologies.structuralReactionsOf(top->type()));
				}
			}
		} break;
		default:
		{
			printf("Recognized but unimplemented parameter %s in ReaDDy SimPkg, resolved as index %lu.\n",
				param_name.c_str(), paramIndex);
			return;
		} break;
	}

}

} // namespace agentsim
} // namespace aics
