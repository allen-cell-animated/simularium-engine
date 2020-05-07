#include "agentsim/readdy_models.h"
#include <graphs/graphs.h>

namespace aics {
namespace agentsim {
namespace models {

/**
 * A method to get a list of all polymer numbers
 * ("type1_1", "type1_2", "type1_3", "type2_1", ... "type3_3")
 * @param particleType base particle type
 * @return vector of types
 */
std::vector<std::string> getPolymerParticleTypes(
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
 * A method to get a list of all polymer numbers
 * ("type1_1", "type1_2", "type1_3", "type2_1", ... "type3_3")
 * @param particleType base particle type
 * @return vector of types
 */
std::vector<std::string> getAllPolymerParticleTypes(
    std::vector<std::string> types)
{
    std::vector<std::string> result;
    result.reserve(9 * types.size());
    for (const auto &t : types)
    {
        auto ts = getPolymerParticleTypes(t);
        result.insert( result.end(), ts.begin(), ts.end() );
    }
    return result;
}

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
    auto types = getPolymerParticleTypes(particleType);

    for (const auto &t : types) {
        typeRegistry.add(t, diffusionCoefficient, readdy::model::particleflavor::TOPOLOGY);
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
        auto types = getPolymerParticleTypes(t);
        allParticleTypes.insert(
            allParticleTypes.end(), types.begin(), types.end());
    }
    addRepulsion(
        context, allParticleTypes, allParticleTypes, forceConstant, distance);
}

/**
* A method to check if the first type contains or equals the second
* @param type1 string
* @param type2 string
* @param exactMatch bool
* @return true if matches
*/
bool typesMatch(
    std::string type1,
    std::string type2,
    bool exactMatch)
{
    if (exactMatch)
    {
        return type1 == type2;
    }
    else
    {
        return type1.find(type2) != std::string::npos;
    }
}

/**
* A method to get the type of the vertex at the given index
* @param context ReaDDy Context
* @param top ReaDDy GraphTopology
* @param vertexIndex ReaDDy PersistentIndex the vertex
* @return string type of vertex
*/
std::string getVertexType(
    readdy::model::Context &context,
    readdy::model::top::GraphTopology &top,
    graphs::PersistentIndex vertexIndex)
{
    const auto &types = context.particleTypes();
    auto vertex = top.graph().vertices().at(vertexIndex);
    return types.infoOf(top.particleForVertex(vertex).type()).name;
}

/**
* A method to get the first vertex in the topology with the given type
* @param context ReaDDy Context
* @param top ReaDDy GraphTopology
* @param type string to find
* @param bool should particle's type contain the type or match it exactly?
* @param vertexIndex ReaDDy PersistentIndex a reference to be set to the neighbor
* @return success
*/
bool setIndexOfVertexOfType(
    readdy::model::Context &context,
    readdy::model::top::GraphTopology &top,
    std::string type,
    bool exactMatch,
    graphs::PersistentIndex &vertexIndex)
{
    const auto &types = context.particleTypes();
    auto vertex = top.graph().vertices().begin();
    while (vertex != top.graph().vertices().end()) {
        auto t = types.infoOf(top.particleForVertex(*vertex).type()).name;
        if (typesMatch(t, type, exactMatch))
        {
            vertexIndex = vertex.persistent_index();
            return true;
        }
        std::advance(vertex, 1);
    }
    return false;
}

/**
* A method to set the vertexIndex reference to the first neighbor
* of the given vertex with the given type
* @param context ReaDDy Context
* @param graph ReaDDy topology graph
* @param vertex ReaDDy Vertex
* @param type string to find
* @param bool should particle's type contain the type or match it exactly?
* @param neighborIndex ReaDDy PersistentIndex a reference to be set to the neighbor
* @return success
*/
bool setIndexOfNeighborOfType(
    readdy::model::Context &context,
    readdy::model::top::GraphTopology &top,
    graphs::PersistentIndex vertexIndex,
    std::string type,
    bool exactMatch,
    graphs::PersistentIndex &neighborIndex)
{
    const auto &types = context.particleTypes();
    const auto &vertex = top.graph().vertices().at(vertexIndex);
    for (const auto &neighbor : vertex.neighbors())
    {
        auto t = types.infoOf(top.particleForVertex(neighbor).type()).name;
        if (typesMatch(t, type, exactMatch))
        {
            neighborIndex = neighbor;
            return true;
        }
    }
    return false;
}

} // namespace models
} // namespace agentsim
} // namespace aics
