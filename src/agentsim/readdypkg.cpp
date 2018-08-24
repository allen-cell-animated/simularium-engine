#include "agentsim/simpkg/readdypkg.h"
#include "agentsim/agents/agent.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>


void configureSubHelixPotential(
	readdy::Simulation& sim,
	float force_constant, float angle_degrees,
	std::string A, std::string B, std::string C, std::string D)
{
	readdy::api::Angle angle;
	angle.forceConstant = force_constant;
	angle.equilibriumAngle = 3.14 * angle_degrees / 180;

	auto &topologies = sim.context().topologyRegistry();
	topologies.configureAnglePotential(A,C,D, angle);
	topologies.configureAnglePotential(C,D,B, angle);
	topologies.configureAnglePotential(D,B,A, angle);
	topologies.configureAnglePotential(B,A,C, angle);

	topologies.configureAnglePotential(A,B,C, angle);
	topologies.configureAnglePotential(A,C,B, angle);
	topologies.configureAnglePotential(A,D,B, angle);
	topologies.configureAnglePotential(A,D,C, angle);

	topologies.configureAnglePotential(B,C,D, angle);
	topologies.configureAnglePotential(B,D,A, angle);
	topologies.configureAnglePotential(B,A,D, angle);
	topologies.configureAnglePotential(B,C,A, angle);

	topologies.configureAnglePotential(D,C,B, angle);
	topologies.configureAnglePotential(D,B,C, angle);
	topologies.configureAnglePotential(D,A,B, angle);
	topologies.configureAnglePotential(D,A,C, angle);

	topologies.configureAnglePotential(C,A,D, angle);
	topologies.configureAnglePotential(C,D,A, angle);
	topologies.configureAnglePotential(C,B,D, angle);
	topologies.configureAnglePotential(C,B,A, angle);
}

void configureHelixPotential(
	readdy::Simulation& sim,
	float angle_fc, float angle_degrees,
	float bond_fc, float bond_dist,
	std::string A, std::string B, std::string C, std::string D,
	std::string E, std::string F)
{
	configureSubHelixPotential(
		sim, angle_fc, angle_degrees, A, B, C, D
	);
	configureSubHelixPotential(
		sim, angle_fc, angle_degrees, C, D, E, F
	);
	configureSubHelixPotential(
		sim, angle_fc, angle_degrees, E, F, A, B
	);

	readdy::api::Angle angle;
	angle.forceConstant = angle_fc;
	angle.equilibriumAngle = 3.14;

	auto &topologies = sim.context().topologyRegistry();
	topologies.configureAnglePotential(A,C,E, angle);
	topologies.configureAnglePotential(C,E,A, angle);
	topologies.configureAnglePotential(E,A,C, angle);

	topologies.configureAnglePotential(B,D,F, angle);
	topologies.configureAnglePotential(D,F,B, angle);
	topologies.configureAnglePotential(F,B,D, angle);

	topologies.configureAnglePotential(A,C,E, angle);
	topologies.configureAnglePotential(C,E,A, angle);
	topologies.configureAnglePotential(E,A,C, angle);

	topologies.configureAnglePotential(B,D,F, angle);
	topologies.configureAnglePotential(D,F,B, angle);
	topologies.configureAnglePotential(F,B,D, angle);

	readdy::api::Bond bond;
	bond.forceConstant = bond_fc;
	bond.length = bond_dist;

	topologies.configureBondPotential(A,A, bond);
	topologies.configureBondPotential(A,B, bond);
	topologies.configureBondPotential(A,C, bond);
	topologies.configureBondPotential(A,D, bond);
	topologies.configureBondPotential(A,E, bond);
	topologies.configureBondPotential(A,F, bond);

	topologies.configureBondPotential(B,B, bond);
	topologies.configureBondPotential(B,C, bond);
	topologies.configureBondPotential(B,D, bond);
	topologies.configureBondPotential(B,E, bond);
	topologies.configureBondPotential(B,F, bond);

	topologies.configureBondPotential(C,C, bond);
	topologies.configureBondPotential(C,D, bond);
	topologies.configureBondPotential(C,E, bond);
	topologies.configureBondPotential(C,F, bond);

	topologies.configureBondPotential(D,D, bond);
	topologies.configureBondPotential(D,E, bond);
	topologies.configureBondPotential(D,F, bond);

	topologies.configureBondPotential(E,E, bond);
	topologies.configureBondPotential(E,F, bond);

	topologies.configureBondPotential(F,F, bond);
}

void connect_subhelix_graph(
	readdy::model::top::GraphTopology* top,
	int start
)
{
	top->graph().addEdgeBetweenParticles(start + 0, start + 1);
	top->graph().addEdgeBetweenParticles(start + 0, start + 2);
	top->graph().addEdgeBetweenParticles(start + 1, start + 3);
	top->graph().addEdgeBetweenParticles(start + 2, start + 3);

	top->graph().addEdgeBetweenParticles(start + 0, start + 3);
	top->graph().addEdgeBetweenParticles(start + 1, start + 2);
}

void connect_helix_graph(
	readdy::model::top::GraphTopology* top
)
{
	int size = top->graph().vertices().size();
	size = size - 4;
	for(;size >= 0; size = size - 2)
	{
		connect_subhelix_graph(top, size);
	}
}

void create_helix_particle(
	readdy::Simulation& sim,
	std::size_t size,
	float x, float y, float z,
	std::string A, std::string B, std::string C, std::string D,
	std::string E, std::string F, std::string top_type
)
{
	readdy::Vec3 pos(x,y,z);
	readdy::Vec3 offset(3,0,0);

	std::vector<readdy::model::TopologyParticle> tp;

	for(std::size_t i = 0; i < size; ++i)
	{
		tp.push_back(sim.createTopologyParticle(
			A, pos + readdy::Vec3(0,0,0) + i * offset));
		tp.push_back(sim.createTopologyParticle(
			B, pos + readdy::Vec3(0,1,0) + i * offset));
		tp.push_back(sim.createTopologyParticle(
			C, pos + readdy::Vec3(1,0,0) + i * offset));
		tp.push_back(sim.createTopologyParticle(
			D, pos + readdy::Vec3(1,1,0) + i * offset));
		tp.push_back(sim.createTopologyParticle(
			E, pos + readdy::Vec3(2,0,0) + i * offset));
		tp.push_back(sim.createTopologyParticle(
			F, pos + readdy::Vec3(2,1,0) + i * offset));
	}

	auto top = sim.addTopology(top_type, tp);
	connect_helix_graph(top);
}

readdy::model::top::graph::Vertex* find_closest_n_edge_particle(
	readdy::model::top::graph::Vertex* v,
	std::size_t n
)
{
	std::unordered_map<readdy::model::top::graph::Vertex*, bool> visited;
	std::queue<readdy::model::top::graph::Vertex*> to_visit;
	to_visit.push(v);
	visited[v] = true;

	while(to_visit.size() > 0)
	{
		auto current = to_visit.front();
		if(!visited[current] && current->neighbors().size() == n)
		{
			return current;
		}

		for(std::size_t i = 0; i < current->neighbors().size(); ++i)
		{
			if(visited[&(*current->neighbors()[i])] == true)
			{
				continue;
			}

			to_visit.push(&(*current->neighbors()[i]));
		}

		to_visit.pop();
		visited[current] = true;
	}

	return nullptr;
}

bool is_helix_dir_forward(int t5, int tx)
{
	int b = t5;
	int f = t5;
	while(1)
	{
		b--;
		f++;

		if(b < 0) { b += 6; }
		if(f >= 6) { f -= 6; }

		if(b == tx && f == tx)
		{
			return t5 % 2 == 0;
		}
		else if(b == tx)
		{
			return false;
		}
		else if(f == tx)
		{
			return true;
		}
	}

	printf("Unexpected error in helix direction detection. \n");
	return false;
}

void createHelixBindRx(
	readdy::Simulation& sim,
	std::vector<std::string> types
)
{
	auto stage_1 = [&](readdy::model::top::GraphTopology &top) {
		readdy::model::top::reactions::Recipe recipe(top);

		auto* vertices = &(top.graph().vertices());
		auto& topologies = top.context().topologyRegistry();

		auto uv = --top.graph().vertices().begin();
		for(auto iterator = vertices->begin(), end = vertices->end();
					iterator != end; ++iterator)
		{
			if (iterator->neighbors().size() == 1)
			{
				uv = iterator;
				break;
			}
		}

		if(uv == --top.graph().vertices().begin())
		{
			printf("New vertex not found in FT_stage_1 Reaction. \n");
			recipe.changeTopologyType("FT");
			return recipe;
		}

		readdy::Vec3 offset(1,0,0);
		auto uvn = uv->neighbors()[0];
		switch(uvn->neighbors().size())
		{
			case 5:
			{
				auto v2 = find_closest_n_edge_particle(&(*uvn), 2);
				auto v4 = find_closest_n_edge_particle(&(*uvn), 4);
				auto v5 = find_closest_n_edge_particle(&(*uvn), 5);

				int t5 = v5->particleType();
				int t4 = v4->particleType();
				bool increasing = is_helix_dir_forward(t5, t4);

				int tc = v2->particleType();
				int sign = increasing ? 1 : -1;

				int nt;
				nt = tc + sign * 1;
				nt = nt % 6;
				if(nt < 0) { nt += 6; }
				if(nt > 5) { nt -= 6; }

				recipe.changeParticleType(uv, nt);
				recipe.addEdge(*uv, *v2);
				recipe.addEdge(*uv, *v4);
				offset = 2 *
					(top.particleForVertex(*v4).pos() -
					 top.particleForVertex(*v5).pos());
			} break;
			case 4:
			{
				auto v3 = find_closest_n_edge_particle(&(*uvn), 3);
				auto v5 = find_closest_n_edge_particle(&(*uvn), 5);

				int t3 = v3->particleType();
				int t5 = v5->particleType();
				bool increasing = is_helix_dir_forward(t5, t3);

				std::vector<int> ids = {
					uvn->particleType(), v3->particleType()
				};

				int tc = increasing ?
					*std::max_element(ids.begin(), ids.end()) :
					*std::min_element(ids.begin(), ids.end());
				int sign = increasing ? 1 : -1;

				int nt;
				nt = tc + sign * 1;
				nt = nt % 6;
				if(nt < 0) { nt += 6; }
				if(nt > 5) { nt -= 6; }

				recipe.changeParticleType(uv, nt);
				recipe.addEdge(*uv, *v3);
				offset = 2 *
					(top.particleForVertex(*v3).pos() -
					 top.particleForVertex(*v5).pos());
			} break;
			default:
			{
				recipe.changeParticleType(uv, "M");
				recipe.removeEdge(uv, uvn);
			} break;
		}

		auto new_pos = top.particleForVertex(uvn).pos() + offset;
		recipe.changeParticlePosition(*uv, new_pos);
		recipe.changeTopologyType("FT");
		return recipe;
	};

	readdy::model::top::reactions::StructuralTopologyReaction stage_1_rx(
			stage_1, 1e30
		);
	sim.context().topologyRegistry().addStructuralReaction(
		"FT_stage_1", stage_1_rx);
}

namespace aics {
namespace agentsim {

void ReaDDyPkg::Setup()
{
	int boxSize = 1000;
	if(!this->m_initialized)
	{
		this->m_simulation.context().boxSize()[0] = boxSize;
		this->m_simulation.context().boxSize()[1] = boxSize;
		this->m_simulation.context().boxSize()[2] = boxSize;
		std::vector<std::string> helix_types = { "A", "B", "C", "D", "E", "F" };

		auto &particles = this->m_simulation.context().particleTypes();
		for(std::size_t i = 0; i < helix_types.size(); ++i)
		{
			std::string t = helix_types[i];
			particles.addTopologyType(t, 0.01);
		}
		particles.addTopologyType("M", 1e3);
		particles.addTopologyType("U", 0.1);

		auto &topologies = this->m_simulation.context().topologyRegistry();
		topologies.addType("FT");
		topologies.addType("FT_stage_1");

		readdy::api::Bond bond;
		bond.forceConstant = 10;
		bond.length = 2;

		for(std::size_t i = 0; i < helix_types.size(); ++i)
		{
			std::string t = helix_types[i];
			topologies.configureBondPotential(t, "U", bond);
			topologies.configureBondPotential(t, "M", bond);
			topologies.addSpatialReaction(
				t + "M: FT(" + t + ") + FT(M) -> FT_stage_1(" + t + "--U) [self=false]", 1, 50);
		}

		configureHelixPotential(
			this->m_simulation,
			60, 165, 10, 2,
			"A", "B", "C", "D", "E", "F"
		);

		createHelixBindRx(this->m_simulation, helix_types);

		auto &potentials = this->m_simulation.context().potentials();
		this->m_initialized = true;
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
			"M", readdy::Vec3(x,y,z)));
		this->m_simulation.addTopology("FT", tp);
	}

	std::size_t filamentCount = 1;
	for(std::size_t i = 0; i < filamentCount; ++i)
	{
		float x,y,z;
		x = rand() % boxSize - boxSize / 2;
		y = rand() % boxSize - boxSize / 2;
		z = rand() % boxSize - boxSize / 2;

		create_helix_particle(
			this->m_simulation,
			1, 0, 0, 0,
			"A","B","C","D","E","F",
			"FT");
	}

	auto loop = this->m_simulation.createLoop(1);
	loop.runInitialize();
	loop.runInitializeNeighborList();
}

void ReaDDyPkg::Shutdown()
{
	this->m_simulation.stateModel().clear();
	this->m_timeStepCount = 0;
}

void ReaDDyPkg::RunTimeStep(
	float timeStep, std::vector<std::shared_ptr<Agent>>& agents)
{
	auto loop = this->m_simulation.createLoop(timeStep);

	this->m_timeStepCount++;
	loop.runIntegrator(); // propagate particles
	loop.runUpdateNeighborList(); // neighbor list update
	loop.runReactions(); // evaluate reactions
	loop.runTopologyReactions(); // and topology reactions
	loop.runUpdateNeighborList(); // neighbor list update
	loop.runForces(); // evaluate forces based on current particle configuration
	loop.runEvaluateObservables(this->m_timeStepCount); // evaluate observables

	agents.clear();
	std::vector<std::string> pTypes = { "A", "B", "C", "D", "E", "F", "M", "U" };

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

		} break;
		case 1: // GrowthRate
		{

		} break;
		case 2: // DissociationRate
		{

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
