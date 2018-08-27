#include "agentsim/simpkg/readdypkg.h"
#include "agentsim/agents/agent.h"
#include <algorithm>
#include <stdlib.h>
#include <time.h>

/**
* Oriented Species
**/
void add_oriented_species(readdy::Simulation& sim,
	std::string name, float diff_coeff,
	std::vector<std::string>& out_types,
	std::vector<float>& out_radii);

void add_bound_constraints(
	readdy::Simulation& sim,
	std::string n1, std::string n2, float r1, float r2);

std::vector<readdy::model::top::graph::Vertex*> get_particle_and_basis_vertices(
	std::string type, readdy::model::top::GraphTopology& top);

Eigen::Matrix3d get_rotation_matrix(
	std::vector<readdy::model::top::graph::Vertex*> vertices,
	std::string type, readdy::model::top::GraphTopology& topology
);

/**
*	Assumes the particle rotations are reasonably orthogonal
*		if true, relaxation in the force evaluation system should
*		correct small 'jitters'
*/
Eigen::Matrix3d get_rotation_matrix(
	std::vector<Eigen::Vector3d> positions
);

void add_oriented_particle(readdy::Simulation& sim,
	std::string name, Eigen::Vector3d pos);

void add_parent_child_bind_reaction(readdy::Simulation& sim,
	std::string& parent, std::string& child);

/**
*	Helix Creation
*/
void configureSubHelixPotential(
	readdy::Simulation& sim,
	readdy::api::Angle angle,
	std::string A, std::string B, std::string C, std::string D);

void configureHelixPotential(
	readdy::Simulation& sim,
	readdy::api::Angle angle,
	readdy::api::Bond bond,
	std::string A, std::string B, std::string C, std::string D,
	std::string E, std::string F);

void configureHelixPotential(
	readdy::Simulation& sim,
	readdy::api::Angle angle,
	readdy::api::Bond bond,
	std::vector<std::string> helix_types
);

void connect_subhelix_graph(
	readdy::model::top::GraphTopology* top,
	int start
);

void connect_helix_graph(
	readdy::model::top::GraphTopology* top
);

void create_helix_particle(
	readdy::Simulation& sim,
	std::size_t size,
	readdy::Vec3 pos,
	std::string A, std::string B, std::string C, std::string D,
	std::string E, std::string F, std::string top_type
);

void create_helix_particle(
	readdy::Simulation& sim,
	std::size_t size,
	readdy::Vec3 pos,
	std::vector<std::string> helixt_types, std::string top_type
);

readdy::model::top::graph::Vertex* find_closest_n_edge_particle(
	readdy::model::top::graph::Vertex* v,
	std::size_t n
);

bool is_helix_dir_forward(int t5, int tx);

void createHelixBindRx(
	readdy::Simulation& sim,
	std::vector<std::string> types
);

/**
*	Unscoped variables
**/

	// helix
	std::vector<std::string> helix_types = { "A", "B", "C", "D", "E", "F" };
	int helix_pid_0 = 0; // the id of the first helix_type

	std::vector<std::string> particle_types;

	// oriented binding
	std::string parent_type = "parent";
	std::string child_type = "child";
	Eigen::Vector3d position_offset;

/**
*	Simulation API
**/
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

		auto &particles = this->m_simulation.context().particleTypes();
		particles.addTopologyType("M", 1e3);
		particles.addTopologyType("U", 0.1);
		particle_types.push_back("M");
		particle_types.push_back("U");

		helix_pid_0 = particle_types.size();
		for(std::size_t i = 0; i < helix_types.size(); ++i)
		{
			std::string t = helix_types[i];
			particles.addTopologyType(t, 0.01);
		}

		particle_types.insert(std::end(particle_types),
			std::begin(helix_types), std::end(helix_types));

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
			topologies.addSpatialReaction(
				t + "M: FT(" + t + ") + FT(M) -> FT_stage_1(" + t + "--U) [self=false]", 1, 50);
		}

		readdy::api::Angle angle;
		angle.forceConstant = 60;
		angle.equilibriumAngle = 3.14 * 165 / 180;

		configureHelixPotential( this->m_simulation, angle, bond, helix_types);
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

		readdy::Vec3 pos(0,0,0);
		int filemant_size = 1;
		create_helix_particle(
			this->m_simulation,
			filemant_size, pos, helix_types,"FT");
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
	std::vector<std::string> pTypes = particle_types;
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


/**
*	Helper function implementations
**/

/**
*	Oriented
**/
void add_oriented_species(readdy::Simulation& sim,
	std::string name, float diff_coeff,
	std::vector<std::string>& out_types,
	std::vector<float>& out_radii)
{
	if(diff_coeff > 1.0f)
	{
		return;
		printf("add_oriented_species failed: diffusion coefficient must be less than 1. \n");
	}

	std::string cxn, cyn, czn, tn, bn, bcxn, bcyn, bczn;
	cxn = name + "_x";
	cyn = name + "_y";
	czn = name + "_z";
	tn = "complex_" + name;
	bn = name + "_bound";
	bcxn = name + "_bound_x";
	bcyn = name + "_bound_y";
	bczn = name + "_bound_z";

	out_types.push_back(cxn);
	out_types.push_back(cyn);
	out_types.push_back(czn);
	out_types.push_back(bcxn);
	out_types.push_back(bcyn);
	out_types.push_back(bczn);
	out_types.push_back(name);
	out_types.push_back(bn);

	float component_radii = 0.3f;
	float radii = 1.f;
	out_radii.push_back(component_radii);
	out_radii.push_back(component_radii);
	out_radii.push_back(component_radii);
	out_radii.push_back(component_radii);
	out_radii.push_back(component_radii);
	out_radii.push_back(component_radii);
	out_radii.push_back(radii);
	out_radii.push_back(radii);

	float fc = diff_coeff == 0 ? 1 : 15 * pow(10, -log(diff_coeff));

	readdy::api::Bond bond;
	bond.forceConstant = fc;
	bond.length = 1;

	readdy::api::Angle angle;
	angle.forceConstant = fc;
	angle.equilibriumAngle = M_PI_2;

	auto &particles = sim.context().particleTypes();
	particles.addTopologyType(cxn, diff_coeff);
	particles.addTopologyType(cyn, diff_coeff);
	particles.addTopologyType(czn, diff_coeff);
	particles.addTopologyType(bcxn, diff_coeff);
	particles.addTopologyType(bcyn, diff_coeff);
	particles.addTopologyType(bczn, diff_coeff);
	particles.addTopologyType(name, diff_coeff);
	particles.addTopologyType(bn, diff_coeff / 10);

	auto &topologies = sim.context().topologyRegistry();
	topologies.addType(tn);

	bond.length = 1;
	topologies.configureBondPotential(name, cxn, bond);
	topologies.configureBondPotential(name, cyn, bond);
	topologies.configureBondPotential(name, czn, bond);

	topologies.configureBondPotential(bn, cxn, bond);
	topologies.configureBondPotential(bn, cyn, bond);
	topologies.configureBondPotential(bn, czn, bond);

	topologies.configureBondPotential(bn, bcxn, bond);
	topologies.configureBondPotential(bn, bcyn, bond);
	topologies.configureBondPotential(bn, bczn, bond);

	bond.length = M_SQRT2;
	topologies.configureBondPotential(cxn, cyn, bond);
	topologies.configureBondPotential(cxn, czn, bond);
	topologies.configureBondPotential(cyn, czn, bond);

	topologies.configureBondPotential(bcxn, bcyn, bond);
	topologies.configureBondPotential(bcxn, bczn, bond);
	topologies.configureBondPotential(bcyn, bczn, bond);

	if(diff_coeff > 0)
	{
		angle.equilibriumAngle = M_PI_2;
		topologies.configureAnglePotential(cxn, name, cyn, angle);
		topologies.configureAnglePotential(cxn, name, czn, angle);
		topologies.configureAnglePotential(cyn, name, czn, angle);

		topologies.configureAnglePotential(cxn, bn, cyn, angle);
		topologies.configureAnglePotential(cxn, bn, czn, angle);
		topologies.configureAnglePotential(cyn, bn, czn, angle);

		topologies.configureAnglePotential(bcxn, bn, bcyn, angle);
		topologies.configureAnglePotential(bcxn, bn, bczn, angle);
		topologies.configureAnglePotential(bcyn, bn, bczn, angle);

		angle.equilibriumAngle = M_PI / 3;
		topologies.configureAnglePotential(cyn, cxn, czn, angle);
		topologies.configureAnglePotential(czn, cyn, cxn, angle);
		topologies.configureAnglePotential(cxn, czn, cyn, angle);

		topologies.configureAnglePotential(bcyn, bcxn, bczn, angle);
		topologies.configureAnglePotential(bczn, bcyn, bcxn, angle);
		topologies.configureAnglePotential(bcxn, bczn, bcyn, angle);
	}
}

void add_bound_constraints(
	readdy::Simulation& sim,
	std::string n1, std::string n2, float r1, float r2)
{
	auto &topologies = sim.context().topologyRegistry();
	topologies.addType("complex_" + n1 + n2);
	topologies.addType("complex_" + n1 + n2 + "_bound");

	readdy::api::Bond bond;
	bond.forceConstant = 1000;
	bond.length = r1 + r2;

	bond.forceConstant = 30;
	topologies.configureBondPotential(n1 + "_bound", n2 + "_bound", bond);

	bond.forceConstant = 1e-13;
	topologies.configureBondPotential(n1 + "_bound", n2 + "_y", bond);
	topologies.configureBondPotential(n2 + "_bound", n1 + "_y", bond);
	topologies.configureBondPotential(n1 + "_x", n2 + "_x", bond);
	topologies.configureBondPotential(n1 + "_y", n2 + "_y", bond);
	topologies.configureBondPotential(n1 + "_z", n2 + "_z", bond);

	topologies.configureBondPotential(n1 + "_bound", n2 + "_bound_y", bond);
	topologies.configureBondPotential(n2 + "_bound", n1 + "_bound_y", bond);
	topologies.configureBondPotential(n1 + "_bound_x", n2 + "_bound_x", bond);
	topologies.configureBondPotential(n1 + "_bound_y", n2 + "_bound_y", bond);
	topologies.configureBondPotential(n1 + "_bound_z", n2 + "_bound_z", bond);

	readdy::api::Angle angle;
	angle.forceConstant = 50;
	angle.equilibriumAngle = M_PI;

	topologies.configureAnglePotential(
		n1 + "_bound", n1 + "_y", n2 + "_bound", angle);
	topologies.configureAnglePotential(
		n2 + "_bound", n2 + "_y", n1 + "_bound", angle);
	topologies.configureAnglePotential(
		n1 + "_y", n1 + "_bound", n2 + "_y", angle);
	topologies.configureAnglePotential(
		n2 + "_y", n2 + "_bound", n1 + "_y", angle);

	topologies.configureAnglePotential(
		n1 + "_bound", n1 + "_bound_y", n2 + "_bound", angle);
	topologies.configureAnglePotential(
		n2 + "_bound", n2 + "_bound_y", n1 + "_bound", angle);
	topologies.configureAnglePotential(
		n1 + "_bound_y", n1 + "_bound", n2 + "_bound_y", angle);
	topologies.configureAnglePotential(
		n2 + "_bound_y", n2 + "_bound", n1 + "_bound_y", angle);

	readdy::api::TorsionAngle tangle;
	tangle.forceConstant = 500;
	tangle.multiplicity = 1;
	tangle.phi_0 = 0;

	topologies.configureTorsionPotential(
		n1 + "_bound_x", n1 + "_bound", n2 + "_bound", n2 + "_x", tangle);
	topologies.configureTorsionPotential(
		n1 + "_bound_z", n1 + "_bound", n2 + "_bound", n2 + "_z", tangle);

	topologies.configureTorsionPotential(
		n1 + "_bound_x", n1 + "_bound", n2 + "_bound", n2 + "_bound_x", tangle);
	topologies.configureTorsionPotential(
		n1 + "_bound_z", n1 + "_bound", n2 + "_bound", n2 + "_bound_z", tangle);
}

std::vector<readdy::model::top::graph::Vertex*> get_particle_and_basis_vertices(
	std::string type, readdy::model::top::GraphTopology& top)
{
	std::vector<readdy::model::top::graph::Vertex*> out_vertices;
	out_vertices = std::vector<readdy::model::top::graph::Vertex*>(4);

	for(auto vert = top.graph().vertices().begin();
		vert != top.graph().vertices().end(); ++vert)
	{
		std::string ptype = top.context().particleTypes().nameOf(top.particleForVertex(vert).type());
		if(ptype.find(type) != std::string::npos)
		{
			if(ptype.find("x") != std::string::npos) { out_vertices[1] = &(*vert); }
			else if(ptype.find("y") != std::string::npos) { out_vertices[2] = &(*vert); }
			else if(ptype.find("z") != std::string::npos) { out_vertices[3] = &(*vert); }
			else { out_vertices[0] = &(*vert); }
		}
	}

	return out_vertices;
}

Eigen::Matrix3d get_rotation_matrix(
	std::vector<readdy::model::top::graph::Vertex*> vertices,
	std::string type, readdy::model::top::GraphTopology& topology
)
{
	auto pos = topology.particleForVertex(*(vertices[0])).pos();
	auto x_pos = topology.particleForVertex(*(vertices[1])).pos();
	auto y_pos = topology.particleForVertex(*(vertices[2])).pos();
	auto z_pos = topology.particleForVertex(*(vertices[3])).pos();
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

void add_oriented_particle(readdy::Simulation& sim,
	std::string name, Eigen::Vector3d pos)
{
	std::string cxn, cyn, czn, tn, bn;
	cxn = name + "_x";
	cyn = name + "_y";
	czn = name + "_z";
	tn = "complex_" + name;
	bn = name + "_bound";

	auto rpos = readdy::Vec3(pos[0], pos[1], pos[2]);

	std::vector<readdy::model::TopologyParticle> tp;
	tp.push_back(sim.createTopologyParticle(
		name, rpos));
	tp.push_back(sim.createTopologyParticle(
		cxn , readdy::Vec3(1,0,0) + rpos));
	tp.push_back(sim.createTopologyParticle(
		cyn, readdy::Vec3(0,1,0) + rpos));
	tp.push_back(sim.createTopologyParticle(
		czn, readdy::Vec3(0,0,1) + rpos));

	auto tp_inst = sim.addTopology(tn, tp);
	tp_inst->graph().addEdgeBetweenParticles(0,1);
	tp_inst->graph().addEdgeBetweenParticles(0,2);
	tp_inst->graph().addEdgeBetweenParticles(0,3);

	tp_inst->graph().addEdgeBetweenParticles(1,2);
	tp_inst->graph().addEdgeBetweenParticles(2,3);
	tp_inst->graph().addEdgeBetweenParticles(3,1);
}

void add_parent_child_bind_reaction(readdy::Simulation& sim,
	std::string& parent, std::string& child)
{
	float srx_rate = 1;
	float srx_tolerance = 10;

	std::string srx_def = "attach_" + parent + child +
		": complex_" + child +  "(" + child + ")" +
		" + complex_" + parent + "(" + parent + ")" +
		" -> complex_" + child + parent +
		"(" + child + "_bound--" + parent + "_bound) [self=false]";

	auto &topologies = sim.context().topologyRegistry();
	topologies.addSpatialReaction(srx_def, srx_rate, srx_tolerance);

	position_offset << 0.0, 2.0, 0.0;
	auto bind_rx_func = [&](readdy::model::top::GraphTopology &top) {
		readdy::model::top::reactions::Recipe recipe(top);

		auto child_vertices = get_particle_and_basis_vertices(child, top);
		auto child_rotation_matrix = get_rotation_matrix(child_vertices, child, top);

		auto parent_vertices = get_particle_and_basis_vertices(parent, top);
		auto parent_rotation_matrix = get_rotation_matrix(parent_vertices, parent, top);

		auto child_position = top.particleForVertex(*(child_vertices[0])).pos();
		Eigen::Vector3d cp;
		cp << child_position[0], child_position[1], child_position[2];

		auto parent_position = top.particleForVertex(*(parent_vertices[0])).pos();
		Eigen::Vector3d pp;
		pp << parent_position[0] , parent_position[1] , parent_position[2];

		cp = pp + parent_rotation_matrix * position_offset;

		Eigen::Matrix3d child_inv_rot = child_rotation_matrix.inverse();
		auto child_to_parent_rotation =
			parent_rotation_matrix * child_inv_rot;

		for(std::size_t i = 0; i < 3; ++i)
		{
			child_position[i] = cp[i];
		}
		recipe.changeParticlePosition(*(child_vertices[0]), child_position);

		for(std::size_t i = 1; i < child_vertices.size(); ++i)
		{
			auto new_position = child_position;
			Eigen::Vector3d pos = child_rotation_matrix.col(i-1);
			Eigen::Vector3d fcp = cp + child_to_parent_rotation * pos;
			for(std::size_t i = 0; i < 3; ++i)
			{
				new_position[i] = fcp[i];
			}

			recipe.changeParticlePosition(*(child_vertices[i]), new_position);
		}

		for(std::size_t i = 1; i < parent_vertices.size(); ++i)
		{
			auto new_position = parent_position;
			Eigen::Vector3d pos = parent_rotation_matrix.col(i-1);
			Eigen::Vector3d fpp = pp + pos;
			for(std::size_t i = 0; i < 3; ++i)
			{
				new_position[i] = fpp[i];
			}

			recipe.changeParticlePosition(*(parent_vertices[i]), new_position);
		}

		recipe.addEdge(*(child_vertices[1]), *(parent_vertices[1]));
		recipe.addEdge(*(child_vertices[3]), *(parent_vertices[3]));
		recipe.addEdge(*(child_vertices[0]), *(parent_vertices[2]));
		recipe.changeTopologyType("complex_" + child + parent + "_bound");
		printf("(child) %s bound to (parent) %s, correcting orientation particles.\n",
		 child.c_str(), parent.c_str());

		recipe.changeParticleType(*(child_vertices[1]), child + "_bound_x");
		recipe.changeParticleType(*(child_vertices[2]), child + "_bound_y");
		recipe.changeParticleType(*(child_vertices[3]), child + "_bound_z");
		recipe.changeParticleType(*(parent_vertices[1]), parent + "_bound_x");
		recipe.changeParticleType(*(parent_vertices[2]), parent + "_bound_y");
		recipe.changeParticleType(*(parent_vertices[3]), parent + "_bound_z");
		return recipe;
	};

	readdy::model::top::reactions::StructuralTopologyReaction bindrx(
		bind_rx_func, 1e30 // make it happen ASAP
	);

	topologies.addStructuralReaction("complex_" + child + parent, bindrx);
}

/**
*	Helix
**/
void configureSubHelixPotential(
	readdy::Simulation& sim,
	readdy::api::Angle angle,
	std::string A, std::string B, std::string C, std::string D)
{
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
	readdy::api::Angle angle,
	readdy::api::Bond bond,
	std::string A, std::string B, std::string C, std::string D,
	std::string E, std::string F)
{
	configureSubHelixPotential(
		sim, angle, A, B, C, D
	);
	configureSubHelixPotential(
		sim, angle, C, D, E, F
	);
	configureSubHelixPotential(
		sim, angle, E, F, A, B
	);

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

void configureHelixPotential(
	readdy::Simulation& sim,
	readdy::api::Angle angle,
	readdy::api::Bond bond,
	std::vector<std::string> helix_types
)
{
	if(helix_types.size() != 6)
	{
		printf("A Helix potential must have 6 particle types.\n");
		return;
	}

	configureHelixPotential(sim, angle, bond,
		helix_types[0], helix_types[1], helix_types[2],
		helix_types[3], helix_types[4], helix_types[5]);
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
	readdy::Vec3 pos,
	std::string A, std::string B, std::string C, std::string D,
	std::string E, std::string F, std::string top_type
)
{
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

void create_helix_particle(
	readdy::Simulation& sim,
	std::size_t size,
	readdy::Vec3 pos,
	std::vector<std::string> helixt_types, std::string top_type
)
{
	if(helix_types.size() != 6)
	{
		printf("A Helix potential must have 6 particle types.\n");
		return;
	}

	create_helix_particle(sim, size, pos,
		helix_types[0], helix_types[1], helix_types[2],
		helix_types[3], helix_types[4], helix_types[5], top_type);
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

				int t5 = v5->particleType() - helix_pid_0;
				int t4 = v4->particleType() - helix_pid_0;
				bool increasing = is_helix_dir_forward(t5, t4);

				int tc = v2->particleType() - helix_pid_0;
				int sign = increasing ? 1 : -1;

				int nt;
				nt = tc + sign * 1;
				nt = nt % 6;
				if(nt < 0) { nt += 6; }
				if(nt > 5) { nt -= 6; }

				recipe.changeParticleType(uv, nt + helix_pid_0);
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

				int t3 = v3->particleType() - helix_pid_0;
				int t5 = v5->particleType() - helix_pid_0;
				bool increasing = is_helix_dir_forward(t5, t3);

				std::vector<int> ids = {
					uvn->particleType(), v3->particleType()
				};

				int tc = increasing ?
					*std::max_element(ids.begin(), ids.end()) - helix_pid_0 :
					*std::min_element(ids.begin(), ids.end()) - helix_pid_0;
				int sign = increasing ? 1 : -1;

				int nt;
				nt = tc + sign * 1;
				nt = nt % 6;
				if(nt < 0) { nt += 6; }
				if(nt > 5) { nt -= 6; }

				recipe.changeParticleType(uv, nt + helix_pid_0);
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
