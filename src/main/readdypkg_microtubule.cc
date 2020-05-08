#include "agentsim/readdy_models.h"
// #include <math.h>
// #include <stdlib.h>
// #include "readdy/kernel/singlecpu/SCPUKernel.h"
// #include "readdy/model/topologies/common.h"

namespace aics {
namespace agentsim {
namespace models {

/**
 * A method to add particle types and constraints to the ReaDDy context
 * @param context ReaDDy Context
 */
void addReaDDyMicrotubuleToSystem(
    readdy::model::Context &context,
    bool diffuse,
    std::shared_ptr<std::unordered_map<std::string,float>>& particleTypeRadiusMapping)
{
    float forceConstant = 90.;
    float eta = 8.1;
    float temperature = 37. + 273.15;

    // types
    auto &topologyRegistry = context.topologyRegistry();
    topologyRegistry.addType("Microtubule");

    auto &typeRegistry = context.particleTypes();
    std::vector<std::string> tubulinTypes = {
        "tubulinA#", "tubulinB#", "tubulinA#bound_", "tubulinB#bound_"};
    float diffCoeff = diffuse ? calculateDiffusionCoefficient(2., eta, temperature) : 0.;
    for (const auto &t : tubulinTypes)
    {
        addPolymerTopologySpecies(
            typeRegistry, t, 2., diffCoeff, particleTypeRadiusMapping);
    }

    // bonds between protofilaments
    addPolymerBond(topologyRegistry,
        tubulinTypes, {0, 0},
        tubulinTypes, {0, -1},
        forceConstant, 5.2);

    // bonds between rings
    addPolymerBond(topologyRegistry,
        tubulinTypes, {0, 0},
        tubulinTypes, {-1, 0},
        forceConstant, 4.);

    // angles
    forceConstant *= 2.;
    addPolymerAngle(topologyRegistry,
        tubulinTypes, {0, 1},
        tubulinTypes, {0, 0},
        tubulinTypes, {-1, 0},
        forceConstant, 1.75);
    addPolymerAngle(topologyRegistry,
        tubulinTypes, {0, 1},
        tubulinTypes, {0, 0},
        tubulinTypes, {1, 0},
        forceConstant, 1.40);
    addPolymerAngle(topologyRegistry,
        tubulinTypes, {0, -1},
        tubulinTypes, {0, 0},
        tubulinTypes, {-1, 0},
        forceConstant, 1.40);
    addPolymerAngle(topologyRegistry,
        tubulinTypes, {0, -1},
        tubulinTypes, {0, 0},
        tubulinTypes, {1, 0},
        forceConstant, 1.75);
    addPolymerAngle(topologyRegistry,
        tubulinTypes, {-1, 0},
        tubulinTypes, {0, 0},
        tubulinTypes, {1, 0},
        forceConstant, 3.14);
    addPolymerAngle(topologyRegistry,
        tubulinTypes, {0, -1},
        tubulinTypes, {0, 0},
        tubulinTypes, {0, 1},
        forceConstant, 2.67);

    // repulsions
    addPolymerRepulsion(context, tubulinTypes,
        75., 4.2);
}

/**
 * A method to get a list of topology particles in a microtubule
 * @param typeRegistry ReaDDY type registry
 * @param nFilaments protofilaments
 * @param nRings rings
 * @param radius of the microtubule [nm]
 */
std::vector<readdy::model::Particle> getMicrotubuleParticles(
    readdy::model::ParticleTypeRegistry &typeRegistry,
    int nFilaments,
    int nRings,
    float radius)
{
    std::vector<readdy::model::Particle> particles {};

    for(int filament = 0; filament < nFilaments; ++filament)
    {
        float angle = (nFilaments - filament) * 2. * M_PI / (float)nFilaments + M_PI / 2.;
        Eigen::Vector3d normal = Eigen::Vector3d(cos(angle), sin(angle), 0.);
        normal.normalize();
        Eigen::Vector3d tangent = Eigen::Vector3d(0., 0., 1.);
        Eigen::Vector3d pos = (12./13. * filament - 2. * nRings) * tangent + radius * normal;

        for(int ring = 0; ring < nRings; ++ring)
        {
            std::string number1 = std::to_string(ring % 3 + 1);
            std::string number2 = std::to_string(int(filament + floor(ring / 3.)) % 3 + 1);
            std::string a = ring % 2 == 0 ? "A#" : "B#";
            std::string type = "tubulin" + a + number1 + "_" + number2;

            particles.push_back({-pos[0], pos[1], pos[2], typeRegistry.idOf(type)});

            pos = pos + 4. * tangent;
        }
    }

    return particles;
}

/**
 * A method to add edges to a microtubule topology
 * @param graph ReaDDy topology graph
 * @param nFilaments protofilaments
 * @param nRings rings
 */
void addMicrotubuleEdges(
    readdy::model::top::GraphTopology &top,
    int nFilaments,
    int nRings)
{
    for(int filament = 0; filament < nFilaments; ++filament)
    {
        for(int ring = 0; ring < nRings; ++ring)
        {
            std::size_t i = filament * nRings + ring;

            //bond along filament
            if (ring < nRings-1)
            {
                top.addEdge({i}, {i+1});
            }

            if (ring < nRings-3 || filament < nFilaments-1)
            {
                std::size_t i_filament = (filament < nFilaments-1 ?
                    i + nRings : i - ((nFilaments-1) * nRings) + 3);
                top.addEdge({i}, {i_filament});
            }
        }
    }
}

/**
 * A method to add a seed microtubule to the simulation
 * @param stateModel ReaDDy StateModel
 * @param n_filaments protofilaments
 * @param n_rings rings
 */
void addReaDDyMicrotubuleToSimulation(
    std::unique_ptr<readdy::kernel::scpu::SCPUKernel>* _kernel,
    int nRings)
{
    std::vector<readdy::model::Particle> particles = getMicrotubuleParticles(
        (*_kernel)->context().particleTypes(), 13, nRings, 10.86);

    auto topology = (*_kernel)->stateModel().addTopology(
        (*_kernel)->context().topologyRegistry().idOf("Microtubule"), particles);

    addMicrotubuleEdges(*topology, 13, nRings);
}

} // namespace models
} // namespace agentsim
} // namespace aics
