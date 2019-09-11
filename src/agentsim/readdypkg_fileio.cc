#include "agentsim/aws/aws_util.h"
#include "agentsim/util/math_util.h"

inline bool file_exists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

bool get_file_path(std::string& name);

std::vector<std::pair<MonomerType,std::vector<MonomerType>>> orientationNeighborData {
    {MonomerType ("actin", {"pointed"}, 0), {
        MonomerType ("actin", {}, 1)
    }},
    {MonomerType ("actin", {"branch", "barbed"}, 0), {
        MonomerType ("arp3", {}, 0)
    }},
    {MonomerType ("actin", {"barbed"}, 0), {
        MonomerType ("actin", {}, -1)
    }},
    {MonomerType ("actin", {"branch"}, 0), {
        MonomerType ("arp3", {}, 0), 
        MonomerType ("actin", {}, 1)
    }},
    {MonomerType ("actin", {"new"}, 0), {}},
    {MonomerType ("actin", {"free"}, 0), {}},
    {MonomerType ("actin", {}, 0), {
        MonomerType ("actin", {}, -1), 
        MonomerType ("actin", {}, 1)
    }},
    {MonomerType ("arp2", {}, 0), {
        MonomerType ("actin", {}, 0), 
        MonomerType ("arp3", {}, 0)
    }},
    {MonomerType ("arp3", {}, 0), {
        MonomerType ("arp2", {}, 0), 
        MonomerType ("actin", {"branch"}, 0)
    }},
    {MonomerType ("cap", {}, 0), {
        MonomerType ("actin", {}, 0)
    }},
    {MonomerType ("cap", {"bound"}, 0), {}}
};

MonomerTypeRotationMap calculateInitialRotations() {
    /*
    *   These values were calculated manually by Blair using Unity
    *    Ultimatley these should come from the model definition
    *    which will have information about spatial offests and
    *    relations in complexes (topologies in ReaDDy)
    */
    auto actinRotation = aics::agentsim::mathutil::GetRotationMatrix(
        std::vector({
            Eigen::Vector3d(0,0,0),
            Eigen::Vector3d(-1.453012, 3.27238, 2.330608),
            Eigen::Vector3d(-5.26267,  2.193798, 0.7260392)
        }));

    auto arp3Rotation = aics::agentsim::mathutil::GetRotationMatrix(
        std::vector({
            Eigen::Vector3d(0,0,0),
            Eigen::Vector3d(-2.390059, -1.559484,  -3.054445),
            Eigen::Vector3d(-2.3088,   -4.948046,  -0.596437)
        }));

    auto branchRotation = aics::agentsim::mathutil::GetRotationMatrix(
        std::vector({
            Eigen::Vector3d(0,0,0),
            Eigen::Vector3d(-1.345952, 3.272221, 2.238688),
            Eigen::Vector3d(-5.155611, 2.193637, 0.63413)
        }));

    auto tubulinRotation = aics::agentsim::mathutil::GetRotationMatrix(
        std::vector({
            Eigen::Vector3d(0, 0, -1),
            Eigen::Vector3d(0, 0, 0),
            Eigen::Vector3d(0, 1, 0)
        }));

    return MonomerTypeRotationMap {
        {MonomerType ("actin", {"branch"}, 0), branchRotation},
        {MonomerType ("actin", {}, 0), actinRotation},
        {MonomerType ("arp2", {}, 0), arp3Rotation},
        {MonomerType ("tubulin", {}, 0), tubulinRotation}
    };
}

MonomerTypeRotationMap calculateOffsetRotations() {
    /*
    *   These values were calculated manually by Blair using Unity
    *    Ultimatley these should come from the model definition
    *    which will have information about spatial offests and
    *    relations in complexes (topologies in ReaDDy)
    */
    Eigen::Matrix3d rotation_to_prev_actin;
    rotation_to_prev_actin <<  0.58509899, -0.80874088, -0.05997798,
                              -0.79675461, -0.55949104, -0.22836785,
                               0.15113327,  0.18140554, -0.97172566;
    
    Eigen::Matrix3d rotation_to_next_actin;
    rotation_to_next_actin <<  0.5851038,  -0.79675251, 0.15112575,
                              -0.80873679, -0.55949491, 0.18141181,
                              -0.05998622, -0.2283657, -0.97172566;
    
    Eigen::Matrix3d rotation_barbed_from_actin_dimer_axis;
    rotation_barbed_from_actin_dimer_axis <<  0.17508484, -0.52345361, -0.83387146,
                                             -0.3445482,   0.76082255, -0.54994144,
                                              0.92229704,  0.38359532, -0.0471465;
    
    Eigen::Matrix3d rotation_from_arp_dimer_axis;
    rotation_from_arp_dimer_axis <<  0.81557928, -0.35510793,  0.45686846,
                                    -0.57175434, -0.37306342,  0.73069875,
                                    -0.08903601, -0.85715929, -0.5072973;

    return MonomerTypeRotationMap {
        {MonomerType ("actin", {}, 1), rotation_to_next_actin},
        {MonomerType ("actin", {}, -1), rotation_to_prev_actin},
        {MonomerType ("arp2", {}, 0), rotation_from_arp_dimer_axis}
    };
}

void run_and_save_h5file(
    readdy::Simulation* sim,
    std::string file_name,
    float time_step,
    float n_time_steps,
    float write_stride);

void read_h5file(
    std::string file_name,
    TimeTrajectoryH5Info& trajectoryInfo,
    TimeTopologyH5Info& topologyInfo,
    RotationH5Info& rotationInfo,
    IdParticleMapping& particleLookup
);

void copy_frame(
    readdy::Simulation* sim,
    const std::vector<ParticleData>& particle_data,
    RotationH5List& rotationInfo,
    std::vector<std::shared_ptr<aics::agentsim::Agent>>& agents);

TimeTrajectoryH5Info readTrajectory(
    const std::shared_ptr<h5rd::File>& file,
    h5rd::Group& group,
    IdParticleMapping& particleLookup
);

TimeTopologyH5Info readTopologies(
    h5rd::Group& group,
    std::size_t from,
    std::size_t to,
    std::size_t stride);

std::vector<std::size_t> getNeighbors(
    std::size_t particleId,
    TopologyH5List& topologies
);

std::string getMainParticleType(
    std::string readdyParticleType
);

std::string getMainParticleType(
    std::string readdyParticleType
);

MonomerType getMonomerType(
    std::string readdyParticleType
);

int getMonomerN(
    MonomerType startMonomerNumber,
    MonomerType referenceMonomerType
);

bool monomerTypeIsSatisfied(
    MonomerType referenceMonomerType,
    MonomerType testMonomerType,
    int startMonomerNumber
);

std::vector<MonomerType> getOrientationNeighborTypes(
    MonomerType particleMonomerType
);

std::vector<MonomerType> getOrientationNeighbors(
    std::string readdyParticleType
);

Eigen::Matrix3d eulerToMatrix( 
    Eigen::Vector3d rotation);

Eigen::Matrix3d getRandomOrientation();

Eigen::Matrix3d getErrorOrientation();

std::string monomerTypeToString(
    MonomerType monomerType
);

bool neighborMatchesMonomerType(
    std::string neighborReaDDyType,
    std::string particleReaDDyType,
    MonomerType monomerType
);

void calculateOrientations(
    const TopologyH5Info& topologyH5Info,
    const TrajectoryH5Info& trajectoryH5Info,
    RotationH5Info& rotationInfo,
    IdParticleMapping& particleLookup,
    MonomerTypeRotationMap& initialRotations,
    MonomerTypeRotationMap& offsetRotations
);

namespace aics {
namespace agentsim {

    std::size_t frame_no = 0;
    std::string last_loaded_file = "";
    IdParticleMapping ID_PARTICLE_CACHE;

    void ResetFileIO()
    {
        frame_no = 0;
        last_loaded_file = "";
    }

    void ReaDDyPkg::GetNextFrame(
        std::vector<std::shared_ptr<Agent>>& agents)
    {
        if (this->m_hasFinishedStreaming)
            return;

        if (agents.size() == 0) {
            for (std::size_t i = 0; i < 5000; ++i) {
                std::shared_ptr<Agent> agent;
                agent.reset(new Agent());
                agent->SetVisibility(false);
                agent->SetCollisionRadius(1);
                agents.push_back(agent);
            }
        }

        if (!this->m_hasLoadedRunFile) {
            this->LoadTrajectoryFile("/tmp/test.h5");
        }

        auto& trajectoryInfo = std::get<1>(this->m_trajectoryInfo);

        if (frame_no >= trajectoryInfo.size()) {
            this->m_hasFinishedStreaming = true;
        } else {

            copy_frame(
                this->m_simulation,
                std::get<1>(this->m_trajectoryInfo).at(frame_no),
                this->m_rotationInfo[frame_no],
                agents);

            frame_no++;
        }
    }

    bool ReaDDyPkg::IsFinished()
    {
        return this->m_hasFinishedStreaming;
    }

    void ReaDDyPkg::Run(float timeStep, std::size_t nTimeStep)
    {
        if (this->m_hasAlreadyRun)
            return;

        run_and_save_h5file(this->m_simulation, "/tmp/test.h5", timeStep, nTimeStep, 10);
        this->m_hasAlreadyRun = true;
        this->m_hasLoadedRunFile = false;
    }

    void ReaDDyPkg::LoadTrajectoryFile(std::string file_path)
    {
        if (last_loaded_file != file_path) {
            this->m_hasLoadedRunFile = false;
        } else {
            std::cout << "Using loaded file:  " << file_path << std::endl;
            return;
        }

        if (!this->m_hasLoadedRunFile) {
            std::cout << "Loading trajectory file " << file_path << std::endl;
            frame_no = 0;

            read_h5file(file_path,
                this->m_trajectoryInfo,
                this->m_topologyInfo,
                this->m_rotationInfo,
                ID_PARTICLE_CACHE
            );
            this->m_hasLoadedRunFile = true;
            last_loaded_file = file_path;
            std::cout << "Finished loading trajectory file: " << file_path << std::endl;
        }
    }

    double ReaDDyPkg::GetTime(std::size_t frameNumber)
    {
        return std::get<0>(this->m_trajectoryInfo).at(frameNumber);
    }

} // namespace agentsim
} // namespace aics

/**
*	File IO Functions
**/
bool get_file_path(std::string& file_name)
{
    // Download the file from AWS if it is not present locally
    if (!file_exists(file_name)) {
        std::cout << file_name << " doesn't exist locally, checking S3..." << std::endl;
        if (!aics::agentsim::aws_util::Download(Aws::String(file_name.c_str(), file_name.size()))) {
            std::cout << file_name << " not found on AWS S3" << std::endl;
            return false;
        }
    }

    // Modifies the file-path  to a format that H5rd can reliably load
    // H5rd is a library written by the ReaDDy developers to load H5 files
    if (file_exists("/" + file_name)) {
        file_name = "/" + file_name;
        std::cout << "file name modified to " << file_name << std::endl;
    } else if (file_exists("./" + file_name)) {
        file_name = "./" + file_name;
        std::cout << "file name modified to " << file_name << std::endl;
    }

    return true;
}

void run_and_save_h5file(
    readdy::Simulation* sim,
    std::string file_name,
    float time_step,
    float n_time_steps,
    float write_stride)
{
    auto file = readdy::File::create(file_name, h5rd::File::Flag::OVERWRITE);

    auto traj_ptr = sim->observe().flatTrajectory(1);
    traj_ptr->enableWriteToFile(*file.get(), "", write_stride);
    auto traj_handle = sim->registerObservable(std::move(traj_ptr));

    try {
        auto loop = sim->createLoop(time_step);
        loop.writeConfigToFile(*file);
        loop.run(n_time_steps);
    } catch (...) {
        std::raise(SIGABRT);
        std::cout << "Exception while running ReaDDy simulation" << std::endl;
    }

    file->close();
}

void read_h5file(
    std::string file_name,
    TimeTrajectoryH5Info& trajectoryInfo,
    TimeTopologyH5Info& topologyInfo,
    RotationH5Info& rotationInfo,
    IdParticleMapping& particleLookup
    )
{
    if (!get_file_path(file_name)) {
        return;
    }

    // open the output file
    auto file = h5rd::File::open(file_name, h5rd::File::Flag::READ_ONLY);

    // read back trajectory
    auto traj = file->getSubgroup("readdy/trajectory");
    trajectoryInfo = readTrajectory(file, traj, particleLookup);

    auto topGroup = file->getSubgroup("readdy/observables/topologies");
    topologyInfo = readTopologies(topGroup, 0, std::numeric_limits<std::size_t>::max(), 1);
    auto initialRotations = calculateInitialRotations();
    auto offsetRotations = calculateOffsetRotations();

    calculateOrientations(
        std::get<1>(topologyInfo),
        std::get<1>(trajectoryInfo),
        rotationInfo,
        particleLookup,
        initialRotations,
        offsetRotations
    );

    std::cout << "Found trajectory for " << std::get<0>(trajectoryInfo).size() << " frames" << std::endl;
    std::cout << "Found topology for " << std::get<0>(topologyInfo).size() << " frames" << std::endl;

    file->close();
}

void copy_frame(
    readdy::Simulation* sim,
    const std::vector<ParticleData>& particle_data,
    RotationH5List& rotationInfo,
    std::vector<std::shared_ptr<aics::agentsim::Agent>>& agents)
{
    std::size_t agent_index = 0;
    std::size_t ignore_count = 0;
    std::size_t bad_topology_count = 0;
    std::size_t good_topology_count = 0;
    std::size_t particleCount = particle_data.size();
    bool useRotation = particle_data.size() == rotationInfo.size();

    auto& particles = sim->context().particleTypes();

    for (std::size_t particleIndex = 0; particleIndex < particleCount; ++particleIndex) {
        auto& p = particle_data.at(particleIndex);
        auto pos = p.position;

        // check if this particle has a valid readdy location
        if (isnan(pos[0]) || isnan(pos[1]) || isnan(pos[2])) {
            continue;
        }

        // if this particle is a _CHILD particle, don't stream its position
        //  to stream child positions, we would need to correlate the children
        //  the owning parent from the trajectory/H5 file info
        if (p.type.find("_CHILD") != std::string::npos) {
            continue;
        }

        // check if we have counted past the avaliable agents provided
        //	this would mean we used every agent in the 'agents' already
        if (agent_index >= agents.size()) {
            std::cout << "Not enough agents to represent Readdy file run: " << std::endl;
            readdy::log::info("\t type {}, id {}, pos ({}, {}, {})",
                p.type, p.id, p.position[0], p.position[1], p.position[2]);
            break;
        }

        // copy the position of the particle to an AgentViz agent
        auto currentAgent = agents[agent_index].get();
        currentAgent->SetLocation(Eigen::Vector3d(pos[0], pos[1], pos[2]));
        if(useRotation) {
            currentAgent->SetRotation(rotationInfo[particleIndex]);
        }
        currentAgent->SetTypeID(p.type_id);
        currentAgent->SetName(p.type);
        currentAgent->SetVisibility(true);
        agent_index++;
    }

    for (auto i = agent_index; i < agents.size(); ++i) {
        if (!agents[i]->IsVisible()) {
            break;
        }

        agents[i]->SetVisibility(false);
    }
}

TimeTrajectoryH5Info readTrajectory(
    const std::shared_ptr<h5rd::File>& file,
    h5rd::Group& group,
    IdParticleMapping& particleLookup
)
{
    particleLookup.clear();
    TrajectoryH5Info results;

    // retrieve the h5 particle type info type
    auto particleInfoH5Type = readdy::model::ioutils::getParticleTypeInfoType(file->ref());

    // get particle types from config
    std::vector<readdy::model::ioutils::ParticleTypeInfo> types;
    {
        auto config = file->getSubgroup("readdy/config");
        config.read(
            "particle_types",
            types,
            &std::get<0>(particleInfoH5Type),
            &std::get<1>(particleInfoH5Type));
    }
    std::unordered_map<std::size_t, std::string> typeMapping;
    for (const auto& type : types) {
        typeMapping[type.type_id] = std::string(type.name);
        std::cout << "Particle type found: " << type.type_id
            << " with name " << type.name << std::endl;
    }

    // limits of length 2T containing [start_ix, end_ix] for each time step
    std::vector<std::size_t> limits;
    group.read("limits", limits);

    // time of length T containing the time steps of recording the trajectory
    std::vector<readdy::time_step_type> time;
    group.read("time", time);

    // records containing all particles for all times, slicing according to limits
    // yield particles for a particular time step
    std::vector<readdy::model::observables::TrajectoryEntry> entries;
    auto trajectoryEntryTypes = readdy::model::observables::util::getTrajectoryEntryTypes(file->ref());
    group.read(
        "records", entries,
        &std::get<0>(trajectoryEntryTypes),
        &std::get<1>(trajectoryEntryTypes));

    auto n_frames = limits.size() / 2;

    // mapping the read-back data to the ParticleData struct
    results.reserve(n_frames);
    particleLookup.reserve(n_frames);

    auto timeIt = time.begin();
    for (std::size_t frame = 0; frame < limits.size(); frame += 2, ++timeIt) {
        auto begin = limits[frame];
        auto end = limits[frame + 1];

        results.emplace_back();
        auto& currentFrame = results.back();
        currentFrame.reserve(end - begin);

        particleLookup.emplace_back();
        auto& idparticleCache = particleLookup.back();

        for (auto it = entries.begin() + begin; it != entries.begin() + end; ++it) {
            currentFrame.emplace_back(
                typeMapping[it->typeId],
                readdy::model::particleflavor::particleFlavorToString(it->flavor),
                it->pos.data, it->id, it->typeId, *timeIt);

            idparticleCache[currentFrame.back().id] =
              currentFrame.size() - 1;
        }
    }

    return std::make_tuple(time, results);
}

TimeTopologyH5Info readTopologies(
    h5rd::Group& group,
    std::size_t from,
    std::size_t to,
    std::size_t stride)
{
    readdy::io::BloscFilter bloscFilter;
    bloscFilter.registerFilter();

    std::size_t nFrames { 0 };
    // limits
    std::vector<std::size_t> limitsParticles;
    std::vector<std::size_t> limitsEdges;
    {
        if (stride > 1) {
            group.read("limitsParticles", limitsParticles, { stride, 1 });
        } else {
            group.read("limitsParticles", limitsParticles);
        }
        if (stride > 1) {
            group.read("limitsEdges", limitsEdges, { stride, 1 });
        } else {
            group.read("limitsEdges", limitsEdges);
        }

        if (limitsParticles.size() != limitsEdges.size()) {
            throw std::logic_error(fmt::format("readTopologies: Incompatible limit sizes, "
                                               "limitsParticles.size={}, limitsEdges.size={}",
                limitsParticles.size(), limitsEdges.size()));
        }

        nFrames = limitsParticles.size() / 2;
        from = std::min(nFrames, from);
        to = std::min(nFrames, to);

        if (from == to) {
            throw std::invalid_argument(fmt::format("readTopologies: not enough frames to cover range ({}, {}]",
                from, to));
        } else {
            limitsParticles = std::vector<std::size_t>(limitsParticles.begin() + 2 * from,
                limitsParticles.begin() + 2 * to);
            limitsEdges = std::vector<std::size_t>(limitsEdges.begin() + 2 * from,
                limitsEdges.begin() + 2 * to);
        }
    }

    from = std::min(nFrames, from);
    to = std::min(nFrames, to);

    // time
    std::vector<readdy::time_step_type> time;
    if (stride > 1) {
        group.readSelection("time", time, { from }, { stride }, { to - from });
    } else {
        group.readSelection("time", time, { from }, { 1 }, { to - from });
    }

    if (limitsParticles.size() % 2 != 0 || limitsEdges.size() % 2 != 0) {
        throw std::logic_error(fmt::format(
            "limitsParticles size was {} and limitsEdges size was {}, they should be divisible by 2; from={}, to={}",
            limitsParticles.size(), limitsEdges.size(), from, to));
    }
    // now check that nFrames(particles) == nFrames(edges) == nFrames(time)...
    if (to - from != limitsEdges.size() / 2 || (to - from) != time.size()) {
        throw std::logic_error(fmt::format(
            "(to-from) should be equal to limitsEdges/2 and equal to the number of time steps in the recording, "
            "but was: (to-from) = {}, limitsEdges/2 = {}, nTimeSteps={}; from={}, to={}",
            to - from, limitsEdges.size() / 2, time.size(), from, to));
    }

    std::vector<std::vector<readdy::TopologyTypeId>> types;
    group.readVLENSelection("types", types, { from }, { stride }, { to - from });

    std::vector<std::vector<TopologyRecord>> result;

    std::size_t ix = 0;
    for (std::size_t frame = from; frame < to; ++frame, ++ix) {
        result.emplace_back();
        // this frame's records
        auto& records = result.back();

        const auto& particlesLimitBegin = limitsParticles.at(2 * ix);
        const auto& particlesLimitEnd = limitsParticles.at(2 * ix + 1);
        // since the edges are flattened, we actually have to multiply this by 2
        const auto& edgesLimitBegin = limitsEdges.at(2 * ix);
        const auto& edgesLimitEnd = limitsEdges.at(2 * ix + 1);

        std::vector<std::size_t> flatParticles;
        group.readSelection("particles", flatParticles, { particlesLimitBegin }, { stride }, { particlesLimitEnd - particlesLimitBegin });
        std::vector<std::size_t> flatEdges;
        //readdy::log::critical("edges {} - {}", edgesLimitBegin, edgesLimitEnd);
        group.readSelection("edges", flatEdges, { edgesLimitBegin, 0 }, { stride, 1 }, { edgesLimitEnd - edgesLimitBegin, 2 });

        const auto& currentTypes = types.at(ix);
        auto typesIt = currentTypes.begin();
        for (auto particlesIt = flatParticles.begin();
             particlesIt != flatParticles.end(); ++particlesIt) {

            records.emplace_back();
            auto nParticles = *particlesIt;
            for (std::size_t i = 0; i < nParticles; ++i) {
                ++particlesIt;
                records.back().particleIndices.push_back(*particlesIt);
            }
            records.back().type = *typesIt;
            ++typesIt;
        }

        if (currentTypes.size() != records.size()) {
            throw std::logic_error(fmt::format("for frame {} had {} topology types but {} topologies",
                frame, currentTypes.size(), records.size()));
        }

        std::size_t recordIx = 0;
        for (auto edgesIt = flatEdges.begin();
             edgesIt != flatEdges.end(); ++recordIx) {
            auto& currentRecord = records.at(recordIx);

            auto nEdges = *edgesIt;
            edgesIt += 2;

            for (std::size_t i = 0; i < nEdges; ++i) {
                currentRecord.edges.emplace_back(*edgesIt, *(edgesIt + 1));
                edgesIt += 2;
            }
        }
    }

    if (time.size() != result.size()) {
        throw std::logic_error(fmt::format("readTopologies: size mismatch, time size is {}, result size is {}",
            time.size(), result.size()));
    }
    return std::make_tuple(time, result);
}

std::vector<std::size_t> getNeighbors(
    std::size_t particleId,
    TopologyH5List& topologies
)
{
    std::vector<std::size_t> neighbors;
    bool done = false;

    std::size_t topologyCount = topologies.size();

    for(std::size_t topologyIndex = 0; topologyIndex < topologyCount; ++topologyIndex)
    {
        auto topology = topologies.at(topologyIndex);
        for(std::size_t edgeIndex = 0; edgeIndex < topology.edges.size(); ++edgeIndex)
        {
            auto edge = topology.edges[edgeIndex];
            auto p1 = topology.particleIndices.at(std::get<0>(edge));
            auto p2 = topology.particleIndices.at(std::get<1>(edge));

            if(p1 == particleId) {
                neighbors.push_back(p2);
            }
            else if (p2 == particleId) {
                neighbors.push_back(p1);
            }
        }

        // Assuming each particle is only part of one topology
        //  this means that if nieghbors are found, this function is done
        if(neighbors.size() > 0)
        {
            return neighbors;
        }
    }

    return neighbors;
}

std::string getMainParticleType(
    std::string readdyParticleType
)
{
    char delimiter = '#';
    auto pos = readdyParticleType.find(delimiter);
    if (pos != std::string::npos)
    {
        return readdyParticleType.substr(0, readdyParticleType.find(delimiter));
    }
    else
    {
        std::cout << "Failed to get main particle type for " << readdyParticleType << std::endl;
    }
    return readdyParticleType;
}

MonomerType getMonomerType(
    std::string readdyParticleType
)
{
    // ReaDDy particle types contain the main type as well as state info
    // i.e. ReaDDy particle type = [main type]#[state flags (delimited by "_")]
    
    // get the main type
    char delimiter = '#';
    auto pos = readdyParticleType.find(delimiter);
    std::string mainType;
    std::string flagStr;
    if (pos != std::string::npos)
    {
        mainType = readdyParticleType.substr(0, pos);
        flagStr = readdyParticleType.substr(pos + 1, readdyParticleType.size() - pos);
    }
    
    // get the state flags
    delimiter = '_';
    std::vector<std::string> flags;
    auto start = 0U;
    auto end = flagStr.find(delimiter);
    while (end != std::string::npos)
    {
        flags.push_back(flagStr.substr(start, end - start));
        start = end + 1;
        end = flagStr.find(delimiter, start);
    }
    flags.push_back(flagStr.substr(start, flagStr.length() - start));
    
    // if one of the flags is a number, set it as the monomer number
    int number = 0;
    for (std::size_t i = 0; i < flags.size(); ++i)
    {
        if (!flags[i].empty() && std::all_of(flags[i].begin(), flags[i].end(), ::isdigit))
        {
            number = std::stoi(flags[i]);
            flags.erase(flags.begin() + i);
            break;
        }
    }
    
    return MonomerType (mainType, flags, number);
}

int getMonomerN(
    int startMonomerNumber,
    MonomerType referenceMonomerType)
{   
    auto n = startMonomerNumber + std::get<2>(referenceMonomerType);
    
    if (n > 3)
        n -= 3;
    if (n < 1)
        n += 3;
        
    return n;
}

bool monomerTypeIsSatisfied(
    MonomerType referenceMonomerType,
    MonomerType testMonomerType,
    int startMonomerNumber
)
{
    // check that the main types are the same
    if (std::get<0>(testMonomerType) != std::get<0>(referenceMonomerType))
    {
        return false;
    }
    
    // check that the monomer number is correct
    if (std::get<2>(referenceMonomerType) != 0)
    {
        if (std::get<2>(testMonomerType) != getMonomerN(startMonomerNumber, referenceMonomerType))
        {
            return false;
        }
    }
    
    // check that all the required flags are present
    auto referenceFlags = std::get<1>(referenceMonomerType);
    auto testFlags = std::get<1>(testMonomerType);
    for (std::size_t i = 0; i < referenceFlags.size(); ++i)
    {
        if (std::find(testFlags.begin(), testFlags.end(), referenceFlags[i]) == testFlags.end())
        {
            return false;
        }
    }
    return true;
}

std::vector<MonomerType> getOrientationNeighborTypes(
    MonomerType particleMonomerType
)
{   
    // get the types of the neighbors to use for orientation calculation
    for (std::size_t i = 0; i < orientationNeighborData.size(); ++i)
    {
        if (monomerTypeIsSatisfied(orientationNeighborData[i].first, particleMonomerType, 0))
        {
            return orientationNeighborData[i].second;
        }
    }
    return {MonomerType("", {""}, 0)};
}

Eigen::Matrix3d eulerToMatrix( 
    Eigen::Vector3d rotation)
{
    Eigen::AngleAxisd rollAngle(rotation[0], Eigen::Vector3d::UnitX());
    Eigen::AngleAxisd pitchAngle(rotation[1], Eigen::Vector3d::UnitY());
    Eigen::AngleAxisd yawAngle(rotation[2], Eigen::Vector3d::UnitZ());

    Eigen::Quaterniond q = yawAngle * pitchAngle * rollAngle;
    return q.matrix();
}

Eigen::Matrix3d getRandomOrientation()
{
    Eigen::Vector3d rotation;
    rotation[0] = (rand() % 8) * 45;
    rotation[1] = (rand() % 8) * 45;
    rotation[2] = (rand() % 8) * 45;
    return eulerToMatrix(rotation);
}

Eigen::Matrix3d getErrorOrientation()
{
    // Assign an error orientation that can be checked to know if the rotation algorithm failed
    float errValue = 0;
    
    Eigen::Vector3d rotation;
    rotation[0] = errValue;
    rotation[1] = errValue;
    rotation[2] = errValue;
    return eulerToMatrix(rotation);
}

std::string monomerTypeToString(
    MonomerType monomerType
)
{
    std::string s = "[" + std::get<0>(monomerType) + " (";
    auto flags = std::get<1>(monomerType);
    for(std::size_t i = 0; i < flags.size(); ++i) //numberOfFrames
    {
        s += flags[i] + ", ";
    }
    return s + ") " + std::to_string(std::get<2>(monomerType)) + "]";
}

void calculateOrientations(
    const TopologyH5Info& topologyH5Info,
    const TrajectoryH5Info& trajectoryH5Info,
    RotationH5Info& outRotations,
    IdParticleMapping& particleLookup,
    MonomerTypeRotationMap& initialRotations,
    MonomerTypeRotationMap& offsetRotations
)
{
    auto numberOfFrames = topologyH5Info.size();
    outRotations.resize(numberOfFrames);

    for(std::size_t frameIndex = 0; frameIndex < 1; ++frameIndex) //numberOfFrames
    {
        auto trajectoryFrame = trajectoryH5Info.at(frameIndex);
        auto topologyFrame = topologyH5Info.at(frameIndex);
        auto& rotationFrame = outRotations.at(frameIndex);
        std::vector<Eigen::Matrix3d> rotationMatrixFrame {};
        auto& idmappingFrame = particleLookup.at(frameIndex);
        std::unordered_map<std::size_t,MonomerType> orientRelativeToNeighbor = {};

        for(std::size_t particleIndex = 0;
            particleIndex < trajectoryFrame.size();
            ++particleIndex)
        {
            auto currentParticle = trajectoryFrame.at(particleIndex);
            std::string particleName = currentParticle.type;
            MonomerType particleMonomerType = getMonomerType(particleName);
            
            auto neighborIds = getNeighbors(currentParticle.id, topologyFrame);
            std::size_t neighborCount = neighborIds.size();
            
            // get the types of the neighbors to use for orientation calculation
            std::vector<MonomerType> orientationNeighborTypes = getOrientationNeighborTypes(particleMonomerType);
            std::size_t orientationNeighborTypesCount = orientationNeighborTypes.size();
            
            if (neighborCount < orientationNeighborTypesCount)
            {
                // not enough neighbors to correctly calculate orientation
                std::cout << frameIndex << ": Expected " << orientationNeighborTypesCount 
                    << " neighbors for " << particleName << ", found " << neighborCount << std::endl;
                rotationMatrixFrame.push_back(getErrorOrientation());
                continue;
            }
            
            if (orientationNeighborTypesCount < 1)
            {
                // no neighbors, use random rotation
                std::cout << frameIndex << ": Use random orientation for " 
                    << particleName << " " << currentParticle.id << std::endl;
                rotationMatrixFrame.push_back(getRandomOrientation());
                continue;
            }
            
            if (orientationNeighborTypesCount < 2)
            {
                std::cout << frameIndex << ": Use one neighbor for orientation of " 
                    << particleName << " " << currentParticle.id << std::endl;
                rotationMatrixFrame.push_back(getErrorOrientation());
                orientRelativeToNeighbor[particleIndex] = orientationNeighborTypes[0];
                continue;
            }
            
            // get neighbor particles of the orientation neighbor types
            std::vector<ParticleData> orientationNeighbors = {};
            for (std::size_t i = 0; i < orientationNeighborTypesCount; ++i)
            {   
                for (std::size_t j = 0; j < neighborCount; ++j)
                {
                    if (idmappingFrame.count(neighborIds.at(j)) == 0)
                    {
                        std::cout << frameIndex << ": Invalid neighbor ID for " 
                            << particleName << " " << currentParticle.id << std::endl;
                        continue;
                    }
                    auto neighborID = idmappingFrame.at(neighborIds.at(j));
                    auto neighborParticle = trajectoryFrame.at(neighborID);
                    MonomerType neighborMonomerType = getMonomerType(neighborParticle.type);
                
                    if (monomerTypeIsSatisfied(
                            orientationNeighborTypes[i], 
                            neighborMonomerType, 
                            std::get<2>(particleMonomerType)))
                    {
                        orientationNeighbors.push_back(neighborParticle);
                    }
                }
            }
            if (orientationNeighbors.size() < 2)
            {
                std::cout << frameIndex << ": Failed to find orientation neighbors for " 
                    << particleName << " " << currentParticle.id << std::endl;
                rotationMatrixFrame.push_back(getErrorOrientation());
                continue;
            }

            auto rpos0 = orientationNeighbors[0].position;
            auto pos0 = Eigen::Vector3d(rpos0[0], rpos0[1], rpos0[2]);
            auto rpos1 = currentParticle.position;
            auto pos1 = Eigen::Vector3d(rpos1[0], rpos1[1], rpos1[2]);
            auto rpos2 = orientationNeighbors[1].position;
            auto pos2 = Eigen::Vector3d(rpos2[0], rpos2[1], rpos2[2]);

            std::vector<Eigen::Vector3d> basisPositions;
            basisPositions.push_back(pos0);
            basisPositions.push_back(pos1);
            basisPositions.push_back(pos2);
            auto rmat = aics::agentsim::mathutil::GetRotationMatrix(basisPositions);
            
            Eigen::Matrix3d initialRotation;
            for (std::size_t i = 0; i < initialRotations.size(); ++i)
            {
                if (monomerTypeIsSatisfied(initialRotations[i].first, particleMonomerType, 0))
                {
                    initialRotation = initialRotations[i].second;
                    break;
                }
            }
            
            rotationMatrixFrame.push_back(rmat.inverse() * initialRotation);
            std::cout << frameIndex << ": Successfully oriented " 
                << particleName << " " << currentParticle.id << std::endl;
        }
        
        // go back and calculate orientation for particles with one neighbor
        // since their orientation is dependent on the neighbor's orientation
        for (auto i = orientRelativeToNeighbor.begin(); i != orientRelativeToNeighbor.end(); ++i)
        {
            auto particleIndex = i->first;
            auto orientationNeighborType = i->second;
            
            auto currentParticle = trajectoryFrame.at(particleIndex);
            std::string particleName = currentParticle.type;
            MonomerType particleMonomerType = getMonomerType(particleName);
            auto particleID = currentParticle.id;
            
            std::cout << monomerTypeToString(particleMonomerType) << std::endl;
            std::cout << monomerTypeToString(orientationNeighborType) << std::endl;
            
            auto neighborIds = getNeighbors(particleID, topologyFrame);
                
            // get neighbor particle of the orientation neighbor type
            std::size_t orientationNeighborID;
            for(std::size_t j = 0; j < neighborIds.size(); ++j)
            {
                if(idmappingFrame.count(neighborIds.at(j)) == 0)
                {
                    std::cout << frameIndex << ": Invalid neighbor ID for " 
                        << particleName << " " << particleID << std::endl;
                    continue;
                }
                auto neighborID = idmappingFrame.at(neighborIds.at(j));
                auto neighborParticle = trajectoryFrame.at(neighborID);
                MonomerType neighborMonomerType = getMonomerType(neighborParticle.type);

                if (monomerTypeIsSatisfied(
                        orientationNeighborType, 
                        neighborMonomerType, 
                        std::get<2>(particleMonomerType)))
                {
                    orientationNeighborID = neighborID;
                    break;
                }
            }
            if (orientationNeighborID == 0)
            {
                std::cout << frameIndex << ": Failed to find orientation neighbor for " 
                    << particleName << " " << particleID << std::endl;
                continue;
            }
            
            // multiply the neighbor's orientation by an offset rotation
            auto neighborRMat = rotationMatrixFrame.at(orientationNeighborID);
            Eigen::Matrix3d offsetRotation;
            for(std::size_t j = 0; j < offsetRotations.size(); ++j)
            {
                if (monomerTypeIsSatisfied(offsetRotations[j].first, orientationNeighborType, 0))
                {
                    offsetRotation = offsetRotations[j].second;
                    break;
                }
            }
            rotationMatrixFrame.at(i->first) = neighborRMat * offsetRotation;
            std::cout << frameIndex << ": Successfully oriented " 
                << particleName << " " << particleID << " with 1 neighbor " << std::endl;
        }
        
        for(std::size_t i = 0; i < rotationMatrixFrame.size(); ++i)
        {
            rotationFrame.push_back(rotationMatrixFrame[i].eulerAngles(0,1,2));
        }
    }
}
