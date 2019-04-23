#include "agentsim/simpkg/readdypkg.h"
#include "agentsim/agents/agent.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>
#include <math.h>
#include <csignal>
#include <unordered_map>
#include <array>

#define debug_printf //printf

/**
*	Assumes the particle rotations are reasonably orthogonal
*		if true, relaxation in the force evaluation system should
*		correct small 'jitters'
*/
Eigen::Matrix3d get_rotation_matrix(
	std::vector<Eigen::Vector3d> positions);

/**
*	BNGL Data Spatial constraints
*/
void add_spatial_constraints(
	readdy::Simulation& sim,
	std::vector<Eigen::Vector3d>& positions,
	std::vector<std::string>& names,
	std::vector<float> radii,
	float bond_force_constant,
	float angle_force_constant);

/**
*	BNGL Reaction Construction
*/
bool parent_matches(
	const readdy::model::ParticleTypeRegistry& particles,
	readdy::model::top::graph::Vertex* vert,
	aics::agentsim::rxParentState parent_state);

bool find_reactant(
	std::list<readdy::model::top::graph::Vertex>& vertices,
	const readdy::model::ParticleTypeRegistry& particles,
	aics::agentsim::rxComplex complex,
	std::vector<readdy::model::top::graph::Vertex*>& out,
	std::vector<readdy::model::top::graph::Vertex*>& ignore_list);

void configure_fusion_reaction(
	readdy::Simulation& sim,
	aics::agentsim::Reaction& rx,
	aics::agentsim::Model& model,
	std::vector<std::string>& added,
	float bond_force_constant,
	float angle_force_constant);

void construct_fusion_reaction(
	readdy::Simulation& sim,
	aics::agentsim::Reaction& rx);

void configure_fission_reaction(
	readdy::Simulation& sim,
	aics::agentsim::Reaction& rx,
	std::vector<std::string>& added);

void construct_fission_reaction(
	readdy::Simulation& sim,
	aics::agentsim::Reaction& rx);

/**
*	Static variables
**/

	std::unordered_map<std::string, aics::agentsim::Reaction> Topology_Reaction_Dictionary;
	std::unordered_map<std::string, std::vector<aics::agentsim::Reaction>> Topology_Fission_Reaction_Dictionary;
	std::unordered_map<std::string, bool> Registered_Reactions;
	std::unordered_map<std::string, bool> Fusion_Reactions;
	std::unordered_map<std::string, bool> Fission_Reactions;

	std::unordered_map<std::string, float> srx_Fission_Modifier;
	std::unordered_map<std::string, aics::agentsim::ComplexModelData> Reaction_Complexes; // for fission

	void ResetStaticVariables()
	{
		Topology_Reaction_Dictionary.clear();
		Topology_Fission_Reaction_Dictionary.clear();
		Registered_Reactions.clear();
		Fusion_Reactions.clear();
		Fission_Reactions.clear();
		srx_Fission_Modifier.clear();
		Reaction_Complexes.clear();
	}

	#define IMMEDIATE_RATE 1e30
	#include "readdypkg_fileio.cc"

/**
*	Simulation API
**/
namespace aics {
namespace agentsim {

void ReaDDyPkg::Setup()
{

}

void ReaDDyPkg::Shutdown()
{
	readdy::model::ParticleTypeRegistry empty_pr;
	this->m_simulation.context().particleTypes() = empty_pr;

	readdy::model::top::TopologyRegistry empty_tr(this->m_simulation.context().particleTypes());
	this->m_simulation.context().topologyRegistry() = empty_tr;

	readdy::model::reactions::ReactionRegistry empty_rr(this->m_simulation.context().particleTypes());
	this->m_simulation.context().reactions() = empty_rr;

	this->m_simulation.stateModel().clear();
	this->m_timeStepCount = 0;
	this->m_agents_initialized = false;
	this->m_reactions_initialized = false;
	this->m_hasAlreadyRun = false;

	ResetStaticVariables();
}

void ReaDDyPkg::InitAgents(std::vector<std::shared_ptr<Agent>>& agents, Model& model)
{
	int boxSize = pow(model.volume, 1.0/3.0);
	if(!this->m_agents_initialized)
	{
		this->m_simulation.context().boxSize()[0] = boxSize;
		this->m_simulation.context().boxSize()[1] = boxSize;
		this->m_simulation.context().boxSize()[2] = boxSize;

		auto &topologies = this->m_simulation.context().topologyRegistry();
		auto &particles = this->m_simulation.context().particleTypes();
		auto &potentials = this->m_simulation.context().potentials();

		for(auto entry : model.agents)
		{
			aics::agentsim::AgentModelData agent_model_data = entry.second;
			if(agent_model_data.children.size() > 0)
			{
				topologies.addType(agent_model_data.name);

				particles.addTopologyType(
					agent_model_data.name + "_FLAGGED", 0); // used for reaction evaluation
				particles.addTopologyType(
					agent_model_data.name, agent_model_data.diffusion_coefficient);
				particles.addTopologyType(
					agent_model_data.name + "_BOUND", agent_model_data.bound_diffusion_coefficient);

				for(auto subagent : agent_model_data.children)
				{
					particles.addTopologyType(
						subagent.name + "_CHILD", model.child_diffusion_coefficient);
				}
			}
			else
			{
				particles.addTopologyType(
					agent_model_data.name, agent_model_data.diffusion_coefficient);
			}
		}

		for(auto entry : model.constraints)
		{
			if(entry.type == "dihedral")
			{
				readdy::api::TorsionAngle ta;
				ta.forceConstant = entry.parameters["force-constant"];
				ta.phi_0 = entry.parameters["angle"] * M_PI / 180;
				ta.multiplicity = entry.parameters["multiplicity"];
				topologies.configureTorsionPotential(
					entry.agents[0] + "_BOUND",
					entry.agents[1] + "_CHILD",
					entry.agents[2] + "_CHILD",
					entry.agents[3] + "_BOUND",
					ta
				);
			}
			else if(entry.type == "harmonic-angle")
			{
				readdy::api::Angle angle;
				angle.forceConstant = entry.parameters["force-constant"];
				angle.equilibriumAngle = entry.parameters["angle"] * M_PI / 180;
				topologies.configureAnglePotential(
					entry.agents[0] + "_BOUND",
					entry.agents[1] + "_BOUND",
					entry.agents[2] + "_BOUND",
					angle
				);
			}
			else if (entry.type == "harmonic-repulsion")
			{
				potentials.addHarmonicRepulsion(
					entry.agents[0], entry.agents[1],
					entry.parameters["force-constant"], entry.parameters["radius"]);
			}
			else if (entry.type == "harmonic-bond")
			{
				readdy::api::Bond bond;
				bond.forceConstant = entry.parameters["force-constant"];
				bond.length = entry.parameters["length"];
				topologies.configureBondPotential(
					entry.agents[0],
					entry.agents[1],
					bond
				);
			}
		}

		for(auto entry : model.agents)
		{
			auto agent_model_data = entry.second;

			std::vector<Eigen::Vector3d> positions;
			std::vector<std::string> names;
			std::vector<float> radii;

			names.push_back(agent_model_data.name);

			positions.push_back(Eigen::Vector3d(0,0,0));
			radii.push_back(agent_model_data.collision_radius);
			for(auto subagent_model_data : agent_model_data.children)
			{
				float x = subagent_model_data.x_offset;
				float y = subagent_model_data.y_offset;
				float z = subagent_model_data.z_offset;
				positions.push_back(Eigen::Vector3d(x,y,z));
				names.push_back(subagent_model_data.name + "_CHILD");
				radii.push_back(model.agents[subagent_model_data.name].collision_radius);
			}

			if(agent_model_data.children.size() > 0)
			{
				add_spatial_constraints(
					this->m_simulation, positions, names, radii,
					model.parent_child_bond_force_constant,
					model.parent_child_angle_force_constant);
			}
		}

		this->m_agents_initialized = true;
	} else
	{
		printf("This simulation has already been initialized; skipping definitions.\n");
	}

	printf("Creating %lu dynamic ReaDDy topologies.\n", agents.size());
	for(std::size_t i = 0; i < agents.size(); ++i)
	{
		auto parent = agents[i];
		auto agent_model_data = model.agents[parent->GetName()];

		std::vector<readdy::model::TopologyParticle> tp;

		// create the parent particles
		Eigen::Vector3d v = parent->GetLocation();
		tp.push_back(this->m_simulation.createTopologyParticle(
			parent->GetName(), readdy::Vec3(v[0], v[1], v[2])));

		// create the child particles
		for(std::size_t j = 0; j < parent->GetNumChildAgents(); ++j)
		{
			auto child = parent->GetChildAgent(j);
			v += child->GetLocation();
			tp.push_back(this->m_simulation.createTopologyParticle(
				child->GetName() + "_CHILD", readdy::Vec3(v[0], v[1], v[2])));
		}

		// connect the parent-child particles
		auto tp_inst = this->m_simulation.addTopology(parent->GetName(), tp);
		for(std::size_t j = 0; j < parent->GetNumChildAgents(); ++j)
		{
			tp_inst->graph().addEdgeBetweenParticles(0, j+1);
		}

		if(parent->GetNumChildAgents() > 2)
		{
			auto c1 = parent->GetChildAgent(0);
			auto c2 = parent->GetChildAgent(1);

			auto v1 = c1->GetLocation().normalized();
			auto v2 = c2->GetLocation().normalized();
			v1 = (v1 - v1.dot(v2) * v2).normalized();
			auto v3 = (v1.cross(v2)).normalized();

			std::vector<Eigen::Vector3d> basis;
			basis.push_back(parent->GetLocation());
			basis.push_back(v1);
			basis.push_back(v2);
			basis.push_back(v3);
			Eigen::Matrix3d rm = get_rotation_matrix(basis);
			parent->SetInverseChildRotationMatrix(rm.inverse());
		}
	}
}

void ReaDDyPkg::InitReactions(Model& model)
{
	auto &topologies = this->m_simulation.context().topologyRegistry();
	auto &particles = this->m_simulation.context().particleTypes();
	auto &reactions = this->m_simulation.context().reactions();
	Reaction_Complexes = model.complexes;

	if(!this->m_reactions_initialized)
	{
		std::vector<std::string> added;
		for(auto reaction : model.reactions)
		{
			auto rx = reaction.second;

			if(rx.type == "fusion")
			{
				configure_fusion_reaction(this->m_simulation, rx, model, added,
					model.fusion_bond_force_constant, model.fusion_angle_force_constant);
			}
			else if (rx.type == "fission")
			{
				configure_fission_reaction(this->m_simulation, rx, added);
			}
		}

		for(auto reaction : model.reactions)
		{
			auto rx = reaction.second;

			if(rx.type == "fusion")
			{
				construct_fusion_reaction(this->m_simulation, rx);
			}
			else if (rx.type == "fission")
			{
				construct_fission_reaction(this->m_simulation, rx);
			}
			else if (rx.type == "readdy-reaction")
			{
				reactions.add(rx.name + ": " + rx.description, rx.rate);
				printf("New Readdy Reaction %s : %s\n", rx.name.c_str(), rx.description.c_str());
			}
			else if (rx.type == "readdy-spatial")
			{
				topologies.addSpatialReaction(rx.name + ": " + rx.description, rx.rate, rx.distance);
				printf("New Readdy Spatial Reaction %s : %s\n", rx.name.c_str(), rx.description.c_str());
			}
			else
			{
				printf("ReaDDy PKG does not yet recognize reaction of types %s\n", rx.type.c_str());
			}

		}

		this->m_reactions_initialized = true;
	}
}

void ReaDDyPkg::RunTimeStep(
	float timeStep, std::vector<std::shared_ptr<Agent>>& agents)
{
	if(!this->m_realTimeInitialized)
	{
		auto loop = this->m_simulation.createLoop(1);
		loop.runInitialize();
		loop.runInitializeNeighborList();
		this->m_realTimeInitialized = true;
	}

	if(agents.size() == 0)
	{
		return;
	}

	float max_time_step = 1.000f;

	if(timeStep > max_time_step)
	{
		auto loop = this->m_simulation.createLoop(max_time_step);
		unsigned n_iterations = timeStep / max_time_step;

		for(unsigned i = 0; i < n_iterations; ++i)
		{
			std::cout << "Running iteration " << i << " of " << n_iterations << std::endl;
			this->m_timeStepCount++;
			loop.runIntegrator(); // propagate particles
			loop.runUpdateNeighborList(); // neighbor list update
			loop.runReactions(); // evaluate reactions
			loop.runTopologyReactions(); // and topology reactions
			loop.runUpdateNeighborList(); // neighbor list update
			loop.runForces(); // evaluate forces based on current particle configuration
			loop.runEvaluateObservables(this->m_timeStepCount); // evaluate observables
		}
	}
	else
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
	}

  auto unused_agents = agents;

  const auto &topology_list = this->m_simulation.stateModel().getTopologies();
  auto &particles = this->m_simulation.context().particleTypes();
	auto &topologies = this->m_simulation.context().topologyRegistry();

  for(auto& top : topology_list)
  {
		for(auto& vertex : top->graph().vertices())
		{
			auto particle = top->particleForVertex(vertex);
			auto name = particles.nameOf(particle.type());
			bool isBound = false;
			bool foundAgent = false;
			std::size_t index = 0;

			if(name.find("_CHILD") != std::string::npos)
			{
				continue;
			}

			if(name.find("_BOUND") != std::string::npos)
			{
				name.erase(name.begin() + name.find("_BOUND"), name.end());
				isBound = true;
			}

			if(name.find("_FLAGGED") != std::string::npos)
			{
				name.erase(name.begin() + name.find("_FLAGGED"), name.end());
			}

			for(std::size_t i = 0;
				i < unused_agents.size() && !foundAgent;
				++i)
			{
				auto current = unused_agents[i];
				if(current->GetName() == name)
				{
					index = i;
					foundAgent = true;
				}
			}

			if(!foundAgent)
			{
				printf("Failed to find a visual agent to represent ReaDDy simulation agent: %s\n", name.c_str());
				continue;
			}

			auto rp = particle.pos();
			unused_agents[index]->SetLocation(Eigen::Vector3d(rp[0],rp[1],rp[2]));

			std::size_t top_type_offset = 1000;
      unused_agents[index]->SetTypeID(top->type() + top_type_offset);

			auto vertex_with_children = isBound ? &vertex : &(*vertex.neighbors()[0]);
			std::unordered_map<std::string, Eigen::Vector3d> child_positions;
			for(auto& neighbor : vertex_with_children->neighbors())
			{
				auto np = top->particleForVertex(neighbor);
				auto np_name = particles.nameOf(np.type());

				std::size_t pos = np_name.find("_CHILD");
				if(pos == std::string::npos)
				{
					continue;
				}

				np_name.erase(np_name.begin() + pos, np_name.end());
				auto rcp = isBound ? np.pos() : np.pos() + particle.pos();
				auto cp = Eigen::Vector3d(rcp[0], rcp[1], rcp[2]);

				child_positions[np_name] = cp;
			}

			for(std::size_t i = 0; i < unused_agents[index]->GetNumChildAgents(); ++i)
			{
				auto child = unused_agents[index]->GetChildAgent(i);
				auto cp = child_positions[child->GetName()];

				cp = cp - unused_agents[index]->GetLocation();
				child->SetLocation(cp);
			}

			unused_agents.erase(unused_agents.begin() + index);
		}
  }

	for(std::size_t i = 0; i < agents.size(); ++i)
	{
		auto agent = agents[i];

		if(agent->GetNumChildAgents() < 2)
		{
			continue;
		}

		auto v1 = agent->GetChildAgent(0)->GetLocation().normalized();
		auto v2 = agent->GetChildAgent(1)->GetLocation().normalized();
		v1 = (v1 - v1.dot(v2) * v2).normalized();
		auto v3 = (v1.cross(v2)).normalized();

		std::vector<Eigen::Vector3d> basis;
		basis.push_back(agent->GetLocation());
		basis.push_back(v1);
		basis.push_back(v2);
		basis.push_back(v3);
		Eigen::Matrix3d rm = get_rotation_matrix(basis);
		Eigen::Vector3d rea = rm.eulerAngles(0, 1, 2);
		agent->SetRotationFromChildren(rm);
	}
}

void ReaDDyPkg::UpdateParameter(std::string param_name, float param_value)
{
	if(!Registered_Reactions[param_name])
	{
		printf("Unrecognized parameter %s passed to ReaDDy Simulation.\n", param_name.c_str());
		return;
	}

	if(Fusion_Reactions[param_name])
	{
		auto &topologies = this->m_simulation.context().topologyRegistry();
		auto &rx = topologies.spatialReactionByName(param_name).rate() = param_value;
	}

	if(Fission_Reactions[param_name])
	{
		printf("Reaction Rate Adjustment %s : %f\n", param_name.c_str(), param_value);
		srx_Fission_Modifier[param_name] = param_value;

		auto &topologies = this->m_simulation.context().topologyRegistry();
		auto tops = this->m_simulation.currentTopologies();
		for(auto&& top : tops)
		{
			top->updateReactionRates(topologies.structuralReactionsOf(top->type()));
		}
	}

}

} // namespace agentsim
} // namespace aics


/**
*	Helper function implementations
**/

/**
*	Assumes the particle rotations are reasonably orthogonal
*		if true, relaxation in the force evaluation system should
*		correct small 'jitters'
*/
Eigen::Matrix3d get_rotation_matrix(
	std::vector<Eigen::Vector3d> positions
)
{
	auto pos = positions[0];
	auto x_pos = positions[1];
	auto y_pos = positions[2];
	auto z_pos = positions[3];
	auto x_basis = x_pos - pos;
	auto y_basis = y_pos - pos;
	auto z_basis = z_pos - pos;

	Eigen::Matrix3d rotation;
	rotation << x_basis[0], x_basis[1], x_basis[2],
							y_basis[0], y_basis[1], y_basis[2],
							z_basis[0], z_basis[1], z_basis[2];

	rotation.normalize();
	return rotation;
}

/**
*	BNGL Spatial Constraints
*/
void add_spatial_constraints(
	readdy::Simulation& sim,
	std::vector<Eigen::Vector3d>& positions,
	std::vector<std::string>& names,
	std::vector<float> radii,
	float bond_force_constant,
	float angle_force_constant)
{
	readdy::api::Bond bond;
	bond.forceConstant = bond_force_constant;

	readdy::api::Angle angle;
	angle.forceConstant = angle_force_constant;
	angle.equilibriumAngle = M_PI_2;

	auto &topologies = sim.context().topologyRegistry();
	auto &potentials = sim.context().potentials();
	for(std::size_t i = 1; i < positions.size(); ++i)
	{
		float parent_repel_dist = fmin(
			(positions[0] - positions[i]).norm(),
			radii[0] + radii[i]
		);

		bond.length = (positions[0] - positions[i]).norm();
		bond.forceConstant = bond_force_constant;
		topologies.configureBondPotential(names[0], names[i], bond);
		topologies.configureBondPotential(names[0] + "_BOUND", names[i], bond);
		topologies.configureBondPotential(names[0] + "_FLAGGED", names[i], bond);

		potentials.addHarmonicRepulsion(names[0] + "_BOUND", names[i], 70, parent_repel_dist - 0.01f);

		for(std::size_t j = i + 1; j < positions.size(); ++j)
		{
			float child_repel_dist = fmin(
				(positions[i] - positions[j]).norm(),
				radii[i] + radii[j]
			);

			auto v1 = (positions[j] - positions[0]).normalized();
			auto v2 = (positions[i] - positions[0]).normalized();
			angle.equilibriumAngle = acos(v1.dot(v2));

			topologies.configureAnglePotential(names[i], names[0], names[j], angle);
			topologies.configureAnglePotential(names[i], names[0] + "_BOUND", names[j], angle);

			potentials.addHarmonicRepulsion(names[i], names[j], 1, child_repel_dist - 0.01f);
		}
	}

	bond.forceConstant = 0;
	bond.length = radii[0];
	topologies.configureBondPotential(names[0], names[0], bond);
}

/**
*	BNGL Reaction Construction
*/
bool parent_matches(
const readdy::model::ParticleTypeRegistry& particles,
readdy::model::top::graph::Vertex* vert,
aics::agentsim::rxParentState parent_state)
{
	// match state of child molecules
	for(auto child : vert->neighbors())
	{
		auto child_id = child->particleType();
		auto child_name = particles.nameOf(child_id);

		std::size_t pos = child_name.find("_CHILD");
		if(pos == std::string::npos)
		{
			continue;
		}

		debug_printf("Evaluating child %s: ", child_name.c_str());

		child_name.erase(child_name.begin() + pos, child_name.end());

		auto child_state = std::find_if(
			parent_state.children.begin(),
			parent_state.children.end(),
			[&child_name](auto cs)
			{
				return cs.name == child_name;
			}
		);

		if(child_state == parent_state.children.end())
		{
			debug_printf("unspecified child state ignored.\n");
			continue;
		}

		if(child_state->isBound != child->neighbors().size() > 1)
		{
			debug_printf("child state does not match %s.\n",
				child_state->isBound ? "(bound)" : "(unbound)");
			return false;
		}

		debug_printf("child state matches %s.\n",
			child_state->isBound ? "(bound)" : "(unbound)");
	}

	return true;
}

bool find_reactant(
	std::list<readdy::model::top::graph::Vertex>& vertices,
	const readdy::model::ParticleTypeRegistry& particles,
	aics::agentsim::rxComplex complex,
	std::vector<readdy::model::top::graph::Vertex*>& out,
	std::vector<readdy::model::top::graph::Vertex*>& ignore_list)
{
	if(complex.parents.size() == 0)
	{
		printf("This complex has no parent-agents, therefore cannot participate in a reaction.\n");
		return false;
	}

	auto first_parent_state = complex.parents[0];
	readdy::model::top::graph::Vertex* first_vertex = &*std::find_if(
		vertices.begin(), vertices.end(),
		[&first_parent_state, &particles, &ignore_list]
		(readdy::model::top::graph::Vertex& vert)
		{
				auto type = vert.particleType();
				auto name = particles.nameOf(type);

				if(name.find(first_parent_state.name) == std::string::npos
					|| name.find("_CHILD") != std::string::npos)
				{
					return false;
				}

				if(std::find(ignore_list.begin(), ignore_list.end(),&vert)
						!=ignore_list.end())
				{
					return false;
				}

				debug_printf("Checking vertex %s for #0. [%p]\n", name.c_str(), &vert);
				return parent_matches(particles, &vert, first_parent_state);
		}
	);

	if(first_vertex == &*vertices.end())
	{
		return false;
	}

	out.push_back(first_vertex);

	for(std::size_t i = 1; i < complex.parents.size(); ++i)
	{
		bool found = false;
		for(auto partner : first_vertex->neighbors())
		{
			auto partner_id = partner->particleType();
			auto partner_name = particles.nameOf(partner_id);

			if(partner_name.find("_CHILD") != std::string::npos)
			{
				continue;
			}

			debug_printf("checking vertex %s for #%lu. [%p]\n", partner_name.c_str(), i, &*partner);
			if(parent_matches(particles, &*partner,complex.parents[i]))
			{
				out.push_back(&*partner);
				found = true;
				break;
			}
		}

		if(!found)
		{
			debug_printf("failed to find parent %lu\n", i);
			return false; // no match found
		}
	}

	return true;
}

void configure_fusion_reaction(
	readdy::Simulation& sim,
	aics::agentsim::Reaction& rx,
	aics::agentsim::Model& model,
	std::vector<std::string>& added,
	float bond_force_constant,
	float angle_force_constant)
{
	auto &topologies = sim.context().topologyRegistry();
	auto &particles = sim.context().particleTypes();
	auto &potentials = sim.context().potentials();

	std::string tf =
		rx.name + "_" +
		rx.complexes[0].parents[0].name + "_" +
		rx.complexes[1].parents[0].name + "_" +
		rx.product;

	topologies.addType(tf);
	if(std::find(added.begin(), added.end(), rx.product) == added.end())
	{
		topologies.addType(rx.product);
		added.push_back(rx.product);
	}

	readdy::api::Bond bond;
	bond.forceConstant = bond_force_constant;

	readdy::api::Angle angle;
	angle.forceConstant = angle_force_constant;
	angle.equilibriumAngle = M_PI;

	for(auto center : rx.centers)
	{
		std::string parent1 = rx.complexes[0].parents[center.parent_1].name;
		std::string parent2 = rx.complexes[1].parents[center.parent_2].name;

		std::string child1 = center.child_1 + "_CHILD";
		std::string child2 = center.child_2 + "_CHILD";

		bond.length =
			model.agents[center.child_1].collision_radius +
			model.agents[center.child_2].collision_radius;

		bond.forceConstant = bond_force_constant;
		topologies.configureBondPotential(child1, child2, bond);

		float radius = model.agents[parent1].collision_radius +
			model.agents[parent2].collision_radius;

		bond.forceConstant = 0;
		bond.length = radius;
		topologies.configureBondPotential(parent1 + "_BOUND", parent2 + "_BOUND", bond);
		topologies.configureBondPotential(parent1 + "_FLAGGED", parent2 + "_FLAGGED", bond);
		topologies.configureBondPotential(parent1 + "_BOUND", parent2 + "_FLAGGED", bond);
		topologies.configureBondPotential(parent2 + "_BOUND", parent1 + "_FLAGGED", bond);

		topologies.configureAnglePotential(child1, child2, parent2, angle);
		topologies.configureAnglePotential(parent1, child1, child2, angle);

		potentials.addHarmonicRepulsion(parent1 + "_BOUND", parent2 + "_BOUND", 1, radius);
	}
}

void construct_fusion_reaction(readdy::Simulation& sim, aics::agentsim::Reaction& rx)
{
	auto &topologies = sim.context().topologyRegistry();
	auto &particles = sim.context().particleTypes();

	auto complex1_parent1 = rx.complexes[0].parents[0];
	auto complex2_parent1 = rx.complexes[1].parents[0];

	Registered_Reactions[rx.name] = true;
	Fusion_Reactions[rx.name] = true;

	// determine if the particle will have the "_BOUND" tag
	std::string rxr1 = complex1_parent1.name;
	for(std::size_t i = 0; i < complex1_parent1.children.size(); ++i)
	{
		if(complex1_parent1.children[i].isBound)
		{
			rxr1 += "_BOUND";
			break;
		}
	}

	std::string rxr2 = complex2_parent1.name;
	for(std::size_t i = 0; i < complex2_parent1.children.size(); ++i)
	{
		if(complex2_parent1.children[i].isBound)
		{
			rxr2 += "_BOUND";
			break;
		}
	}

	std::string pf1 = rx.complexes[0].parents[0].name + "_FLAGGED";
	std::string pf2 = rx.complexes[1].parents[0].name + "_FLAGGED";
	std::string tf =
		rx.name + "_" +
		rx.complexes[0].parents[0].name + "_" +
		rx.complexes[1].parents[0].name + "_" +
		rx.product;

	// "Name: C1(P1) + C2(P2) -> Product_FLAGGED(P1_FLAGGED--P2_FLAGGED) [self=false]"
	std::string rx_string = rx.name + ": " +
		rx.complexes[0].name + "(" + rxr1 + ") + " +
		rx.complexes[1].name + "(" + rxr2 + ") -> " +
		tf + "(" + pf1 + "--" + pf2 + ") [self=false]";

	Topology_Reaction_Dictionary[tf] = rx;

	printf("NEW ReaDDy REACTION | %s\n", rx_string.c_str());
	topologies.addSpatialReaction(rx_string, rx.rate, rx.distance);

	auto srx_func = [](readdy::model::top::GraphTopology &top) {
		readdy::model::top::reactions::Recipe recipe(top);
		auto &particles = top.context().particleTypes();
		auto &topologies = top.context().topologyRegistry();
		auto &vertices = top.graph().vertices();

		auto tname = topologies.nameOf(top.type());
		auto reaction = Topology_Reaction_Dictionary[tname];
		debug_printf("\nEvaluating %s reaction for %s topology.\n", reaction.name.c_str(), tname.c_str());

		debug_printf("Searching for reactant: %s\n", reaction.complexes[0].name.c_str());
		std::vector<readdy::model::top::graph::Vertex*> reactant1,reactant2;
		if(!find_reactant(
			vertices,	particles,
			reaction.complexes[0],
			reactant1, reactant2
			))
		{
			printf("Failed to find reactant 1 for %s reaction.\n",reaction.name.c_str());
			std::raise(SIGABRT);
		}

		debug_printf("Searching for reactant: %s\n", reaction.complexes[1].name.c_str());
		if(!find_reactant(
			vertices,	particles,
			reaction.complexes[1],
			reactant2, reactant1
			))
		{
			printf("Failed to find reactant 2 for %s reaction.\n",reaction.name.c_str());
			std::raise(SIGABRT);
		}

		std::vector<readdy::model::top::graph::Vertex*> flagged;
		for(auto vert = vertices.begin(), end = vertices.end(); vert != end; ++vert)
		{
			auto type = vert->particleType();
			auto name = particles.nameOf(type);

			std::size_t pos = name.find("_FLAGGED");
			if(pos != std::string::npos)
			{
				flagged.push_back(&*vert);

				// correct particle type
				name.erase(name.begin() + pos, name.end());
				name += "_BOUND";
				recipe.changeParticleType(*vert, name);
			}
		}

		if(flagged.size() == 2)
		{
			recipe.removeEdge(*flagged[0], *flagged[1]);
		}
		else
		{
			printf("Found %lu flagged vertices for %s reaction. Expected 2. \n",
				flagged.size(), reaction.name.c_str());
		}

		auto find_child = [&particles, &recipe](
			auto vertex, std::string parent, std::string child)
		{
			auto child_id = particles.idOf(child + "_CHILD");

			// search neighbors first
			for(auto& other : vertex->neighbors())
			{
				if(other->particleType() == child_id)
				{
					return other;
				}
			}

			printf("Failed to find child vertex %s on parent %s\n", child.c_str(), parent.c_str());
			std::raise(SIGABRT);
			return vertex->neighbors()[0];
		};

		bool reposition1 = reactant1.size() == 1 && reactant2.size() != 1;
		bool reposition2 = reactant2.size() == 1 && reactant1.size() != 1;
		readdy::Vec3 reposition_location;
		std::size_t reposition_divisor = 0;

		for(auto center : reaction.centers)
		{
			auto parent1_name = reaction.complexes[0].parents[center.parent_1].name;
			auto parent2_name = reaction.complexes[1].parents[center.parent_2].name;

			debug_printf("Binding %s(%s)--%s(%s) [%p | %p]\n",
				parent1_name.c_str(), center.child_1.c_str(),
				parent2_name.c_str(), center.child_2.c_str(),
				reactant1[center.parent_1], reactant2[center.parent_2]);

			auto child1 = find_child(
				reactant1[center.parent_1],
				parent1_name,
				center.child_1);
			auto child2 = find_child(
				reactant2[center.parent_2],
				parent2_name,
				center.child_2);

			recipe.addEdge(child1, child2);
			recipe.addEdge(*reactant1[center.parent_1],*reactant2[center.parent_2]);

			reposition_location += reposition1 ?
				2*top.particleForVertex(child2).pos() - top.particleForVertex(*reactant2[0]).pos() :
				readdy::Vec3(0,0,0);

			reposition_location += reposition2 ?
				2*top.particleForVertex(child1).pos() - top.particleForVertex(*reactant1[0]).pos() :
				readdy::Vec3(0,0,0);

			reposition_divisor++;
		}

		reposition_location = reposition_location / reposition_divisor;
		auto current_pos_parent1 = top.particleForVertex(*reactant1[0]).pos();
		auto current_pos_parent2 = top.particleForVertex(*reactant2[0]).pos();

		auto new_position1 = reposition1 ? reposition_location : current_pos_parent1;
		auto new_position2 = reposition1 ? reposition_location : current_pos_parent2;

		recipe.changeParticlePosition(*reactant1[0], new_position1);
		recipe.changeParticlePosition(*reactant2[0], new_position2);

		if(reposition1)
		{
			for(auto child : reactant1[0]->neighbors())
			{
				auto name = particles.nameOf(child->particleType());
				if(name.find("_CHILD") == std::string::npos)
				{
					continue;
				}

				auto current_pos_child = top.particleForVertex(*&child).pos();
				auto current_pos_parent = top.particleForVertex(*reactant1[0]).pos();
				auto offset = current_pos_child - current_pos_parent;
				recipe.changeParticlePosition(child, new_position1 + offset);
			}
		}

		if(reposition2)
		{
			for(auto child : reactant2[0]->neighbors())
			{
				auto name = particles.nameOf(child->particleType());
				if(name.find("_CHILD") == std::string::npos)
				{
					continue;
				}

				auto current_pos_child = top.particleForVertex(*&child).pos();
				auto current_pos_parent = top.particleForVertex(*reactant2[0]).pos();
				auto offset = current_pos_child - current_pos_parent;
				recipe.changeParticlePosition(child, new_position2 + offset);
			}
		}

		std::string delimiter = "_";

		for(std::size_t i = 0; i < reactant1.size(); ++i)
		{
			auto type = reactant1[i]->particleType();
			auto name = particles.nameOf(type);

			if(name.find("_BOUND") != std::string::npos)
			{
				continue;
			}

			name.erase(name.begin() + name.find(delimiter), name.end());
			recipe.changeParticleType(*reactant1[i], name + "_BOUND");
		}

		for(std::size_t i = 0; i < reactant2.size(); ++i)
		{
			auto type = reactant2[i]->particleType();
			auto name = particles.nameOf(type);

			if(name.find("_BOUND") != std::string::npos)
			{
				continue;
			}

			name.erase(name.begin() + name.find(delimiter), name.end());
			recipe.changeParticleType(*reactant2[i], name + "_BOUND");
		}

		// tname = [Reactant1 Name]_[Reactant2 Name]_[Product Name]
		size_t pos = 0;
		std::vector<std::string> tokens;
		while ((pos = tname.find(delimiter)) != std::string::npos) {
				std::string token = tname.substr(0, pos);
				tname.erase(0, pos + delimiter.length());
		}

		debug_printf("Finished Reaction.\n");
		recipe.changeTopologyType(tname);
		return recipe;
	};

	readdy::model::top::reactions::StructuralTopologyReaction srx(
		srx_func, IMMEDIATE_RATE
	);
	topologies.addStructuralReaction(tf, srx);
}

void configure_fission_reaction(
	readdy::Simulation& sim,
	aics::agentsim::Reaction& rx,
	std::vector<std::string>& added)
{
	auto &topologies = sim.context().topologyRegistry();

	std::string product =  rx.complexes[0].name + "_FISSION_FLAGGED";
	if(std::find(added.begin(), added.end(), product) == added.end())
	{
		printf("Adding topology %s for reaction %s \n", rx.name.c_str(), rx.product.c_str());
		topologies.addType(product);
		added.push_back(product);
	}
}

void construct_fission_reaction(
	readdy::Simulation& sim,
	aics::agentsim::Reaction& rx)
{
	auto &topologies = sim.context().topologyRegistry();

	Registered_Reactions[rx.name] = true;
	Fission_Reactions[rx.name] = true;

	std::string rx_fission_product = rx.complexes[0].name + "_FISSION_FLAGGED";
	Topology_Reaction_Dictionary[rx.complexes[0].name] = rx;
	Topology_Fission_Reaction_Dictionary[rx.complexes[0].name].push_back(rx);
	srx_Fission_Modifier[rx.name] = rx.rate;

	auto srx_func = [](readdy::model::top::GraphTopology &top) {
		readdy::model::top::reactions::Recipe recipe(top);
		auto &particles = top.context().particleTypes();
		auto &topologies = top.context().topologyRegistry();
		auto &vertices = top.graph().vertices();

		auto tname = topologies.nameOf(top.type());
		auto reaction = Topology_Reaction_Dictionary[tname];

		// Choose reaction
		auto rxs = Topology_Fission_Reaction_Dictionary[tname];
		float rate = 0;
		for(auto rx : rxs)
		{
			rate += srx_Fission_Modifier[rx.name];
		}

		auto rand_int = rand();
		debug_printf("Fission RX int selector: %i\n", rand_int);
		debug_printf("Fission RX rate sum: %f\n", rate);

		float rx_selector = (static_cast <float> (rand_int) / static_cast <float> (RAND_MAX)) * rate;
		debug_printf("Fission RX selector random number: %f\n", rx_selector);
		for(auto rx : rxs)
		{
			debug_printf("Fission RX selector random number: %f subtracting %f\n", rx_selector, srx_Fission_Modifier[rx.name]);
			rx_selector -= srx_Fission_Modifier[rx.name];
			if(rx_selector <= 0.0f)
			{
				debug_printf("Fission RX selected.\n");
				reaction = rx;
				break;
			}
		}

		debug_printf("\nEvaluating %s reaction for %s topology.\n", reaction.name.c_str(), tname.c_str());

		debug_printf("Searching for reactant: %s\n", reaction.complexes[0].name.c_str());
		std::vector<readdy::model::top::graph::Vertex*> reactant;
		if(!find_reactant(
			vertices,	particles,
			reaction.complexes[0],
			reactant, reactant
			))
		{
			debug_printf("Failed to find reactant for %s reaction.\n",reaction.name.c_str());
			return recipe;
		}

		auto find_child = [&particles, &recipe](
			auto vertex, std::string parent, std::string child)
		{
			auto child_id = particles.idOf(child + "_CHILD");

			// search neighbors first
			for(auto& other : vertex->neighbors())
			{
				if(other->particleType() == child_id)
				{
					return other;
				}
			}

			printf("Failed to find child vertex %s on parent %s\n", child.c_str(), parent.c_str());
			std::raise(SIGABRT);
			return vertex->neighbors()[0];
		};

		for(auto center : reaction.centers)
		{
			auto parent1_name = reaction.complexes[0].parents[center.parent_1].name;
			auto parent2_name = reaction.complexes[0].parents[center.parent_2].name;

			debug_printf("Unbinding %s(%s)--%s(%s) [%p | %p]\n",
				parent1_name.c_str(), center.child_1.c_str(),
				parent2_name.c_str(), center.child_2.c_str(),
				reactant[center.parent_1], reactant[center.parent_2]);

			debug_printf("Center: parent[%lu] child[%s] -/- parent[%lu] child[%s]\n",
				center.parent_1, center.child_1.c_str(),
				center.parent_2, center.child_2.c_str());

			auto child1 = find_child(
				reactant[center.parent_1],
				parent1_name,
				center.child_1);
			auto child2 = find_child(
				reactant[center.parent_2],
				parent2_name,
				center.child_2);

			recipe.removeEdge(child1, child2);
			recipe.removeEdge(*reactant[center.parent_1],*reactant[center.parent_2]);
		}

		recipe.changeTopologyType(tname + "_FISSION_FLAGGED");
		return recipe;
	};

	readdy::model::top::reactions::StructuralTopologyReaction srx(
		srx_func, [](const auto& top)
	{
			auto &topologies = top.context().topologyRegistry();
			auto &vertices = top.graph().vertices();

			auto tname = topologies.nameOf(top.type());
			auto rxs = Topology_Fission_Reaction_Dictionary[tname];
			float rate = 0;
			for(auto rx : rxs)
			{
				rate += srx_Fission_Modifier[rx.name];
			}

			return rate;
	});

	topologies.addStructuralReaction(rx.complexes[0].name, srx);

	auto srx_typing_func = [](readdy::model::top::GraphTopology &top) {
		readdy::model::top::reactions::Recipe recipe(top);
		auto &particles = top.context().particleTypes();
		auto &topologies = top.context().topologyRegistry();
		auto &vertices = top.graph().vertices();

		auto tname = topologies.nameOf(top.type());

		debug_printf("Evaluating typing reaction for topology %s with %lu vertices\n",
			tname.c_str(), vertices.size());

		std::unordered_map<std::string, std::size_t> agent_counts;
		std::size_t total_count = 0;
		auto count_agents =
			[&particles, &vertices]
			(std::unordered_map<std::string, std::size_t>& map,
			std::size_t& total)
		{
			for(auto vert = vertices.begin(), end = vertices.end(); vert != end; ++vert)
			{
					auto name = particles.nameOf(vert->particleType());
					if(name.find("_CHILD") != std::string::npos)
					{
						continue;
					}

					total++;
					name.erase(name.begin() + name.find("_BOUND"), name.end());
					if(map.count(name) == 0)
					{
						map[name] = 1;
					}
					else
					{
						map[name] = map[name] + 1;
					}
			}
		};

		count_agents(agent_counts, total_count);

		for(auto entry : Reaction_Complexes)
		{
				auto complex = entry.second;
				auto complex_name = entry.first;
				bool match = true;

				for(auto agent : complex.children)
				{
					if(agent_counts.count(agent.name) == 0)
					{
						match = false;
						break;
					}

					auto count = agent_counts[agent.name];
					if(count > agent.max_count || count < agent.min_count)
					{
						match = false;
						break;
					}
				}

				if(match == true)
				{
					debug_printf("Changing topology %s to %s\n", tname.c_str(), complex_name.c_str());
					recipe.changeTopologyType(complex_name);
					return recipe;
				}
		}

		printf("Invalid Fission Reaction: Resultant topology %s does not match any existing topology.\n",
			tname.c_str());
		std::raise(SIGABRT);
		return recipe;
	};

	readdy::model::top::reactions::StructuralTopologyReaction srx_typing_rx(
		srx_typing_func, IMMEDIATE_RATE);

	topologies.addStructuralReaction(rx_fission_product, srx_typing_rx);

}
