#ifndef AICS_READDY_MODELS_H
#define AICS_READDY_MODELS_H

#include <stdlib.h>
#include "readdy/kernel/singlecpu/SCPUKernel.h"
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
        const std::string particleType
    );

    /**
     * A method to get a list of all polymer numbers
     * ("type1_1", "type1_2", "type1_3", "type2_1", ... "type3_3")
     * @param particleType base particle types
     * @return vector of types
     */
    std::vector<std::string> getAllPolymerParticleTypes(
        std::vector<std::string> types
    );

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
        float temperature
    );

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
        std::shared_ptr<std::unordered_map<std::string,float>>& particleTypeRadiusMapping
    );

    /**
    * A method to clamp offsets so y polymer offset is incremented
    * if new x polymer index is not in [1,3]
    * @param polymerIndexX first polymer index
    * @param polymerOffsets offsets for polymer numbers
    */
    std::vector<int> clampPolymerOffsets(
        int polymerIndexX,
        std::vector<int> polymerOffsets
    );

    /**
    * A method to calculate polymer number with an offset
    * @param number starting polymer number
    * @param offset offset to add in [-2, 2]
    * @return number in [1,3]
    */
    int getPolymerNumber(
        int number,
        int offset
    );

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
        std::vector<int> polymerOffsets
    );

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
        float bondLength
    );

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
        float bondLength
    );

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
        float angleRadians
    );

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
        float angleRadians
    );

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
        float distance
    );

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
        float distance
    );

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
        bool exactMatch
    );

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
        graphs::PersistentIndex vertexIndex
    );

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
        graphs::PersistentIndex &vertexIndex
    );

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
        graphs::PersistentIndex &neighborIndex
    );

    /**
     * A method to add particle types and constraints to the ReaDDy context
     * @param context ReaDDy Context
     */
    void addReaDDyMicrotubuleToSystem(
        readdy::model::Context &context,
        std::shared_ptr<std::unordered_map<std::string,float>>& particleTypeRadiusMapping
    );

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
        float radius
    );

    /**
     * A method to add edges to a microtubule topology
     * @param graph ReaDDy topology graph
     * @param nFilaments protofilaments
     * @param nRings rings
     */
    void addMicrotubuleEdges(
        readdy::model::top::GraphTopology &top,
        int nFilaments,
        int nRings
    );

    /**
     * A method to add a seed microtubule to the simulation
     * @param stateModel ReaDDy StateModel
     * @param n_filaments protofilaments
     * @param n_rings rings
     */
    void addReaDDyMicrotubuleToSimulation(
        std::unique_ptr<readdy::kernel::scpu::SCPUKernel>* _kernel,
        int nRings
    );

    /**
    * A method to get persistent indices for the motors in a topologyRegistry
    * as well as their types (i.e. states) as strings
    * @param context ReaDDy Context
    * @param top ReaDDy GraphTopology
    * @return the persistent index and type for each motor
    */
    std::unordered_map<std::string,graphs::PersistentIndex> getMotorsAndTheirStates(
        readdy::model::Context &context,
        readdy::model::top::GraphTopology &top
    );

    /**
    * A method to get the overall state of a kinesin given both of its motors' states
    * @param motorStates the state for each of the two motors
    * @return string representing kinesin's overall state
    */
    std::string getBoundKinesinState(
        std::vector<std::string> motorStates
    );

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
        graphs::PersistentIndex &changedMotor
    );

    /**
     * A method to add spatial and structural reactions
     * to bind a kinesin motor in ADP state to a free tubulinB
     * @param context ReaDDy Context
     * @param rate ReaDDy scalar reaction rate
     */
    void addMotorBindTubulinReaction(
        readdy::model::Context &context,
        readdy::scalar rate
    );

    /**
     * A method to add structural reactions to set bound motor's state to ATP
     * (and implicitly simulate ATP binding)
     * @param context ReaDDy Context
     * @param rate ReaDDy scalar reaction rate
     */
    void addMotorBindATPReaction(
        readdy::model::Context &context,
        readdy::scalar rate
    );

    /**
     * A method to add structural reactions to release a bound motor from tubulin
     * @param context ReaDDy Context
     * @param rate ReaDDy scalar reaction rate
     */
    void addMotorReleaseTubulinReaction(
        readdy::model::Context &context,
        readdy::scalar rate
    );

    /**
     * A method to add particle types and constraints to the ReaDDy context
     * @param context ReaDDy Context
     */
    void addReaDDyKinesinToSystem(
        readdy::model::Context &context,
        std::shared_ptr<std::unordered_map<std::string,float>>& particleTypeRadiusMapping
    );

    /**
     * A method to get lists of positions and types for particles in a kinesin
     * @param typeRegistry ReaDDY type registry
     */
    std::vector<readdy::model::Particle> getKinesinParticles(
        readdy::model::ParticleTypeRegistry &typeRegistry
    );

    /**
     * A method to add a kinesin to the simulation
     * @param stateModel ReaDDy StateModel
     */
    void addReaDDyKinesinToSimulation(
        std::unique_ptr<readdy::kernel::scpu::SCPUKernel>* _kernel
    );

    /**
     * A method to add a breakable kinesin bond
     * @param actions ReaDDy ActionFactory
     */
    std::unique_ptr<readdy::model::actions::top::BreakBonds> addBreakableKinesinBond(
        std::unique_ptr<readdy::kernel::scpu::SCPUKernel>* _kernel,
        float timeStep
    );

    /**
     * A method to check the kinesin state
     * @param kernel Readdy Kernel
     */
    void checkKinesin(
        std::unique_ptr<readdy::kernel::scpu::SCPUKernel>* _kernel
    );

} // namespace models
} // namespace agentsim
} // namespace aics

#endif // AICS_READDY_MODELS_H
