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
    readdy::model::Context &context)
{
    float forceConstant = 90.;
    float eta = 8.1;
    float temperature = 37. + 273.15;

    float motor_radius = 2.;
    float hips_radius = 1.;
    float cargo_radius = 15.;
    float tubulin_radius = 2.;
    float necklinker_length = 7.;

    // types
    auto &topologyRegistry = context.topologyRegistry();
    topologyRegistry.addType("Kinesin");

    auto &typeRegistry = context.particleTypes();
    float motor_diffCoeff = calculateDiffusionCoefficient(
        motor_radius, eta, temperature);
    float hips_diffCoeff = calculateDiffusionCoefficient(
        hips_radius, eta, temperature);
    float cargo_diffCoeff = calculateDiffusionCoefficient(
        cargo_radius, eta, temperature);
    typeRegistry.add("motor", motor_diffCoeff);
    typeRegistry.add("motor#bound", motor_diffCoeff);
    typeRegistry.add("hips", hips_diffCoeff);
    typeRegistry.add("cargo", cargo_diffCoeff);

    // bonds and repulsions within kinesin
    readdy::api::Bond bond1{
        forceConstant, necklinker_length, readdy::api::BondType::HARMONIC};
    topologyRegistry.configureBondPotential("motor", "hips", bond1);
    topologyRegistry.configureBondPotential("motor#bound", "hips", bond1);
    context.potentials().addHarmonicRepulsion(
        "motor", "motor", forceConstant, 2 * motor_radius);
    context.potentials().addHarmonicRepulsion(
        "motor", "motor#bound", forceConstant, 2 * motor_radius);
    context.potentials().addHarmonicRepulsion(
        "motor#bound", "motor#bound", forceConstant, 2 * motor_radius);
    readdy::api::Bond bond2{
        forceConstant, 2 * cargo_radius, readdy::api::BondType::HARMONIC};
    topologyRegistry.configureBondPotential("hips", "cargo", bond2);
    context.potentials().addHarmonicRepulsion(
        "hips", "cargo", forceConstant, 2 * cargo_radius);

    // bonds and repulsions with tubulins
    std::vector<std::string> tubulinTypes = getAllReactiveTubulinTypes();
    float distance = motor_radius + tubulin_radius;
    readdy::api::Bond bond3{
        forceConstant, distance, readdy::api::BondType::HARMONIC};
    for (const auto &t : tubulinTypes)
    {
        topologyRegistry.configureBondPotential("motor", t, bond3);
        topologyRegistry.configureBondPotential("motor#bound", t, bond3);
        context.potentials().addHarmonicRepulsion(
            "motor", t, forceConstant, distance);
        context.potentials().addHarmonicRepulsion(
            "motor#bound", t, forceConstant, distance);
    }
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

    Eigen::Vector3d motor1_pos = position + Eigen::Vector3d(0., 0., -2.);
    Eigen::Vector3d motor2_pos = position + Eigen::Vector3d(0., 0., 2.);
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
