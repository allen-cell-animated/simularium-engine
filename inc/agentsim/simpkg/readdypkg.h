#ifndef AICS_READDYPKG_H
#define AICS_READDYPKG_H

#include "agentsim/simpkg/simpkg.h"
#include "readdy/model/observables/io/TrajectoryEntry.h"
#include "readdy/model/observables/io/Types.h"
#include "readdy/readdy.h"
#include <fstream>
#include <limits>
#include <readdy/model/topologies/TopologyRecord.h>

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

/*
The below has been copied from ReaDDy for developer reference
so that one can quickly double check the definition for the topology
file io structure w/out digging through ReaDDy's python API

struct TopologyRecord {
    Topology::particle_indices particleIndices;
    std::vector<std::tuple<std::size_t, std::size_t>> edges;
    TopologyTypeId type;

    bool operator==(const TopologyRecord &other) const {
        return particleIndices == other.particleIndices && edges == other.edges;
    }
};
*/

using TimestepH5List = std::vector<readdy::time_step_type>;

using ParticleH5List = std::vector<ParticleData>;
using TrajectoryH5Info = std::vector<ParticleH5List>;
using TimeTrajectoryH5Info = std::tuple<TimestepH5List, TrajectoryH5Info>;

using RotationH5List = std::vector<Eigen::Vector3d>;
using RotationH5Info = std::vector<RotationH5List>;

using TopologyRecord = readdy::model::top::TopologyRecord;
using TopologyH5List = std::vector<TopologyRecord>;
using TopologyH5Info = std::vector<TopologyH5List>;
using TimeTopologyH5Info = std::tuple<TimestepH5List, TopologyH5Info>;

using IdParticleMapping = std::vector<std::unordered_map<std::size_t, std::size_t>>;
using NameRotationMap = std::unordered_map<std::string, Eigen::Vector3d>;

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

        virtual void LoadTrajectoryFile(
            std::string file_path,
            TrajectoryFileProperties& fileProps
        ) override;
        virtual double GetSimulationTimeAtFrame(std::size_t frameNumber) override;
        virtual std::size_t GetClosestFrameNumberForTime(double timeNs) override;

    private:
        readdy::Simulation* m_simulation;
        bool m_agents_initialized = false;
        bool m_reactions_initialized = false;
        int m_timeStepCount = 0;

        bool m_hasAlreadyRun = false;
        bool m_hasLoadedRunFile = false;
        bool m_hasFinishedStreaming = false;
        readdy::io::BloscFilter m_bloscFilter;

        // Used to store FileIO data
        TimeTrajectoryH5Info m_trajectoryInfo;
        TimeTopologyH5Info m_topologyInfo;

        // stored seperatley because these are calculated, not native to ReaDDy
        RotationH5Info m_rotationInfo;
    };

} // namespace agentsim
} // namespace aics

#endif // AICS_READDYPKG_H
