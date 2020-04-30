#include "agentsim/readdy_models.h"
#include <graphs/graphs.h>

namespace aics {
namespace agentsim {
namespace models {

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
* A method to get the first vertex in the topology with the given type
* @param context ReaDDy Context
* @param top ReaDDy GraphTopology
* @param type string to find
* @param bool should particle's type contain the type or match it exactly?
* @return the vertex's persistent index
*/
graphs::PersistentIndex getIndexOfVertexOfType(
    readdy::model::Context &context,
    readdy::model::top::GraphTopology &top,
    std::string type,
    bool exactMatch,
    bool &vertexExists)
{
    const auto &types = context.particleTypes();
    auto vertex = top.graph().vertices().begin();
    while (vertex != top.graph().vertices().end()) {
        auto t = types.infoOf(top.particleForVertex(*vertex).type()).name;
        if (typesMatch(t, type, exactMatch))
        {
            vertexExists = true;
            return vertex.persistent_index();
        }
        std::advance(vertex, 1);
    }
    vertexExists = false;
    return graphs::PersistentIndex{0};
}

/**
* A method to get the first neighbor of the given vertex with the given type
* @param context ReaDDy Context
* @param graph ReaDDy topology graph
* @param vertex ReaDDy Vertex
* @param type string to find
* @param bool should particle's type contain the type or match it exactly?
* @return the Vertex
*/
graphs::PersistentIndex getIndexOfNeighborOfType(
    readdy::model::Context &context,
    readdy::model::top::GraphTopology &top,
    graphs::PersistentIndex vertexIndex,
    std::string type,
    bool exactMatch,
    bool &vertexExists)
{
    const auto &types = context.particleTypes();
    const auto &vertex = top.graph().vertices().at(vertexIndex);
    for (const auto &neighbor : vertex.neighbors())
    {
        auto t = types.infoOf(top.particleForVertex(neighbor).type()).name;
        if (typesMatch(t, type, exactMatch))
        {
            vertexExists = true;
            return neighbor;
        }
    }
    vertexExists = false;
    return graphs::PersistentIndex{0};
}

} // namespace models
} // namespace agentsim
} // namespace aics
