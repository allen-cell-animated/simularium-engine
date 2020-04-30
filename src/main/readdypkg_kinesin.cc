#include "agentsim/readdy_models.h"
#include <math.h>
#include "readdy/model/topologies/common.h"
#include <graphs/graphs.h>

namespace aics {
namespace agentsim {
namespace models {

/**
* A method to get all tubulin types (for binding to motors)
* @param types a list of base types
* @return vector of strings for each reactive type
*/
std::vector<std::string> getAllPolymerTypes(
    std::vector<std::string> types
)
{
    std::vector<std::string> result;
    result.reserve(9 * types.size());
    for (const auto &t : types)
    {
        auto ts = getAllPolymerParticleTypes(t);
        result.insert( result.end(), ts.begin(), ts.end() );
    }
    return result;
}

/**
 * A method to add a spatial reaction to bind a kinesin motor to a tubulinB
 * @param context ReaDDy Context
 */
void addKinesinTubulinBindReaction(
    readdy::model::Context &context)
{
    auto &topologyRegistry = context.topologyRegistry();
    topologyRegistry.addType("Microtubule-Kinesin");
    topologyRegistry.addType("Microtubule-Kinesin#Binding");

    //spatial reaction
    auto polymerNumbers = getAllPolymerParticleTypes("");
    int i = 1;
    for (const auto &numbers : polymerNumbers)
    {
        topologyRegistry.addSpatialReaction(
            "Motor-Bind" + std::to_string(i) + ": Microtubule(tubulinB#" +
            numbers + ") + Kinesin(motor) -> " +
            "Microtubule-Kinesin#Binding(tubulinB#bound_" + numbers + "--motor)",
            1e10, 5.);
        i++;
    }

    // structural reaction
    // auto &typeRegistry = context.particleTypes();
    // auto reactionFunction = [&](readdy::model::top::GraphTopology &top)
    // {
    //     readdy::model::top::reactions::Recipe recipe(top);
    //
    //     bool boundMotorExists = false;
    //     auto boundMotorIndex = getIndexOfVertexOfType(context, top, "motor", true, boundMotorExists);
    //     if (!boundMotorExists)
    //     {
    //         std::cout << "failed to find new motor" << std::endl;
    //         return recipe;
    //     }
    //
    //     bool tubulinExists = false;
    //     auto tubulinIndex = getIndexOfNeighborOfType(context, top, boundMotorIndex, "tubulinB", false, tubulinExists);
    //     if (!tubulinExists)
    //     {
    //         std::cout << "failed to find bound tubulin" << std::endl;
    //         return recipe;
    //     }
    //
    //     auto boundMotor = top.vertexIndexForParticle(boundMotorIndex);
    //     recipe.changeParticleType(boundMotor, typeRegistry.idOf("motor#bound"));
    //     auto tubulinPos = top.particleForVertex(top.vertexIndexForParticle(tubulinIndex)).pos();
    //     readdy::Vec3 offset{0., 4., 0.};
    //     recipe.changeParticlePosition(boundMotor, tubulinPos + offset);
    //     recipe.changeTopologyType("Microtubule-Kinesin");
    //
    //     std::cout << "successfully set up new motor" << std::endl;
    //
    //     return recipe;
    // };
    // readdy::model::top::reactions::StructuralTopologyReaction reaction{"Finish-Motor-Bind", reactionFunction, 1e30};
    // topologyRegistry.addStructuralReaction(topologyRegistry.idOf("Microtubule-Kinesin#Binding"), reaction);
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
    particleTypeRadiusMapping->insert(particles.begin(), particles.end());

    typeRegistry.add(
        "hips",
        calculateDiffusionCoefficient(particles.at("hips"), eta, temperature),
        readdy::model::particleflavor::TOPOLOGY
    );

    for (auto it : particles)
    {
        if (it.first != "hips")
        {
            // add type
            typeRegistry.add(
                it.first,
                (it.first == "cargo" ? 35. : 1.) * calculateDiffusionCoefficient(it.second, eta, temperature),
                readdy::model::particleflavor::TOPOLOGY
            );

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
    std::vector<std::string> tubulinTypes = getAllPolymerTypes(
        {"tubulinB#", "tubulinB#bound_"});
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

    addKinesinTubulinBindReaction(context);
}

/**
 * A method to get lists of positions and types for particles in a kinesin
 * @param typeRegistry ReaDDY type registry
 */
std::vector<readdy::model::Particle> getKinesinParticles(
    readdy::model::ParticleTypeRegistry &typeRegistry,
    Eigen::Vector3d position)
{
    Eigen::Vector3d motor1_pos = position + Eigen::Vector3d(0., 0., -4.);
    Eigen::Vector3d motor2_pos = position + Eigen::Vector3d(0., 0., 4.);
    Eigen::Vector3d cargo_pos = position + Eigen::Vector3d(0., 30., 0.);

    std::vector<readdy::model::Particle> particles {
        {position[0], position[1], position[2], typeRegistry.idOf("hips")},
        {motor1_pos[0], motor1_pos[1], motor1_pos[2], typeRegistry.idOf("motor")},
        {motor2_pos[0], motor2_pos[1], motor2_pos[2], typeRegistry.idOf("motor")},
        {cargo_pos[0], cargo_pos[1], cargo_pos[2], typeRegistry.idOf("cargo")},
    };

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
    std::vector<readdy::model::Particle> particles = getKinesinParticles(
        (*_kernel)->context().particleTypes(), position);

    auto top = (*_kernel)->stateModel().addTopology(
        (*_kernel)->context().topologyRegistry().idOf("Kinesin"), particles);

    for (std::size_t i = 1; i < 4; ++i) {
        top->addEdge({0}, {i});
    }
}

/**
 * A method to add a breakable kinesin bond
 * @param actions ReaDDy ActionFactory
 */
std::unique_ptr<readdy::model::actions::top::BreakBonds> addBreakableKinesinBond(
    std::unique_ptr<readdy::kernel::scpu::SCPUKernel>* _kernel,
    float timeStep)
{
    const float break_threshold = 1.0;
    readdy::model::actions::top::BreakConfig breakConfig;
    std::vector<std::string> tubulinTypes = getAllPolymerTypes({"tubulinB#bound_"});
    const auto particleTypes = (*_kernel)->context().particleTypes();
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
 * A method to check the kinesin state
 * @param kernel Readdy Kernel
 */
void checkKinesin(
    std::unique_ptr<readdy::kernel::scpu::SCPUKernel>* _kernel)
{
    std::cout << "---------------------------------------------" <<  std::endl;

    for (auto top : (*_kernel)->stateModel().getTopologies())
    {
        std::cout << "topology " << (*_kernel)->context().topologyRegistry().nameOf(top->type()) << " ----------" <<  std::endl;

        for (auto particle : (*_kernel)->stateModel().getParticlesForTopology(*top))
        {
            auto t = (*_kernel)->context().particleTypes().infoOf(particle.type()).name;
            if (t.find("bound") != std::string::npos)
            {
                std::cout << "  " << t << std::endl;
            }
        }

        std::cout << "---" <<  std::endl;

        bool boundMotorExists = false;
        auto boundMotorIndex = getIndexOfVertexOfType(
            (*_kernel)->context(), *top, "motor", true, boundMotorExists);
        if (!boundMotorExists)
        {
            std::cout << "no motors bound" << std::endl;
            continue;
        }

        bool tubulinExists = false;
        auto tubulinIndex = getIndexOfNeighborOfType(
            (*_kernel)->context(), *top, boundMotorIndex, "tubulinB", false, tubulinExists);
        if (!tubulinExists)
        {
            std::cout << "bound motor has no tubulin neighbor!" << std::endl;
            continue;
        }

        std::cout << "found bound motor & tubulin" << std::endl;
        auto motorPos = top->particleForVertex(top->vertexIndexForParticle(boundMotorIndex)).pos();
        auto tubulinPos = top->particleForVertex(top->vertexIndexForParticle(tubulinIndex)).pos();
        float distance = (motorPos - tubulinPos).norm();
        float energy = 45. * distance * distance;

        std::cout << "bond energy = " << std::to_string(energy) <<  std::endl;
    }
}

} // namespace models
} // namespace agentsim
} // namespace aics
