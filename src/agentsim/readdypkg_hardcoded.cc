#include "agentsim/agents/agent.h"
#include "agentsim/simpkg/readdypkg.h"
#include <algorithm>
#include <csignal>
#include <math.h>
#include <stdlib.h>
#include <time.h>

void add_oriented_species(
    readdy::Simulation* sim,
    std::string name, float diff_coeff,
    std::vector<std::string>& out_types,
    std::vector<float>& out_radii);

void add_oriented_particle(
    readdy::Simulation* sim,
    std::string name,
    std::string topology_name,
    Eigen::Vector3d pos);

void add_filament_constraints(
    readdy::Simulation* sim,
    std::string base_name);

void create_dimerize_rxn(
    readdy::Simulation* sim);

void create_nucleate_rxn(
    readdy::Simulation* sim);

void create_polymerize_rxn(
    readdy::Simulation* sim);

readdy::model::top::graph::Vertex* get_orientation_vertex(
    readdy::model::top::GraphTopology& top);

readdy::model::top::graph::Vertex* get_new_vertex(
    readdy::model::top::GraphTopology& top);

readdy::model::top::graph::Vertex* get_vertex_to_connect_to_end(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* end_vertex);

readdy::model::top::graph::Vertex* get_neighbor_monomer_of_end(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* end_vertex);

readdy::model::top::graph::Vertex* get_last_vertex_of_same_type_as_end(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* end_vertex);

readdy::model::top::graph::Vertex* get_previous_vertex_on_same_chain(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* vertex);

void remove_basis_particles(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::reactions::Recipe& recipe,
    readdy::model::top::graph::Vertex* vertex);

void rename_basis_particles(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::reactions::Recipe& recipe,
    readdy::model::top::graph::Vertex* vertex,
    std::string new_name);

void rename_new_vertex(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::reactions::Recipe& recipe,
    readdy::model::top::graph::Vertex* vertex);

void position_particle_in_filament(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::reactions::Recipe& recipe,
    readdy::model::top::graph::Vertex* orientation_vertex,
    readdy::model::top::graph::Vertex* end_vertex);

void save_filament_rotation(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* orientation_vertex);

void ortho_normalize_matrix(
    Eigen::Matrix3d& m);

Eigen::Matrix3d get_rotation_matrix(
    std::vector<Eigen::Vector3d> basis_positions);

std::vector<Eigen::Vector3d> get_basis_positions(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* main_vertex);

std::vector<readdy::model::top::graph::Vertex*> get_basis_vertices(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* main_vertex);

void toggle_end_marker(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::reactions::Recipe& recipe,
    readdy::model::top::graph::Vertex* vertex,
    bool end);

/**
*	Unscoped variables#include <math.h>
**/

// helix
std::string mol_name = "A";
int n_helix_types = 6;
float mol_radius = 3;
float diffusion_constant_monomer = 3;
float diffusion_constant = 0.3;
float force_constant = 150;
float dimerize_rate = 0.003;
float nucleate_rate = 1000;
float polymerize_rate = 1000;
float reaction_distance = 10;
int boxSize = 400;
int n_monomers = 3010.0 / pow(1000.0 / boxSize, 3.0);

std::vector<std::string> particle_types;
std::vector<float> particle_radii;
std::map<readdy::model::top::graph::Vertex*, Eigen::Matrix3d> filament_rotations;

bool log_events = false;

/**
*	Simulation API
**/
namespace aics {
namespace agentsim {

    void ReaDDyPkg::InitAgents(std::vector<std::shared_ptr<Agent>>& agents, Model& model)
    {
        for (std::size_t i = 0; i < 1000; ++i) {
            std::shared_ptr<Agent> agent;
            agent.reset(new Agent());
            agent->SetVisibility(false);
            agent->SetCollisionRadius(mol_radius);
            agents.push_back(agent);
        }
    }

    void ReaDDyPkg::InitReactions(Model& model) {}

    void ReaDDyPkg::Setup()
    {
        if (!this->m_agents_initialized) {
            this->m_simulation->context().boxSize()[0] = boxSize;
            this->m_simulation->context().boxSize()[1] = boxSize;
            this->m_simulation->context().boxSize()[2] = boxSize;

            //define particles
            auto& particles = this->m_simulation->context().particleTypes();

            add_oriented_species(this->m_simulation, mol_name + "0", diffusion_constant_monomer, particle_types, particle_radii);

            for (std::size_t i = 1; i <= n_helix_types; ++i) {
                if (i == 1) {
                    add_oriented_species(this->m_simulation, mol_name + std::to_string(i), 0, particle_types, particle_radii);
                } else {
                    // add particles for other filament monomers
                    particles.addTopologyType(mol_name + std::to_string(i), diffusion_constant);
                    particle_types.push_back(mol_name + std::to_string(i));
                    particle_radii.push_back(mol_radius);
                }

                if (i > 0) {
                    // add 'end' particles for monomers at reactive end of filament
                    particles.addTopologyType(mol_name + std::to_string(i) + "_end", diffusion_constant);
                    particle_types.push_back(mol_name + std::to_string(i) + "_end");
                    particle_radii.push_back(mol_radius);
                }
            }

            //for decay of basis particles when a monomer joins a filament
            particles.add("basis", diffusion_constant);
            this->m_simulation->context().reactions().addDecay("Basis_decay", "basis", 1e30);

            //define topologies
            auto& topologies = this->m_simulation->context().topologyRegistry();
            topologies.addType("Monomer");
            topologies.addType("Dimer_binding");
            topologies.addType("Dimer");
            topologies.addType("Polymer_nucleating");
            topologies.addType("Polymer_growing");
            topologies.addType("Polymer");

            add_filament_constraints(this->m_simulation, mol_name);

            //define reactions
            topologies.addSpatialReaction(
                "Dimerize: Monomer(A0) + Monomer(A0) -> Dimer_binding(A1--A0) [self=true]", dimerize_rate, 100);
            topologies.addSpatialReaction(
                "Nucleate: Monomer(A0) + Dimer(A3_end) -> Polymer_nucleating(A0--A3)", nucleate_rate, reaction_distance);
            int i_plus_2;
            for (std::size_t i = 1; i <= n_helix_types; ++i) {
                i_plus_2 = i < n_helix_types - 1 ? i + 2 : i + 2 - n_helix_types;
                topologies.addSpatialReaction(
                    "Polymerize" + std::to_string(i) + ": Monomer(A0) + Polymer(A" + std::to_string(i)
                        + "_end) -> Polymer_growing(A0--A" + std::to_string(i) + ")",
                    polymerize_rate, reaction_distance);
            }
            create_dimerize_rxn(this->m_simulation);
            create_nucleate_rxn(this->m_simulation);
            create_polymerize_rxn(this->m_simulation);

            //define repulsions
            auto& potentials = this->m_simulation->context().potentials();
            this->m_agents_initialized = true;
            for (std::size_t i = 0; i <= n_helix_types; ++i) {
                potentials.addHarmonicRepulsion(mol_name + "0", mol_name + std::to_string(i), force_constant, 2 * mol_radius);
                if (i > 0) {
                    potentials.addHarmonicRepulsion(mol_name + "0", mol_name + std::to_string(i) + "_end", force_constant, 2 * mol_radius);
                }
                potentials.addBox(mol_name + std::to_string(i), force_constant,
                    readdy::Vec3(-boxSize / 2, -boxSize / 2, -boxSize / 2), readdy::Vec3(boxSize, boxSize, boxSize));
            }
        }

        //add initial monomers
        for (std::size_t i = 0; i < n_monomers; ++i) {
            float x, y, z;
            x = rand() % boxSize - boxSize / 2;
            y = rand() % boxSize - boxSize / 2;
            z = rand() % boxSize - boxSize / 2;

            add_oriented_particle(this->m_simulation, mol_name + "0", "Monomer", Eigen::Vector3d(x, y, z));
        }

        auto loop = this->m_simulation->createLoop(1);
        loop.runInitialize();
        loop.runInitializeNeighborList();
    }

    void ReaDDyPkg::Shutdown()
    {
        delete this->m_simulation;
        this->m_simulation = nullptr;
        this->m_simulation = new readdy::Simulation("SingleCPU");

        this->m_timeStepCount = 0;
        this->m_agents_initialized = false;
        this->m_reactions_initialized = false;
        this->m_hasAlreadyRun = false;
        this->m_hasFinishedStreaming = false;
        ResetFileIO();
    }

    void ReaDDyPkg::RunTimeStep(
        float timeStep, std::vector<std::shared_ptr<Agent>>& agents)
    {
        auto loop = this->m_simulation->createLoop(timeStep);

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
        for (std::size_t i = 0; i < pTypes.size(); ++i) {
            std::string pt = pTypes[i];
            if (pt.find("_x") != std::string::npos
                || pt.find("_y") != std::string::npos
                || pt.find("_z") != std::string::npos) {
                continue;
            }

            std::vector<readdy::Vec3> positions = this->m_simulation->getParticlePositions(pTypes[i]);
            std::vector<readdy::Vec3> xpositions;
            std::vector<readdy::Vec3> ypositions;
            std::vector<readdy::Vec3> zpositions;

            bool has_orientation = false;
            if (std::find(pTypes.begin(), pTypes.end(), pTypes[i] + "_x") != pTypes.end()) {
                has_orientation = true;
                xpositions = this->m_simulation->getParticlePositions(pTypes[i] + "_x");
                ypositions = this->m_simulation->getParticlePositions(pTypes[i] + "_y");
                zpositions = this->m_simulation->getParticlePositions(pTypes[i] + "_z");
            }

            for (std::size_t j = 0; j < positions.size(); ++j) {
                readdy::Vec3 v = positions[j];
                std::shared_ptr<Agent> newAgent;
                newAgent.reset(new Agent());
                newAgent->SetName(pTypes[i]);
                newAgent->SetTypeID(i);
                newAgent->SetLocation(Eigen::Vector3d(v[0], v[1], v[2]));
                newAgent->SetCollisionRadius(0.5f);

                if (has_orientation) {
                    readdy::Vec3 x = xpositions[j];
                    readdy::Vec3 y = ypositions[j];
                    readdy::Vec3 z = zpositions[j];

                    std::vector<Eigen::Vector3d> basis;
                    basis.push_back(newAgent->GetLocation());
                    basis.push_back(Eigen::Vector3d(x[0], x[1], x[2]));
                    basis.push_back(Eigen::Vector3d(y[0], y[1], y[2]));
                    basis.push_back(Eigen::Vector3d(z[0], z[1], z[2]));
                    Eigen::Matrix3d rm = get_rotation_matrix(basis);
                    Eigen::Vector3d rea = rm.eulerAngles(0, 1, 2);
                    newAgent->SetRotation(rea);
                }

                agents.push_back(newAgent);
            }
        }
    }

    void ReaDDyPkg::UpdateParameter(std::string paramName, float paramValue)
    {
        static const int num_params = 3;
        std::string recognized_params[num_params] = {
            "Nucleation Rate", "Growth Rate", "Dissociation Rate"
        };
        std::size_t paramIndex = 252; // assuming less than 252 parameters

        for (std::size_t i = 0; i < num_params; ++i) {
            if (recognized_params[i] == paramName) {
                paramIndex = i;
                break;
            }
        }

        if (paramIndex == 252) {
            printf("Unrecognized parameter %s passed into ReaddyPkg.\n", paramName.c_str());
            return;
        }

        switch (paramIndex) {
        case 0: // NucleationRate
        {

        } break;
        case 1: // GrowthRate
        {

        } break;
        case 2: // DissociationRate
        {

        } break;
        default: {
            printf("Recognized but unimplemented parameter %s in ReaDDy SimPkg, resolved as index %lu.\n",
                paramName.c_str(), paramIndex);
            return;
        } break;
        }
    }

} // namespace agentsim
} // namespace aics

/**
*	Helper function implementations
**/
void add_oriented_species(
    readdy::Simulation* sim,
    std::string name,
    float diff_coeff,
    std::vector<std::string>& out_types,
    std::vector<float>& out_radii)
{
    //	if(diff_coeff > 1.0f)
    //	{
    //		return;
    //		printf("add_oriented_species failed: diffusion coefficient must be less than 1. \n");
    //	}

    std::string cxn, cyn, czn;
    auto& particles = sim->context().particleTypes();
    float component_radii = 0.1f;

    particles.addTopologyType(name, diff_coeff);
    out_types.push_back(name);
    out_radii.push_back(mol_radius);

    cxn = name + "_x";
    particles.addTopologyType(cxn, diff_coeff);
    out_types.push_back(cxn);
    out_radii.push_back(component_radii);

    cyn = name + "_y";
    particles.addTopologyType(cyn, diff_coeff);
    out_types.push_back(cyn);
    out_radii.push_back(component_radii);

    czn = name + "_z";
    particles.addTopologyType(czn, diff_coeff);
    out_types.push_back(czn);
    out_radii.push_back(component_radii);

    //bond constraints
    readdy::api::Bond bond;
    bond.forceConstant = force_constant;
    auto& topologies = sim->context().topologyRegistry();

    bond.length = 1;
    topologies.configureBondPotential(name, cxn, bond);
    topologies.configureBondPotential(name, cyn, bond);
    topologies.configureBondPotential(name, czn, bond);
    bond.length = M_SQRT2;
    topologies.configureBondPotential(cxn, cyn, bond);
    topologies.configureBondPotential(cyn, czn, bond);
    topologies.configureBondPotential(czn, cxn, bond);

    //angle constraints
    readdy::api::Angle angle;
    angle.forceConstant = force_constant;

    angle.equilibriumAngle = M_PI_2;
    topologies.configureAnglePotential(cxn, name, cyn, angle);
    topologies.configureAnglePotential(cxn, name, czn, angle);
    topologies.configureAnglePotential(cyn, name, czn, angle);
    angle.equilibriumAngle = M_PI / 3;
    topologies.configureAnglePotential(cxn, cyn, czn, angle);
    topologies.configureAnglePotential(cyn, czn, cxn, angle);
    topologies.configureAnglePotential(czn, cxn, cyn, angle);
}

void add_oriented_particle(
    readdy::Simulation* sim,
    std::string name,
    std::string topology_name,
    Eigen::Vector3d pos)
{
    std::string cxn, cyn, czn;
    cxn = name + "_x";
    cyn = name + "_y";
    czn = name + "_z";

    auto rpos = readdy::Vec3(pos[0], pos[1], pos[2]);

    std::vector<readdy::model::TopologyParticle> tp;
    tp.push_back(sim->createTopologyParticle(
        name, rpos));
    tp.push_back(sim->createTopologyParticle(
        cxn, readdy::Vec3(1, 0, 0) + rpos));
    tp.push_back(sim->createTopologyParticle(
        cyn, readdy::Vec3(0, 1, 0) + rpos));
    tp.push_back(sim->createTopologyParticle(
        czn, readdy::Vec3(0, 0, 1) + rpos));

    auto tp_inst = sim->addTopology(topology_name, tp);
    tp_inst->graph().addEdgeBetweenParticles(0, 1);
    tp_inst->graph().addEdgeBetweenParticles(0, 2);
    tp_inst->graph().addEdgeBetweenParticles(0, 3);

    tp_inst->graph().addEdgeBetweenParticles(1, 2);
    tp_inst->graph().addEdgeBetweenParticles(2, 3);
    tp_inst->graph().addEdgeBetweenParticles(3, 1);
}

void add_filament_constraints(
    readdy::Simulation* sim,
    std::string base_name)
{
    auto& topologies = sim->context().topologyRegistry();

    readdy::api::Bond bond1;
    bond1.forceConstant = force_constant;
    bond1.length = 3.72;
    readdy::api::Bond bond2;
    bond2.forceConstant = force_constant;
    bond2.length = 5.54;
    readdy::api::Bond bond3;
    bond3.forceConstant = force_constant;
    bond3.length = 16.6;

    readdy::api::Angle angle1;
    angle1.forceConstant = force_constant;
    angle1.equilibriumAngle = 1.68;
    readdy::api::Angle angle2;
    angle2.forceConstant = force_constant;
    angle2.equilibriumAngle = 0.73;
    readdy::api::Angle angle3;
    angle3.forceConstant = force_constant;
    angle3.equilibriumAngle = 3;

    int i_plus_1, i_plus_2;
    std::string i_str, i_plus_1_str, i_plus_2_str;
    for (std::size_t i = 1; i <= n_helix_types; ++i) {
        i_plus_1 = i < n_helix_types ? i + 1 : i + 1 - n_helix_types;
        i_plus_2 = i < n_helix_types - 1 ? i + 2 : i + 2 - n_helix_types;
        i_str = std::to_string(i);
        i_plus_1_str = std::to_string(i_plus_1);
        i_plus_2_str = std::to_string(i_plus_2);

        //during transitions, newly added particle is still A0
        topologies.configureBondPotential(base_name + "0",
            base_name + i_str, bond2);

        topologies.configureBondPotential(base_name + i_str,
            base_name + i_plus_1_str, bond1);
        topologies.configureBondPotential(base_name + i_str + "_end",
            base_name + i_plus_1_str, bond1);
        topologies.configureBondPotential(base_name + i_str,
            base_name + i_plus_1_str + "_end", bond1);
        topologies.configureBondPotential(base_name + i_str,
            base_name + i_plus_2_str, bond2);
        topologies.configureBondPotential(base_name + i_str,
            base_name + i_plus_2_str + "_end", bond2);
        topologies.configureBondPotential(base_name + i_str,
            base_name + i_str, bond3);
        topologies.configureBondPotential(base_name + i_str,
            base_name + i_str + "_end", bond3);

        topologies.configureAnglePotential(base_name + i_str,
            base_name + i_plus_1_str,
            base_name + i_plus_2_str, angle1);
        topologies.configureAnglePotential(base_name + i_str,
            base_name + i_plus_1_str + "_end",
            base_name + i_plus_2_str, angle1);
        topologies.configureAnglePotential(base_name + i_str,
            base_name + i_plus_1_str,
            base_name + i_plus_2_str + "_end", angle1);
        topologies.configureAnglePotential(base_name + i_str,
            base_name + i_plus_2_str,
            base_name + i_plus_1_str, angle2);
        topologies.configureAnglePotential(base_name + i_str,
            base_name + i_plus_2_str,
            base_name + i_plus_1_str + "_end", angle2);
        topologies.configureAnglePotential(base_name + i_plus_1_str,
            base_name + i_str,
            base_name + i_plus_2_str, angle2);
        topologies.configureAnglePotential(base_name + i_plus_1_str + "_end",
            base_name + i_str,
            base_name + i_plus_2_str, angle2);
        topologies.configureAnglePotential(base_name + i_plus_1_str,
            base_name + i_str,
            base_name + i_plus_2_str + "_end", angle2);
        topologies.configureAnglePotential(base_name + i_str,
            base_name + i_str,
            base_name + i_str, angle3);
        topologies.configureAnglePotential(base_name + i_str,
            base_name + i_str,
            base_name + i_str + "_end", angle3);
    }
    //during dimer transition, A0 becomes A1 before bases can be renamed
    bond1.length = 1;
    topologies.configureBondPotential("A1", "A0_x", bond1);
    topologies.configureBondPotential("A1", "A0_y", bond1);
    topologies.configureBondPotential("A1", "A0_z", bond1);
}

void create_dimerize_rxn(
    readdy::Simulation* sim)
{
    auto dimerize = [&](readdy::model::top::GraphTopology& top) {
        if (log_events) {
            printf("dimerize!!\n");
        }

        readdy::model::top::reactions::Recipe recipe(top);

        recipe.changeTopologyType("Dimer");

        readdy::model::top::graph::Vertex* v1 = get_orientation_vertex(top);
        readdy::model::top::graph::Vertex* v_new = get_new_vertex(top);

        remove_basis_particles(top, recipe, v_new);
        rename_basis_particles(top, recipe, v1, mol_name + "1");
        recipe.changeParticleType(*v_new, mol_name + "3_end");
        save_filament_rotation(top, v1);
        position_particle_in_filament(top, recipe, v1, v_new);

        return recipe;
    };
    readdy::model::top::reactions::StructuralTopologyReaction dimerize_rxn(
        dimerize, 1e30);
    sim->context().topologyRegistry().addStructuralReaction(
        "Dimer_binding", dimerize_rxn);
}

void create_nucleate_rxn(
    readdy::Simulation* sim)
{
    auto nucleate = [&](readdy::model::top::GraphTopology& top) {
        if (log_events) {
            printf("nucleate!!\n");
        }

        readdy::model::top::reactions::Recipe recipe(top);

        recipe.changeTopologyType("Polymer");

        readdy::model::top::graph::Vertex* v1 = get_orientation_vertex(top);
        readdy::model::top::graph::Vertex* v_new = get_new_vertex(top);

        remove_basis_particles(top, recipe, v_new);
        recipe.changeParticleType(*v_new, mol_name + "2_end");
        position_particle_in_filament(top, recipe, v1, v_new);
        recipe.addEdge(*v1, *v_new);

        return recipe;
    };
    readdy::model::top::reactions::StructuralTopologyReaction nucleate_rxn(
        nucleate, 1e30);
    sim->context().topologyRegistry().addStructuralReaction(
        "Polymer_nucleating", nucleate_rxn);
}

void create_polymerize_rxn(
    readdy::Simulation* sim)
{
    auto polymerize = [&](readdy::model::top::GraphTopology& top) {
        int n_monomers = top.graph().vertices().size() - 6;

        if (log_events) {
            printf("polymerize!! %d\n", n_monomers);
        }

        readdy::model::top::reactions::Recipe recipe(top);

        recipe.changeTopologyType("Polymer");

        readdy::model::top::graph::Vertex* v1 = get_orientation_vertex(top);
        readdy::model::top::graph::Vertex* v_new = get_new_vertex(top);
        readdy::model::top::graph::Vertex* v_connect = get_vertex_to_connect_to_end(top, v_new);

        remove_basis_particles(top, recipe, v_new);
        rename_new_vertex(top, recipe, v_new);
        position_particle_in_filament(top, recipe, v1, v_new);
        recipe.addEdge(*v_new, *v_connect);
        toggle_end_marker(top, recipe, v_connect, true);

        if (n_monomers > 6) {
            readdy::model::top::graph::Vertex* v_same_type = get_last_vertex_of_same_type_as_end(top, v_new);
            recipe.addEdge(*v_new, *v_same_type);
        }

        return recipe;
    };
    readdy::model::top::reactions::StructuralTopologyReaction polymerize_rxn(
        polymerize, 1e30);
    sim->context().topologyRegistry().addStructuralReaction(
        "Polymer_growing", polymerize_rxn);
}

readdy::model::top::graph::Vertex* get_orientation_vertex(
    readdy::model::top::GraphTopology& top)
{
    std::string ptype;
    for (auto vert = top.graph().vertices().begin(); vert != top.graph().vertices().end(); ++vert) {
        ptype = top.context().particleTypes().nameOf(top.particleForVertex(vert).type());
        if (ptype.find(mol_name + "1") != std::string::npos) {
            for (std::size_t i = 0; i < vert->neighbors().size(); ++i) {
                ptype = top.context().particleTypes().nameOf(top.particleForVertex(vert->neighbors()[i]).type());
                if (ptype.find("_x") != std::string::npos || ptype.find("_y") != std::string::npos
                    || ptype.find("_z") != std::string::npos) {
                    return &(*vert);
                }
            }
        }
    }
    return nullptr;
}

readdy::model::top::graph::Vertex* get_new_vertex(
    readdy::model::top::GraphTopology& top)
{
    for (auto vert = top.graph().vertices().begin(); vert != top.graph().vertices().end(); ++vert) {
        std::string ptype = top.context().particleTypes().nameOf(top.particleForVertex(vert).type());
        if (ptype == mol_name + "0") {
            return &(*vert);
        }
    }
    return nullptr;
}

readdy::model::top::graph::Vertex* get_vertex_to_connect_to_end(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* end_vertex)
{
    readdy::model::top::graph::Vertex* neighbor_monomer = get_neighbor_monomer_of_end(top, end_vertex);
    if (neighbor_monomer != NULL) {
        std::string neighbor_type = top.context().particleTypes().nameOf(top.particleForVertex(*neighbor_monomer).type());
        int n;
        std::stringstream(neighbor_type.substr(mol_name.length(), 1)) >> n;
        int n_plus_1 = n < n_helix_types ? n + 1 : n + 1 - n_helix_types;
        std::string goal_type = mol_name + std::to_string(n_plus_1);

        for (std::size_t i = 0; i < neighbor_monomer->neighbors().size(); ++i) {
            std::string ptype = top.context().particleTypes().nameOf(top.particleForVertex(neighbor_monomer->neighbors()[i]).type());
            if (ptype.find(mol_name + std::to_string(n_plus_1)) != std::string::npos) {
                return &(*neighbor_monomer->neighbors()[i]);
            }
        }
    }
    return nullptr;
}

readdy::model::top::graph::Vertex* get_neighbor_monomer_of_end(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* end_vertex)
{
    readdy::model::top::graph::Vertex* neighbor_monomer;
    for (std::size_t i = 0; i < end_vertex->neighbors().size(); ++i) {
        std::string ptype = top.context().particleTypes().nameOf(top.particleForVertex(end_vertex->neighbors()[i]).type());
        if (ptype.find(mol_name + "0") == std::string::npos) {
            return &(*end_vertex->neighbors()[i]);
        }
    }
    return nullptr;
}

readdy::model::top::graph::Vertex* get_last_vertex_of_same_type_as_end(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* end_vertex)
{
    readdy::model::top::graph::Vertex* neighbor_monomer = get_neighbor_monomer_of_end(top, end_vertex);

    std::string neighbor_type = top.context().particleTypes().nameOf(top.particleForVertex(*neighbor_monomer).type());
    int n;
    std::stringstream(neighbor_type.substr(mol_name.length(), 1)) >> n;
    int n_plus_2 = n < n_helix_types - 1 ? n + 2 : n + 2 - n_helix_types;
    std::string goal_type = mol_name + std::to_string(n_plus_2);

    while (neighbor_type != goal_type) {
        neighbor_monomer = get_previous_vertex_on_same_chain(top, neighbor_monomer);
        neighbor_type = top.context().particleTypes().nameOf(top.particleForVertex(*neighbor_monomer).type());
    }
    return neighbor_monomer;
}

readdy::model::top::graph::Vertex* get_previous_vertex_on_same_chain(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* vertex)
{
    std::string ptype = top.context().particleTypes().nameOf(top.particleForVertex(*vertex).type());
    int n;
    std::stringstream(ptype.substr(mol_name.length(), 1)) >> n;
    int n_minus_2 = n > 2 ? n - 2 : n - 2 + n_helix_types;

    for (std::size_t i = 0; i < vertex->neighbors().size(); ++i) {
        std::string neighbor_type = top.context().particleTypes().nameOf(top.particleForVertex(vertex->neighbors()[i]).type());
        if (neighbor_type.find(mol_name + std::to_string(n_minus_2)) != std::string::npos) {
            return &(*vertex->neighbors()[i]);
        }
    }

    return nullptr;
}

void remove_basis_particles(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::reactions::Recipe& recipe,
    readdy::model::top::graph::Vertex* vertex)
{
    std::vector<readdy::model::top::graph::Vertex*> basis_vertices = get_basis_vertices(top, vertex);

    for (std::size_t i = 1; i < basis_vertices.size(); ++i) {
        recipe.separateVertex(*basis_vertices[i]);
        recipe.changeParticleType(*basis_vertices[i], "basis");
    }
}

void rename_basis_particles(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::reactions::Recipe& recipe,
    readdy::model::top::graph::Vertex* vertex,
    std::string new_name)
{
    std::vector<readdy::model::top::graph::Vertex*> basis_vertices = get_basis_vertices(top, vertex);
    recipe.changeParticleType(*basis_vertices[1], new_name + "_x");
    recipe.changeParticleType(*basis_vertices[2], new_name + "_y");
    recipe.changeParticleType(*basis_vertices[3], new_name + "_z");
}

void rename_new_vertex(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::reactions::Recipe& recipe,
    readdy::model::top::graph::Vertex* vertex)
{
    readdy::model::top::graph::Vertex* neighbor_monomer = get_neighbor_monomer_of_end(top, vertex);
    if (neighbor_monomer != NULL) {
        std::string neighbor_type = top.context().particleTypes().nameOf(top.particleForVertex(*neighbor_monomer).type());
        int n;
        std::stringstream(neighbor_type.substr(mol_name.length(), 1)) >> n;
        int n_plus_2 = n < n_helix_types - 1 ? n + 2 : n + 2 - n_helix_types;
        recipe.changeParticleType(*vertex, mol_name + std::to_string(n_plus_2));
    }
}

void position_particle_in_filament(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::reactions::Recipe& recipe,
    readdy::model::top::graph::Vertex* orientation_vertex,
    readdy::model::top::graph::Vertex* end_vertex)
{
    int n_monomers = top.graph().vertices().size() - 6;

    // at nucleation, #3 is added before #2
    if (n_monomers < 4) {
        n_monomers = n_monomers < 3 ? 3 : 2;
    }

    // get position offset relative to first monomer in filament
    Eigen::Vector3d first_axis_offset(0, 1.26, 0);
    Eigen::Vector3d axis_position(0, -1.26, 2.75 * (n_monomers - 1));
    Eigen::Matrix3d offset_rotation_matrix = Eigen::AngleAxisd((n_monomers % 2 == 0 ? -2.92 : 0) + -0.52 * floor((n_monomers - 1) / 2),
        Eigen::Vector3d(0, 0, 1))
                                                 .toRotationMatrix();
    auto offset = axis_position + offset_rotation_matrix * first_axis_offset;

    // get world position for local offset using first monomer's orientation
    auto filament_position_readdy = top.particleForVertex(*(orientation_vertex)).pos();
    Eigen::Vector3d filament_position(filament_position_readdy[0], filament_position_readdy[1], filament_position_readdy[2]);

    Eigen::Vector3d new_pos;
    auto filament_rotation_matrix = filament_rotations.find(orientation_vertex);
    if (filament_rotation_matrix != filament_rotations.end()) {
        new_pos = filament_position + filament_rotation_matrix->second * offset;
    } else {
        printf("couldn't find filament orientation!");
        new_pos = filament_position + offset;
    }

    readdy::Vec3 new_pos_readdy(new_pos[0], new_pos[1], new_pos[2]);
    recipe.changeParticlePosition(*end_vertex, new_pos_readdy);
}

void save_filament_rotation(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* orientation_vertex)
{
    filament_rotations.insert(std::map<readdy::model::top::graph::Vertex*, Eigen::Matrix3d>::value_type(
        orientation_vertex,
        get_rotation_matrix(get_basis_positions(top, orientation_vertex))));
}

void ortho_normalize_matrix(Eigen::Matrix3d& m)
{
    auto v1 = m.col(0).normalized();
    auto v2 = m.col(1).normalized();
    auto v3 = m.col(2).normalized();

    v2 = (v2 - v2.dot(v1) * v1).normalized();
    v3 = (v3 - v3.dot(v1) * v1).normalized();
    v3 = (v3 - v3.dot(v2) * v2).normalized();

    m << v1[0], v1[1], v1[2],
        v2[0], v2[1], v2[2],
        v3[0], v3[1], v3[2];
}

Eigen::Matrix3d get_rotation_matrix(
    std::vector<Eigen::Vector3d> basis_positions)
{
    std::vector<Eigen::Vector3d> basis_vectors = std::vector<Eigen::Vector3d>(3);
    basis_vectors[0] = (basis_positions[1] - basis_positions[0]).normalized();
    basis_vectors[1] = (basis_positions[2] - basis_positions[0]).normalized();
    basis_vectors[2] = (basis_positions[3] - basis_positions[0]).normalized();

    Eigen::Matrix3d rotation_matrix;
    rotation_matrix << basis_vectors[0][0], basis_vectors[0][1], basis_vectors[0][2],
        basis_vectors[1][0], basis_vectors[1][1], basis_vectors[1][2],
        basis_vectors[2][0], basis_vectors[2][1], basis_vectors[2][2];

    ortho_normalize_matrix(rotation_matrix);
    return rotation_matrix;
}

std::vector<Eigen::Vector3d> get_basis_positions(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* main_vertex)
{
    std::vector<readdy::model::top::graph::Vertex*> basis_vertices = get_basis_vertices(top, main_vertex);
    std::vector<Eigen::Vector3d> basis_positions = std::vector<Eigen::Vector3d>(4);
    readdy::Vec3 pos_readdy;

    for (std::size_t i = 0; i < basis_vertices.size(); ++i) {
        pos_readdy = top.particleForVertex(*(basis_vertices[i])).pos();
        basis_positions[i] = Eigen::Vector3d(pos_readdy[0], pos_readdy[1], pos_readdy[2]);
    }

    return basis_positions;
}

std::vector<readdy::model::top::graph::Vertex*> get_basis_vertices(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::graph::Vertex* main_vertex)
{
    std::vector<readdy::model::top::graph::Vertex*> out_vertices;
    out_vertices = std::vector<readdy::model::top::graph::Vertex*>(4);
    out_vertices[0] = main_vertex;
    for (std::size_t i = 0; i < main_vertex->neighbors().size(); ++i) {
        std::string ptype = top.context().particleTypes().nameOf(top.particleForVertex(main_vertex->neighbors()[i]).type());
        if (ptype.find("_x") != std::string::npos) {
            out_vertices[1] = &(*main_vertex->neighbors()[i]);
        } else if (ptype.find("_y") != std::string::npos) {
            out_vertices[2] = &(*main_vertex->neighbors()[i]);
        } else if (ptype.find("_z") != std::string::npos) {
            out_vertices[3] = &(*main_vertex->neighbors()[i]);
        }
    }
    return out_vertices;
}

void toggle_end_marker(
    readdy::model::top::GraphTopology& top,
    readdy::model::top::reactions::Recipe& recipe,
    readdy::model::top::graph::Vertex* vertex,
    bool end)
{
    std::string ptype = top.context().particleTypes().nameOf(top.particleForVertex(*vertex).type());
    if (!end) {
        if (ptype.find("_end") != std::string::npos) {
            recipe.changeParticleType(*vertex, ptype.substr(0, ptype.find("_")));
        }
    } else {
        if (ptype.find("_end") == std::string::npos) {
            recipe.changeParticleType(*vertex, ptype + "_end");
        }
    }
}
