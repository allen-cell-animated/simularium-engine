#include "agentsim/readdy_models.h"

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
readdy::model::top::graph::Graph::PersistentVertexIndex getVertexOfType(
    readdy::model::Context &context,
    model::top::GraphTopology &top,
    std::string type,
    bool exactMatch)
{
    const auto &types = context->particleTypes();
    auto v = top->graph().vertices().begin();
    while (v != topology->graph().vertices().end()) {
        auto t = types.infoOf(v->particleIndex).name;
        if (typesMatch(t, type, exactMatch))
        {
            return v->persistent_index();
        }
        std::advance(v, 1);
    }
    return NULL;
}

/**
* A method to get the first neighbor of the given vertex with the given type
* @param context ReaDDy Context
* @param vertex persistent index of vertex
* @param type string to find
* @param bool should particle's type contain the type or match it exactly?
* @return the vertex's persistent index
*/
readdy::model::top::graph::Graph::PersistentVertexIndex getNeighborVertexOfType(
    readdy::model::Context &context,
    readdy::model::top::graph::Graph::PersistentVertexIndex vertex,
    std::string type,
    bool exactMatch)
{
    const auto &types = context.particleTypes();
    for (const auto &v : vertex.neighbors())
    {
        auto t = types.infoOf(v->particleIndex).name;
        if (typesMatch(t, type, exactMatch))
        {
            return v->persistent_index();
        }
    }
    return NULL;
}

} // namespace models
} // namespace agentsim
} // namespace aics
