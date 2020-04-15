#include "agentsim/readdy_models.h"

namespace aics {
namespace agentsim {
namespace models {

/**
 * A method to add particle types and constraints to the ReaDDy context
 * @param context ReaDDy Context
 */
void addReaDDyKinesinToSystem(
    readdy::model::Context &context)
{

}

/**
 * A method to
 * @param stateModel ReaDDy StateModel
 */
void addReaDDyKinesinToSimulation(
    readdy::model::StateModel &stateModel)
{
    // // distance of A and A is 2, equilibrium distance is 1, bond extension is 1, bond energy is 1.
    // std::vector<readdy::model::TopologyParticle> particles{
    //         {0., 0., 0., types.idOf("A")},
    //         {0., 0., 2., types.idOf("A")}
    // };
    // auto graphTop = kernel->stateModel().addTopology(topReg.idOf("T"), particles);
    // auto &graph = graphTop->graph();
    // graph.addEdgeBetweenParticles(0, 1);
}

/**
 * A method to add a breakable kinesin bond
 * @param actions ReaDDy ActionFactory
 */
void addBreakableKinesinBond(
    readdy::model::actions::ActionFactory &actions,
    float timeStep)
{
    // readdy::model::actions::top::BreakConfig breakConfig;
    // if (break_bond)
    // {
    //     breakConfig.addBreakablePair(types.idOf("A"), types.idOf("A"), 0.9, 1e10); // break (low threshold)
    // }
    // else
    // {
    //     breakConfig.addBreakablePair(types.idOf("A"), types.idOf("A"), 1.1, 1e10); // don't break (high threshold)
    // }
    // breakingBonds = actions.breakBonds(timeStep, breakConfig);
}

/**
 * A method to check if a breakable kinesin bond was broken
 * @return whether the bond was broken
 */
bool kinesinBondHasBroken()
{
    return false;
}

} // namespace models
} // namespace agentsim
} // namespace aics
