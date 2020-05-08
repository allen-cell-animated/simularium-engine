#include "agentsim/readdy_models.h"
#include <math.h>
#include "readdy/model/topologies/common.h"
#include <graphs/graphs.h>

namespace aics {
namespace agentsim {
namespace models {

/**
* A method to get persistent indices for the motors in a topologyRegistry
* as well as their types (i.e. states) as strings
* @param context ReaDDy Context
* @param top ReaDDy GraphTopology
* @return the persistent index and type for each motor
*/
std::vector<std::pair<std::string,graphs::PersistentIndex>> getMotorsAndTheirStates(
    readdy::model::Context &context,
    readdy::model::top::GraphTopology &top)
{
    std::vector<std::pair<std::string,graphs::PersistentIndex>> motors;
    const auto &types = context.particleTypes();
    auto vertex = top.graph().vertices().begin();
    while (vertex != top.graph().vertices().end()) {
        auto t = types.infoOf(top.particleForVertex(*vertex).type()).name;
        if (typesMatch(t, "motor", false))
        {
            motors.push_back(std::pair<std::string,graphs::PersistentIndex>(
                t, vertex.persistent_index()));
        }
        std::advance(vertex, 1);
    }
    return motors;
}

/**
* A method to get the overall state of a kinesin given both of its motors' states
* @param motorStates the state for each of the two motors
* @return string representing kinesin's overall state
*/
std::string getBoundKinesinState(
    std::vector<std::string> motorStates)
{
    std::vector<std::string> states = {};
    std::vector<std::string> possibleStates = {"ADP", "ATP", "apo"};
    for (std::size_t i = 0; i < motorStates.size(); ++i)
    {
        for (std::size_t j = 0; j < possibleStates.size(); ++j)
        {
            if (motorStates[i].find(possibleStates[j]) != std::string::npos)
            {
                states.push_back(possibleStates[j]);
            }
        }
    }
    if (states.size() < 2)
    {
        std::cout << "*** failed to get kinesin state, found " << states.size() << " motor(s)" << std::endl;
        return "";
    }
    std::sort(states.begin(), states.end());
    return states[0] + "-" + states[1];
}

/**
* A method to change the state of a motor
* and update the kinesin state to match
* @param context ReaDDy Context
* @param top ReaDDy GraphTopology
* @param recipe ReaDDy recipe
* @param the starting state of the motor to change
* @param the state to change the motor to
* @param a reference to the changed motor's index
* @return success
*/
bool setKinesinState(
    readdy::model::Context &context,
    readdy::model::top::GraphTopology &top,
    readdy::model::top::reactions::Recipe &recipe,
    std::string fromMotorState,
    std::string toMotorState,
    graphs::PersistentIndex &changedMotor)
{
    std::vector<std::pair<std::string,graphs::PersistentIndex>> motors =
        getMotorsAndTheirStates(context, top);
    if (motors.size() < 2)
    {
        std::cout << "*** failed to find 2 motors, found " << motors.size() << std::endl;
        return false;
    }

    std::string otherState = "";
    std::vector<graphs::PersistentIndex> motorsInState;
    for (std::size_t i = 0; i < motors.size(); ++i)
    {
        if (motors[i].first.find(fromMotorState) != std::string::npos)
        {
            motorsInState.push_back(motors[i].second);
        }
        else
        {
            otherState = motors[i].first;
        }
    }

    bool foundMotor = false;
    graphs::PersistentIndex motorToSet;
    if (motorsInState.size() > 0)
    {
        if (motorsInState.size() > 1)
        {
            motorToSet = motorsInState[rand() % 2];
            otherState = fromMotorState;
            foundMotor = true;
        }
        else
        {
            motorToSet = motorsInState[0];
            foundMotor = true;
        }
    }

    if (!foundMotor)
    {
        std::cout << "*** failed to find motor in " << fromMotorState << " state" << std::endl;
        return false;
    }

    changedMotor = motorToSet;
    recipe.changeParticleType(
        motorToSet, context.particleTypes().idOf("motor#" + toMotorState));

    if (toMotorState.find("ADP") != std::string::npos &&
        otherState.find("ADP") != std::string::npos)
    {
        recipe.changeTopologyType("Microtubule-Kinesin#Releasing");
    }
    else
    {
        recipe.changeTopologyType(
            "Microtubule-Kinesin#" + getBoundKinesinState({toMotorState, otherState}));
    }
    return true;
}

/**
 * A method to add spatial and structural reactions
 * to bind a kinesin motor in ADP state to a free tubulinB
 * @param context ReaDDy Context
 * @param rate ReaDDy scalar reaction rate
 */
void addMotorBindTubulinReaction(
    readdy::model::Context &context,
    readdy::scalar rate)
{
    auto &topologyRegistry = context.topologyRegistry();
    topologyRegistry.addType("Microtubule-Kinesin#Binding");

    //spatial reactions
    auto polymerNumbers = getPolymerParticleTypes("");
    int i = 1;
    for (const auto &numbers : polymerNumbers)
    {
        topologyRegistry.addSpatialReaction(
            "Bind-Tubulin" + std::to_string(i) + ": Kinesin(motor#ADP) + Microtubule(tubulinB#" +
            numbers + ") -> Microtubule-Kinesin#Binding(motor#new--tubulinB#bound_" + numbers + ")",
            rate, 4.);
        i++;
    }
    std::vector<std::string> kinesinStates = {"ADP-apo", "ADP-ATP"};
    for (const auto &state : kinesinStates)
    {
        for (const auto &numbers : polymerNumbers)
        {
            topologyRegistry.addSpatialReaction(
                "Bind-Tubulin#" + state + std::to_string(i) + ": Microtubule-Kinesin#" +
                state + "(motor#ADP) + Microtubule-Kinesin#" + state + "(tubulinB#" +
                numbers + ") -> Microtubule-Kinesin#Binding(motor#new--tubulinB#bound_" +
                numbers + ") [self=true]",
                1e10, 4.);
            i++;
        }
    }

    // structural reaction
    auto reactionFunction = [&](readdy::model::top::GraphTopology &top)
    {
        std::cout << "Bind Tubulin" << std::endl;

        readdy::model::top::reactions::Recipe recipe(top);

        graphs::PersistentIndex motorIndex;
        if (!setKinesinState(context, top, recipe, "new", "apo", motorIndex))
        {
            return recipe;
        }

        graphs::PersistentIndex tubulinIndex;
        if (!setIndexOfNeighborOfType(context, top, motorIndex,
            "tubulinB#bound", false, tubulinIndex))
        {
            std::cout << "*** failed to find bound tubulin" << std::endl;
            return recipe;
        }

        auto tubulinPos = top.particleForVertex(tubulinIndex).pos();
        readdy::Vec3 offset{0., 4., 0.};
        recipe.changeParticlePosition(motorIndex, tubulinPos + offset);

        std::cout << "successfully set up new motor" << std::endl;

        return recipe;
    };
    readdy::model::top::reactions::StructuralTopologyReaction reaction{
        "Finish-Bind-Tubulin", reactionFunction, 1e30};
    topologyRegistry.addStructuralReaction(
        topologyRegistry.idOf("Microtubule-Kinesin#Binding"), reaction);
}

/**
 * A method to add structural reactions to set bound motor's state to ATP
 * (and implicitly simulate ATP binding)
 * @param context ReaDDy Context
 * @param rate ReaDDy scalar reaction rate
 */
void addMotorBindATPReaction(
    readdy::model::Context &context,
    readdy::scalar rate)
{
    auto &topologyRegistry = context.topologyRegistry();

    // structural reaction
    auto reactionFunction = [&](readdy::model::top::GraphTopology &top)
    {
        std::cout << "Bind ATP" << std::endl;

        readdy::model::top::reactions::Recipe recipe(top);

        graphs::PersistentIndex motorIndex;
        if (!setKinesinState(context, top, recipe, "apo", "ATP", motorIndex))
        {
            return recipe;
        }

        std::cout << "successfully bound ATP" << std::endl;

        return recipe;
    };
    std::vector<std::string> kinesinStates = {"ADP-apo", "ATP-apo", "apo-apo"};
    for (const auto &state : kinesinStates)
    {
        readdy::model::top::reactions::StructuralTopologyReaction reaction{
            "Bind-ATP#" + state, reactionFunction, rate};
        topologyRegistry.addStructuralReaction(
            topologyRegistry.idOf("Microtubule-Kinesin#" + state), reaction);
    }
}

/**
 * A method to add structural reactions to release a bound motor from tubulin
 * @param context ReaDDy Context
 * @param rate ReaDDy scalar reaction rate
 */
void addMotorReleaseTubulinReaction(
    readdy::model::Context &context,
    readdy::scalar rate)
{
    auto &topologyRegistry = context.topologyRegistry();
    topologyRegistry.addType("Microtubule-Kinesin#Releasing");

    // structural reaction
    auto &typeRegistry = context.particleTypes();
    auto reactionFunction = [&](readdy::model::top::GraphTopology &top)
    {
        std::cout << "Release Tubulin" << std::endl;

        readdy::model::top::reactions::Recipe recipe(top);

        graphs::PersistentIndex motorIndex;
        if (!setKinesinState(context, top, recipe, "ATP", "ADP", motorIndex))
        {
            return recipe;
        }

        graphs::PersistentIndex tubulinIndex;
        if (!setIndexOfNeighborOfType(context, top, motorIndex,
            "tubulinB#bound", false, tubulinIndex))
        {
            std::cout << "*** failed to find bound tubulin" << std::endl;
            return recipe;
        }

        std::string tubulinType = getVertexType(context, top, tubulinIndex);
        recipe.changeParticleType(
            tubulinIndex, typeRegistry.idOf("tubulinB#" + tubulinType.substr(15,3)));

        recipe.removeEdge(motorIndex, tubulinIndex);

        std::cout << "successfully released tubulin" << std::endl;

        return recipe;
    };
    std::vector<std::string> kinesinStates = {"ADP-ATP", "ATP-ATP", "ATP-apo"};
    for (const auto &state : kinesinStates)
    {
        readdy::model::top::reactions::StructuralTopologyReaction reaction{
            "Release-Tubulin-" + state, reactionFunction, rate};
        topologyRegistry.addStructuralReaction(
            topologyRegistry.idOf("Microtubule-Kinesin#" + state), reaction);
    }

    // cleanup structural reaction
    auto reactionFunctionCleanup = [&](readdy::model::top::GraphTopology &top)
    {
        std::cout << "Cleanup Release Tubulin" << std::endl;

        readdy::model::top::reactions::Recipe recipe(top);

        std::string newType = "Microtubule";
        std::vector<std::pair<std::string,graphs::PersistentIndex>> motors =
        getMotorsAndTheirStates(context, top);
        if (motors.size() > 0)
        {
            newType = "Kinesin";
        }

        recipe.changeTopologyType(newType);
        std::cout << "successfully cleaned up " << newType << std::endl;

        return recipe;
    };
    readdy::model::top::reactions::StructuralTopologyReaction reaction2{
        "Cleanup-Release-Tubulin", reactionFunctionCleanup, 1e30};
    topologyRegistry.addStructuralReaction(
        topologyRegistry.idOf("Microtubule-Kinesin#Releasing"), reaction2);
}

/**
 * A method to add particle types and constraints to the ReaDDy context
 * @param context ReaDDy Context
 */
void addReaDDyKinesinToSystem(
    readdy::model::Context &context,
    std::shared_ptr<std::unordered_map<std::string,float>>& particleTypeRadiusMapping)
{
    float motorDiffusionMultiplier = 2.;
    float hipsDiffusionMultiplier = 2.;
    float cargoDiffusionMultiplier = 1.; //35.
    float rateMultiplier = pow(10, 7);

    float forceConstant = 90.;
    float eta = 8.1;
    float temperature = 37. + 273.15;

    auto &topologyRegistry = context.topologyRegistry();
    topologyRegistry.addType("Kinesin");
    topologyRegistry.addType("Microtubule-Kinesin#ADP-ATP");
    topologyRegistry.addType("Microtubule-Kinesin#ADP-apo");
    topologyRegistry.addType("Microtubule-Kinesin#ATP-ATP");
    topologyRegistry.addType("Microtubule-Kinesin#ATP-apo");
    topologyRegistry.addType("Microtubule-Kinesin#apo-apo");

    auto &typeRegistry = context.particleTypes();
    std::unordered_map<std::string,float> particles = {
        {"motor#ADP", 2.},
        {"motor#ATP", 2.},
        {"motor#apo", 2.},
        {"motor#new", 2.},
        {"cargo", 15.},
        {"hips", 1.}
    };
    particleTypeRadiusMapping->insert(particles.begin(), particles.end());

    typeRegistry.add(
        "hips",
        hipsDiffusionMultiplier * calculateDiffusionCoefficient(particles.at("hips"), eta, temperature),
        readdy::model::particleflavor::TOPOLOGY
    );

    for (auto it : particles)
    {
        if (it.first != "hips")
        {
            // add type
            typeRegistry.add(
                it.first,
                (it.first.find("motor") != std::string::npos ? motorDiffusionMultiplier : cargoDiffusionMultiplier) *
                calculateDiffusionCoefficient(it.second, eta, temperature),
                readdy::model::particleflavor::TOPOLOGY
            );

            // bond to hips
            readdy::api::Bond bond{
                (it.first == "cargo" ? 0.1 : 1.) * forceConstant,
                2. * it.second, readdy::api::BondType::HARMONIC};
            topologyRegistry.configureBondPotential("hips", it.first, bond);
        }
    }

    // intramolecular repulsions
    std::vector<std::string> motorTypes = {"motor#ADP", "motor#ATP", "motor#apo", "motor#new"};
    addRepulsion(context, motorTypes, motorTypes, forceConstant, 4.);
    context.potentials().addHarmonicRepulsion(
        "hips", "cargo", forceConstant, 2. * particles.at("cargo"));

    // bonds and repulsions with tubulins
    std::vector<std::string> tubulinTypes = getAllPolymerParticleTypes(
        {"tubulinB#", "tubulinB#bound_"});
    addBond(topologyRegistry, motorTypes, tubulinTypes, forceConstant, 4.);
    addRepulsion(context, motorTypes, tubulinTypes, forceConstant, 3.);

    addMotorBindTubulinReaction(context, rateMultiplier * 4.3 * pow(10, -7));
    addMotorBindATPReaction(context, rateMultiplier * 6. * pow(10, -9));
    addMotorReleaseTubulinReaction(context, rateMultiplier * 1.8 * pow(10, -7));
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
    Eigen::Vector3d motor2_pos = position + Eigen::Vector3d(0., 3., 4.);
    Eigen::Vector3d cargo_pos = position + Eigen::Vector3d(0., 30., 0.);

    std::vector<readdy::model::Particle> particles {
        {position[0], position[1], position[2], typeRegistry.idOf("hips")},
        {motor1_pos[0], motor1_pos[1], motor1_pos[2], typeRegistry.idOf("motor#ADP")},
        {motor2_pos[0], motor2_pos[1], motor2_pos[2], typeRegistry.idOf("motor#ADP")},
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
    const float break_threshold = 1000.0;
    readdy::model::actions::top::BreakConfig breakConfig;
    std::vector<std::string> tubulinTypes = getPolymerParticleTypes("tubulinB#bound_");
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
    std::cout << "check --------------------------" <<  std::endl;
    for (auto top : (*_kernel)->stateModel().getTopologies())
    {
        std::vector<std::pair<std::string,graphs::PersistentIndex>> motors =
            getMotorsAndTheirStates((*_kernel)->context(), *top);
        if (motors.size() < 2)
        {
            // std::cout << "failed to find motors" << std::endl;
            continue;
        }
        std::string motorStates = "";
        for (std::size_t i = 0; i < motors.size(); ++i)
        {
            motorStates += motors[i].first + " ";
        }
        std::cout << "motor states = " << motorStates << std::endl;

        graphs::PersistentIndex tubulinIndex;
        for (std::size_t i = 0; i < motors.size(); ++i)
        {
            if (setIndexOfNeighborOfType((*_kernel)->context(), *top, motors[i].second,
                "tubulinB#bound", false, tubulinIndex))
            {
                auto motorPos = top->particleForVertex(motors[i].second).pos();
                auto tubulinPos = top->particleForVertex(tubulinIndex).pos();
                float distance = (motorPos - tubulinPos).norm();
                float energy = 45. * distance * distance;

                std::cout << motors[i].first << " bond energy = " << std::to_string(energy) <<  std::endl;
            }
        }
    }
}

} // namespace models
} // namespace agentsim
} // namespace aics
