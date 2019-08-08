#ifndef AICS_READDYPKG_H
#define AICS_READDYPKG_H

#include "agentsim/simpkg/simpkg.h"
#include "readdy/model/observables/io/TrajectoryEntry.h"
#include "readdy/model/observables/io/Types.h"
#include "readdy/readdy.h"
#include <readdy/model/topologies/TopologyRecord.h>
#include <limits>
#include <fstream>

#include <string>
#include <vector>

struct ParticleData {
    ParticleData(std::string type, std::string flavor, const std::array<readdy::scalar, 3>& pos,
        readdy::model::Particle::id_type id, std::size_t type_id, readdy::time_step_type t)
        : type(std::move(type))
        , flavor(std::move(flavor))
        , position(pos)
        , id(id)
        , type_id(type_id)
        , t(t)
    {
    }

    std::string type;
    std::string flavor;
    std::array<readdy::scalar, 3> position;
    readdy::model::Particle::id_type id;
    std::size_t type_id;
    readdy::time_step_type t;
};

using ParticleH5List = std::vector<ParticleData>;
using TrajectoryH5Info = std::vector<ParticleH5List>;

using TopologyRecord = readdy::model::top::TopologyRecord;
using TopologyH5List = std::vector<TopologyRecord>;
using TopologyH5Info = std::vector<TopologyH5List>;
using TimestepH5List = std::vector<readdy::time_step_type>;
using TimeTopologyH5Info = std::tuple<TimestepH5List, TopologyH5Info>;

namespace aics {
namespace agentsim {

    class ReaDDyPkg : public SimPkg {
    public:
        ReaDDyPkg()
        {
            this->m_simulation = new readdy::Simulation("SingleCPU");
            this->m_bloscFilter.registerFilter();
        }
        virtual ~ReaDDyPkg()
        {
            delete this->m_simulation;
            this->m_simulation = nullptr;
        }

        virtual void Setup() override;
        virtual void Shutdown() override;
        virtual void InitAgents(std::vector<std::shared_ptr<Agent>>& agents, Model& model) override;
        virtual void InitReactions(Model& model) override;

        virtual void RunTimeStep(
            float timeStep, std::vector<std::shared_ptr<Agent>>& agents) override;

        virtual void UpdateParameter(
            std::string param_name, float param_value) override;

        // Cached Sim API
        virtual void GetNextFrame(
            std::vector<std::shared_ptr<Agent>>& agents) override;

        virtual bool IsFinished() override;
        virtual void Run(float timeStep, std::size_t nTimeStep) override;

        virtual void LoadTrajectoryFile(std::string file_path) override;

    private:
        TopologyH5List& GetFileTopologies(std::size_t frameNumber);
        ParticleH5List& GetFileParticles(std::size_t frameNumber);

        readdy::Simulation* m_simulation;
        bool m_agents_initialized = false;
        bool m_reactions_initialized = false;
        int m_timeStepCount = 0;

        bool m_hasAlreadyRun = false;
        bool m_hasLoadedRunFile = false;
        bool m_hasFinishedStreaming = false;
        readdy::io::BloscFilter m_bloscFilter;

        // Used to store FileIO data
        TrajectoryH5Info m_trajectoryInfo;
        TimeTopologyH5Info m_topologyInfo;
    };

} // namespace agentsim
} // namespace aics

#endif // AICS_READDYPKG_H
