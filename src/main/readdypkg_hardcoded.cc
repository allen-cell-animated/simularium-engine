#include "agentsim/agents/agent.h"
#include "agentsim/simpkg/readdypkg.h"
#include "agentsim/readdy_models.h"
#include "readdy_microtubule.cc"
#include "readdy_kinesin.cc"
#include <algorithm>
#include <csignal>
#include <math.h>
#include <stdlib.h>
#include <time.h>
#include "readdy/kernel/singlecpu/SCPUKernel.h"
#include "readdy/kernel/singlecpu/actions/SCPUEulerBDIntegrator.h"

/**
 * A method to access the particle positions of a certain type.
 * @param type the type
 * @return a vector containing the particle positions
 */
std::vector<readdy::Vec3> getParticlePositions(
    std::unique_ptr<readdy::kernel::scpu::SCPUKernel>* _kernel,
    const readdy::ParticleTypeId &typeID
);

bool break_bond = true;

std::unique_ptr<readdy::kernel::scpu::SCPUKernel> kernel( new readdy::kernel::scpu::SCPUKernel() );
std::unique_ptr<readdy::model::actions::EulerBDIntegrator> integrator = NULL;
std::unique_ptr<readdy::model::actions::CreateNeighborList> initNeighborList = NULL;
std::unique_ptr<readdy::model::actions::UpdateNeighborList> neighborList = NULL;
std::unique_ptr<readdy::model::actions::CalculateForces> forces = NULL;
std::unique_ptr<readdy::model::actions::reactions::UncontrolledApproximation> reactions = NULL;
std::unique_ptr<readdy::model::actions::top::EvaluateTopologyReactions> topologyReactions = NULL;
std::unique_ptr<readdy::model::actions::top::BreakBonds> breakingBonds = NULL;

bool initialized = false; // temporarily using this to track init

/**
*	Simulation API
**/
namespace aics {
namespace agentsim {

    void ReaDDyPkg::InitAgents(std::vector<std::shared_ptr<Agent>>& agents, Model& model)
    {
        for (std::size_t i = 0; i < 1000; ++i) {
            std::shared_ptr<Agent> agent;
            agent.reset(new Agent());
            agent->SetVisibility(false);
            agent->SetCollisionRadius(1.0);
            agents.push_back(agent);
        }
    }

    void ReaDDyPkg::InitReactions(Model& model) {}

    void ReaDDyPkg::Setup()
    {
        printf("setup\n");

        if (!initialized) { // (!this->m_agents_initialized)

            printf("init\n");

            // context
            if (kernel == NULL) {
                printf("kernel is null\n");
                return;
            }
            models::addReaDDyMicrotubuleToSystem(kernel->context());
            // models::addReaDDyKinesinToSystem(kernel->context());

            // stateModel
            models::addReaDDyMicrotubuleToSimulation(&kernel, 16);
            // models::addReaDDyKinesinToSimulation(
            //     &kernel, Eigen::Vector3d(0., 12., 0.));

            readdy::scalar timeStep = 0.1;

            integrator = kernel->actions().eulerBDIntegrator(timeStep);
            forces = kernel->actions().calculateForces();
            initNeighborList = kernel->actions().createNeighborList(kernel->context().calculateMaxCutoff());
            neighborList = kernel->actions().updateNeighborList();
            reactions = kernel->actions().uncontrolledApproximation(timeStep);
            topologyReactions = kernel->actions().evaluateTopologyReactions(timeStep);

            // breakingBonds = models::addBreakableKinesinBond(&kernel, (float)timeStep);

            initialized = true;
        }

        initNeighborList->perform();
        neighborList->perform();
        forces->perform();
        kernel->evaluateObservables(0);
        kernel->initialize();
    }

    void ReaDDyPkg::Shutdown()
    {
        printf("shutdown\n");

       //  // TODO properly reset kernel state
       // delete kernel.get();
       // kernel = new readdy::kernel::scpu::SCPUKernel();

        this->m_timeStepCount = 0;
        this->m_agents_initialized = false;
        this->m_reactions_initialized = false;
        this->m_hasAlreadyRun = false;
        this->m_hasFinishedStreaming = false;
        ResetFileIO();
    }

    void ReaDDyPkg::RunTimeStep(
        float timeStep, std::vector<std::shared_ptr<Agent>>& agents)
    {
        this->m_timeStepCount++;
        integrator->perform();  // propagate particles
        neighborList->perform();  // neighbor list update
        forces->perform(); // evaluate forces based on current particle configuration
        reactions->perform();  // evaluate reactions
        topologyReactions->perform(); // and topology reactions
        // breakingBonds->perform(); // check bonds for breaks
        neighborList->perform(); // neighbor list update
        forces->perform(); // evaluate forces based on current particle configuration
        kernel->evaluateObservables(this->m_timeStepCount); // evaluate observables

        auto particleTypeRegistry = kernel->context().particleTypes();
        auto particle_types = particleTypeRegistry.typesFlat();

        agents.clear();
        for (std::size_t i = 0; i < particle_types.size(); ++i)
        {
            std::vector<readdy::Vec3> positions = getParticlePositions(&kernel, particle_types[i]);
            for (std::size_t j = 0; j < positions.size(); ++j)
            {
                readdy::Vec3 v = positions[j];
                std::shared_ptr<Agent> newAgent;
                newAgent.reset(new Agent());
                newAgent->SetName(particleTypeRegistry.nameOf(particle_types[i]));
                newAgent->SetTypeID(i);
                newAgent->SetLocation(v[0], v[1], v[2]);
                newAgent->SetCollisionRadius(3.0f);

                agents.push_back(newAgent);
            }
        }
        if (models::kinesinBondHasBroken())
        {
            std::cout << "bond broken!!!" << std::endl;
        }
    }

    void ReaDDyPkg::UpdateParameter(std::string paramName, float paramValue)
    {
        printf("update parameter\n");
    }

} // namespace agentsim
} // namespace aics


/**
 * A method to access the particle positions of a certain type.
 * @param type the type
 * @return a vector containing the particle positions
 */
std::vector<readdy::Vec3> getParticlePositions(
    std::unique_ptr<readdy::kernel::scpu::SCPUKernel>* _kernel,
    const readdy::ParticleTypeId &typeID)
{
    const auto particles = (*_kernel)->stateModel().getParticles();
    std::vector<readdy::Vec3> positions;
    positions.reserve(particles.size());
    for (const auto &p : particles) {
        if (p.type() == typeID) {
            positions.push_back(p.pos());
        }
    }
    return positions;
}
