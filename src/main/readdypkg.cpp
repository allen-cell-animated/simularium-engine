#define USE_HARDCODED 1
#include "readdy/readdy.h"
#include "agentsim/agents/agent.h"
#include "agentsim/simpkg/readdypkg.h"
#include "readdy/model/observables/io/TrajectoryEntry.h"
#include "readdy/model/observables/io/Types.h"
#include <fstream>
#include <limits>
#include <readdy/model/topologies/TopologyRecord.h>

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

struct OrientationData {
    OrientationData(Eigen::Vector3d pos, Eigen::Matrix3d rot, Eigen::Matrix3d axisRot)
        : localPosition(pos)
        , localRotation(rot)
        , axisRotation(axisRot)
    {
    }

    OrientationData()
        : localPosition()
        , localRotation()
        , axisRotation()
    {
    }

    Eigen::Vector3d localPosition;
    Eigen::Matrix3d localRotation;
    Eigen::Matrix3d axisRotation;
};

struct MonomerType {
    MonomerType(std::string name, std::vector<std::string> flags, int number)
        : name(name)
        , flags(flags)
        , number(number)
    {
    }

    std::string name;
    std::vector<std::string> flags;
    int number;
};

struct RelativeOrientationData {
    RelativeOrientationData(std::size_t neighborID, OrientationData data)
        : neighborID(neighborID)
        , data(data)
    {
    }

    RelativeOrientationData(const RelativeOrientationData& other)
        : neighborID(other.neighborID)
        , data(other.data)
    {
    }

    RelativeOrientationData()
        : neighborID(0)
        , data()
    {
    }

    std::size_t neighborID;
    OrientationData data;
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
using OrientationDataMap = std::unordered_map<std::string,std::vector<std::pair<MonomerType,OrientationData>>>;

namespace aics {
namespace agentsim {

    struct ReaDDyFileInfo {
        TimeTrajectoryH5Info trajectoryInfo;
        TimeTopologyH5Info topologyInfo;
        std::unordered_map<std::size_t, std::string> typeMapping;
        RotationH5Info rotationInfo;
    };

    ReaDDyPkg::ReaDDyPkg() {
        this->m_simulation = new readdy::Simulation("SingleCPU");

        this->m_fileInfo = std::make_shared<ReaDDyFileInfo>();
        this->m_bloscFilter = std::make_shared<readdy::io::BloscFilter>();

        this->m_bloscFilter->registerFilter();
    }

    ReaDDyPkg::~ReaDDyPkg() {
        delete this->m_simulation;
        this->m_simulation = nullptr;
    }

} // namespace agentsim
} // namespace aics

#include "readdypkg_fileio.cc"

#if USE_HARDCODED == 1
#include "readdypkg_hardcoded.cc"
#elif USE_HARDCODED == 0
#include "readdypkg_generalized.cc"
#endif
