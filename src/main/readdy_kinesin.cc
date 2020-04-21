#include "agentsim/readdy_models.h"
#include <math.h>

namespace aics {
namespace agentsim {
namespace models {

/**
* A method to get all tubulin types (for binding to motors)
* @param context ReaDDy Context
*/
std::vector<std::string> getAllReactiveTubulinTypes()
{
    std::vector<std::string> tubulinTypes = {"tubulinA#", "tubulinB#"};
    std::vector<std::string> result;
    result.reserve(9 * tubulinTypes.size());
    for (const auto &t : tubulinTypes)
    {
        auto types = getAllPolymerParticleTypes(t);
        result.insert( result.end(), types.begin(), types.end() );
    }
    return result;
}

/**
 * A method to add particle types and constraints to the ReaDDy context
 * @param context ReaDDy Context
 */
void addReaDDyKinesinToSystem(
    readdy::model::Context &context,
    std::shared_ptr<std::unordered_map<std::string,float>>& particleTypeRadiusMapping)
{
    float forceConstant = 90.;
    float eta = 8.1;
    float temperature = 37. + 273.15;

    auto &topologyRegistry = context.topologyRegistry();
    topologyRegistry.addType("Kinesin");

    auto &typeRegistry = context.particleTypes();
    std::unordered_map<std::string,float> particles = {
        {"motor", 2.},
        {"motor#bound", 2.},
        {"cargo", 15.},
        {"hips", 1.}
    };
    typeRegistry.add("hips", calculateDiffusionCoefficient(
        particles.at("hips"), eta, temperature));
    for (auto it : particles)
    {
        if (it.first != "hips")
        {
            // add type
            typeRegistry.add(it.first, calculateDiffusionCoefficient(
                it.second, eta, temperature));

            // bonds to hips
            readdy::api::Bond bond{
                forceConstant, 2. * it.second, readdy::api::BondType::HARMONIC};
            topologyRegistry.configureBondPotential("hips", it.first, bond);
        }
    }

    // repulsions
    context.potentials().addHarmonicRepulsion(
        "motor", "motor", forceConstant, 2. * particles.at("motor"));
    context.potentials().addHarmonicRepulsion(
        "motor", "motor#bound", forceConstant, 2. * particles.at("motor"));
    context.potentials().addHarmonicRepulsion(
        "motor#bound", "motor#bound", forceConstant, 2. * particles.at("motor"));
    context.potentials().addHarmonicRepulsion(
        "hips", "cargo", forceConstant, 2. * particles.at("cargo"));

    // bonds and repulsions with tubulins
    std::vector<std::string> tubulinTypes = getAllReactiveTubulinTypes();
    readdy::api::Bond bond{
        forceConstant, 4., readdy::api::BondType::HARMONIC};
    for (const auto &tubulin : tubulinTypes)
    {
        topologyRegistry.configureBondPotential("motor", tubulin, bond);
        topologyRegistry.configureBondPotential("motor#bound", tubulin, bond);
        context.potentials().addHarmonicRepulsion(
            "motor", tubulin, forceConstant, 3.);
        context.potentials().addHarmonicRepulsion(
            "motor#bound", tubulin, forceConstant, 3.);
    }

    particleTypeRadiusMapping->insert(particles.begin(), particles.end());
}

/**
 * A method to get lists of positions and types for particles in a kinesin
 * @param typeRegistry ReaDDY type registry
 */
std::vector<readdy::model::TopologyParticle> getKinesinParticles(
    readdy::model::ParticleTypeRegistry &typeRegistry,
    Eigen::Vector3d position)
{
    std::vector<readdy::model::TopologyParticle> particles {};

    Eigen::Vector3d motor1_pos = position + Eigen::Vector3d(0., 0., -4.);
    Eigen::Vector3d motor2_pos = position + Eigen::Vector3d(0., 0., 4.);
    Eigen::Vector3d cargo_pos = position + Eigen::Vector3d(0., 30., 0.);

    particles.push_back({
        position[0], position[1], position[2],
        typeRegistry.idOf("hips")});
    particles.push_back({
        motor1_pos[0], motor1_pos[1], motor1_pos[2],
        typeRegistry.idOf("motor")});
    particles.push_back({
        motor2_pos[0], motor2_pos[1], motor2_pos[2],
        typeRegistry.idOf("motor")});
    particles.push_back({
        cargo_pos[0], cargo_pos[1], cargo_pos[2],
        typeRegistry.idOf("cargo")});

    return particles;
}

/**
 * A method to add a kinesin to the simulation
 * @param stateModel ReaDDy StateModel
 */
void addReaDDyKinesinToSimulation(
    std::unique_ptr<readdy::kernel::scpu::SCPUKernel>* _kernel,
    Eigen::Vector3d position)
{
    std::vector<readdy::model::TopologyParticle> particles = getKinesinParticles(
        (*_kernel)->context().particleTypes(), position);

    auto topology = (*_kernel)->stateModel().addTopology(
        (*_kernel)->context().topologyRegistry().idOf("Kinesin"), particles);

    topology->graph().addEdgeBetweenParticles(0, 1);
    topology->graph().addEdgeBetweenParticles(0, 2);
    topology->graph().addEdgeBetweenParticles(0, 3);
}

/**
 * A method to add a breakable kinesin bond
 * @param actions ReaDDy ActionFactory
 */
std::unique_ptr<readdy::model::actions::top::BreakBonds> addBreakableKinesinBond(
    std::unique_ptr<readdy::kernel::scpu::SCPUKernel>* _kernel,
    float timeStep)
{
    float break_threshold = 1.0;
    readdy::model::actions::top::BreakConfig breakConfig;
    std::vector<std::string> tubulinTypes = getAllReactiveTubulinTypes();
    auto particleTypes = (*_kernel)->context().particleTypes();
    for (const auto &t : tubulinTypes)
    {
        breakConfig.addBreakablePair(
            particleTypes.idOf("motor#bound"), particleTypes.idOf(t),
            break_threshold, 1e10);
    }
    auto breakingBonds = (*_kernel)->actions().breakBonds(timeStep, breakConfig);
    return breakingBonds;
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
