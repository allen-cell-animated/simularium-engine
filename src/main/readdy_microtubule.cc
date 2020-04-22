#include "agentsim/readdy_models.h"
#include <math.h>
#include <stdlib.h>
#include "readdy/kernel/singlecpu/SCPUKernel.h"

namespace aics {
namespace agentsim {
namespace models {

std::set<std::tuple<std::string, std::string>> bondPairs;
std::set<std::tuple<std::string, std::string, std::string>> angleTriples;
std::set<std::tuple<std::string, std::string>> repulsePairs;

/**
* A method to calculate the theoretical diffusion constant of a spherical particle
* @param radius [nm]
* @param eta viscosity [cP]
* @param temperature [Kelvin]
* @return diffusion coefficient [nm^2/s]
*/
float calculateDiffusionCoefficient(
    float radius,
    float eta,
    float temperature)
{
    return ((1.38065 * pow(10.,-23.) * temperature)
            / (6 * M_PI * eta * pow(10.,-3.) * radius * pow(10.,-9.))
            * pow(10.,18.)/pow(10.,9.));
}

/**
 * A method to get a list of all polymer numbers
 * ("type1_1", "type1_2", "type1_3", "type2_1", ... "type3_3")
 * @param particleType base particle type
 * @return vector of types
 */
std::vector<std::string> getAllPolymerParticleTypes(
    const std::string particleType)
{
    std::vector<std::string> result {};
    for(int x = 1; x < 4; ++x)
    {
        for(int y = 1; y < 4; ++y)
        {
            result.push_back(
                particleType + std::to_string(x) + "_" + std::to_string(y));
        }
    }
    return result;
}

/**
 * A method to add ReaDDy topology species for all polymer numbers
 * ("type1_1", "type1_2", "type1_3", "type2_1", ... "type3_3")
 * @param typeRegistry ReaDDy ParticleTypeRegistry
 * @param particleType base particle type
 * @param diffusionCoefficient diffusion coefficient [nm^2/s]
 */
void addPolymerTopologySpecies(
    readdy::model::ParticleTypeRegistry &typeRegistry,
    const std::string particleType,
    float radius,
    float diffusionCoefficient,
    std::shared_ptr<std::unordered_map<std::string,float>>& particleTypeRadiusMapping)
{
    auto types = getAllPolymerParticleTypes(particleType);

    for (const auto &t : types) {
        typeRegistry.add(t, diffusionCoefficient);
        particleTypeRadiusMapping->insert(
            std::pair<std::string,float>(t, radius));
    }
}

/**
* A method to add a bond (if it hasn't been added already)
* @param topologyRegistry ReaDDy TopologyRegistry
* @param particleTypes1 from particle types
* @param particleTypes2 to particle types
* @param forceConstant force constant
* @param bondLength equilibrium distance [nm]
*/
void addBond(
    readdy::model::top::TopologyRegistry &topologyRegistry,
    std::vector<std::string> particleTypes1,
    std::vector<std::string> particleTypes2,
    float forceConstant,
    float bondLength)
{
    readdy::api::Bond bond{
        forceConstant, bondLength, readdy::api::BondType::HARMONIC};
    for (const auto &t1 : particleTypes1)
    {
        for (const auto &t2 : particleTypes2)
        {
            if (!bondPairs.insert({t1, t2}).second)
            {
                continue;
            }
            topologyRegistry.configureBondPotential(t1, t2, bond);
            bondPairs.insert({t2, t1});
        }
    }
}

/**
* A method to clamp offsets so y polymer offset is incremented
* if new x polymer index is not in [1,3]
* @param polymerIndexX first polymer index
* @param polymerOffsets offsets for polymer numbers
*/
std::vector<int> clampPolymerOffsets(
    int polymerIndexX,
    std::vector<int> polymerOffsets)
{
    if (polymerOffsets.size() < 2)
    {
        return polymerOffsets;
    }

    std::vector<int> offsets(polymerOffsets);
    if (offsets[0] != 0)
    {
        if (polymerIndexX + offsets[0] < 1)
        {
            offsets[1] -= 1;
        }
        else if (polymerIndexX + offsets[0] > 3)
        {
            offsets[1] += 1;
        }
    }
    return offsets;
}

/**
* A method to calculate polymer number with an offset
* @param number starting polymer number
* @param offset offset to add in [-2, 2]
* @return number in [1,3]
*/
int getPolymerNumber(
    int number,
    int offset)
{
    int n = number + offset;
    if (n > 3)
    {
        n -= 3;
    }
    else if (n < 1)
    {
        n += 3;
    }
    return n;
}

/**
* A method to create a list of types with 2D polymer numbers
* @param particleTypes base particle types
* @param x first polymer number
* @param y second polymer number
* @param polymerOffsets offsets for polymer numbers [dx, dy] both in [-1, 1]
* @return vector of string particle types with polymer numbers of the offsets
*/
std::vector<std::string> getTypesWithPolymerNumbers(
    std::vector<std::string> particleTypes,
    int x,
    int y,
    std::vector<int> polymerOffsets)
{
    std::vector<std::string> types {};
    for (const auto &t : particleTypes)
    {
        if (polymerOffsets.size() > 0)
        {
            types.push_back(
                t + std::to_string(getPolymerNumber(x, polymerOffsets[0]))
                + "_" + std::to_string(getPolymerNumber(y, polymerOffsets[1])));
        }
        else
        {
            types.push_back(t);
        }
    }
    return types;
}

/**
* A method to add bonds between all polymer numbers
* @param topologyRegistry ReaDDy TopologyRegistry
* @param particleTypes1 from particle types
* @param polymerOffsets1 offsets for from particle types (likely [0,0])
* @param particleTypes2 to particle types
* @param polymerOffsets2 offsets for to particle types
* @param forceConstant force constant
* @param bondLength equilibrium distance [nm]
*/
void addPolymerBond(
    readdy::model::top::TopologyRegistry &topologyRegistry,
    std::vector<std::string> particleTypes1,
    std::vector<int> polymerOffsets1,
    std::vector<std::string> particleTypes2,
    std::vector<int> polymerOffsets2,
    float forceConstant,
    float bondLength)
{
    for(int x = 1; x < 4; ++x)
    {
        for(int y = 1; y < 4; ++y)
        {
            auto offsets1 = clampPolymerOffsets(x, polymerOffsets1);
            auto offsets2 = clampPolymerOffsets(x, polymerOffsets2);

            addBond(
                topologyRegistry,
                getTypesWithPolymerNumbers(particleTypes1, x, y, offsets1),
                getTypesWithPolymerNumbers(particleTypes2, x, y, offsets2),
                forceConstant, bondLength
            );
        }
    }
}

/**
* A method to add an angle (if it hasn't been added already)
* @param topologyRegistry ReaDDy TopologyRegistry
* @param particleTypes1 from particle types
* @param particleTypes2 through particle types
* @param particleTypes3 to particle types
* @param forceConstant force constant
* @param angleRadians equilibrium angle [radians]
*/
void addAngle(
    readdy::model::top::TopologyRegistry &topologyRegistry,
    std::vector<std::string> particleTypes1,
    std::vector<std::string> particleTypes2,
    std::vector<std::string> particleTypes3,
    float forceConstant,
    float angleRadians)
{
    readdy::api::Angle angle{
        forceConstant, angleRadians, readdy::api::AngleType::HARMONIC};
    for (const auto &t1 : particleTypes1)
    {
        for (const auto &t2 : particleTypes2)
        {
            for (const auto &t3 : particleTypes3)
            {
                if (!angleTriples.insert({t1, t2, t3}).second)
                {
                    continue;
                }
                topologyRegistry.configureAnglePotential(t1, t2, t3, angle);
                angleTriples.insert({t3, t2, t1});
            }
        }
    }
}

/**
* A method to add an angle between all polymer numbers
* @param topologyRegistry ReaDDy TopologyRegistry
* @param particleTypes1 from particle types
* @param polymerOffsets1 offsets for from particle types (likely [0,0])
* @param particleTypes2 through particle types
* @param polymerOffsets2 offsets for through particle types
* @param particleTypes3 to particle types
* @param polymerOffsets3 offsets for to particle types
* @param forceConstant force constant
* @param bondLength equilibrium angle [radians]
*/
void addPolymerAngle(
    readdy::model::top::TopologyRegistry &topologyRegistry,
    std::vector<std::string> particleTypes1,
    std::vector<int> polymerOffsets1,
    std::vector<std::string> particleTypes2,
    std::vector<int> polymerOffsets2,
    std::vector<std::string> particleTypes3,
    std::vector<int> polymerOffsets3,
    float forceConstant,
    float angleRadians)
{
    for(int x = 1; x < 4; ++x)
    {
        for(int y = 1; y < 4; ++y)
        {
            auto offsets1 = clampPolymerOffsets(x, polymerOffsets1);
            auto offsets2 = clampPolymerOffsets(x, polymerOffsets2);
            auto offsets3 = clampPolymerOffsets(x, polymerOffsets3);

            addAngle(
                topologyRegistry,
                getTypesWithPolymerNumbers(particleTypes1, x, y, offsets1),
                getTypesWithPolymerNumbers(particleTypes2, x, y, offsets2),
                getTypesWithPolymerNumbers(particleTypes3, x, y, offsets3),
                forceConstant, angleRadians
            );
        }
    }
}

/**
* A method to add a pairwise repulsion (if it hasn't been added already)
* @param context ReaDDy context
* @param particleTypes1 from particle types
* @param particleTypes2 to particle types
* @param forceConstant force constant
* @param distance equilibrium distance [nm]
*/
void addRepulsion(
    readdy::model::Context &context,
    std::vector<std::string> particleTypes1,
    std::vector<std::string> particleTypes2,
    float forceConstant,
    float distance)
{
    for (const auto &t1 : particleTypes1)
    {
        for (const auto &t2 : particleTypes2)
        {
            if (!repulsePairs.insert({t1, t2}).second)
            {
                continue;
            }
            context.potentials().addHarmonicRepulsion(
                t1, t2, forceConstant, distance);
            repulsePairs.insert({t2, t1});
        }
    }
}

/**
* A method to add a pairwise repulsion between all polymer numbers
* @param context ReaDDy context
* @param particleTypes1 between particle types
* @param forceConstant force constant
* @param distance equilibrium distance [nm]
*/
void addPolymerRepulsion(
    readdy::model::Context &context,
    std::vector<std::string> particleTypes,
    float forceConstant,
    float distance)
{
    std::vector<std::string> allParticleTypes {};
    for (const auto &t : particleTypes)
    {
        auto types = getAllPolymerParticleTypes(t);
        allParticleTypes.insert(
            allParticleTypes.end(), types.begin(), types.end());
    }
    addRepulsion(
        context, allParticleTypes, allParticleTypes, forceConstant, distance);
}

/**
 * A method to add particle types and constraints to the ReaDDy context
 * @param context ReaDDy Context
 */
void addReaDDyMicrotubuleToSystem(
    readdy::model::Context &context,
    std::shared_ptr<std::unordered_map<std::string,float>>& particleTypeRadiusMapping)
{
    float forceConstant = 90.;
    float eta = 8.1;
    float temperature = 37. + 273.15;

    // types
    auto &topologyRegistry = context.topologyRegistry();
    topologyRegistry.addType("Microtubule");

    auto &typeRegistry = context.particleTypes();
    std::vector<std::string> tubulinTypes = {"tubulinA#", "tubulinB#"};
    float diffCoeff = calculateDiffusionCoefficient(2., eta, temperature);
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
std::vector<readdy::model::TopologyParticle> getMicrotubuleParticles(
    readdy::model::ParticleTypeRegistry &typeRegistry,
    int nFilaments,
    int nRings,
    float radius)
{
    std::vector<readdy::model::TopologyParticle> particles {};

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
    readdy::model::top::graph::Graph &graph,
    int nFilaments,
    int nRings)
{
    for(int filament = 0; filament < nFilaments; ++filament)
    {
        for(int ring = 0; ring < nRings; ++ring)
        {
            int i = filament * nRings + ring;

            //bond along filament
            if (ring < nRings-1)
            {
                graph.addEdgeBetweenParticles(i, i+1);
            }

            if (ring < nRings-3 || filament < nFilaments-1)
            {
                graph.addEdgeBetweenParticles(i, filament < nFilaments-1 ? i + nRings :
                    i - ((nFilaments-1) * nRings) + 3);
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
    std::vector<readdy::model::TopologyParticle> particles = getMicrotubuleParticles(
        (*_kernel)->context().particleTypes(), 13, nRings, 10.86);

    auto topology = (*_kernel)->stateModel().addTopology(
        (*_kernel)->context().topologyRegistry().idOf("Microtubule"), particles);

    addMicrotubuleEdges(topology->graph(), 13, nRings);
}

} // namespace models
} // namespace agentsim
} // namespace aics
