#include "agentsim/aws/aws_util.h"
#include "agentsim/util/math_util.h"

bool verboseOrientation = false;

inline bool file_exists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

bool get_file_path(std::string& name);

OrientationDataMap initOrientationData() {
    
    Eigen::Matrix3d zero_rotation;
    zero_rotation << 1, 0, 0,
                     0, 1, 0,
                     0, 0, 1;
    
    Eigen::Matrix3d rotation_actin_to_prev_actin;
    rotation_actin_to_prev_actin <<  0.58509899, -0.80874088, -0.05997798,
                                    -0.79675461, -0.55949104, -0.22836785,
                                     0.15113327,  0.18140554, -0.97172566;
    
    Eigen::Matrix3d rotation_barbed_from_actin_dimer_axis; //TODO
    rotation_barbed_from_actin_dimer_axis <<  0.17508484, -0.52345361, -0.83387146,
                                             -0.3445482,   0.76082255, -0.54994144,
                                              0.92229704,  0.38359532, -0.0471465;
    
    Eigen::Matrix3d rotation_actin_to_next_actin;
    rotation_actin_to_next_actin <<  0.5851038,  -0.79675251, 0.15112575,
                                    -0.80873679, -0.55949491, 0.18141181,
                                    -0.05998622, -0.2283657, -0.97172566;
    
    Eigen::Matrix3d rotation_pointed_from_actin_dimer_axis; //TODO
    rotation_pointed_from_actin_dimer_axis <<  0.17508484, -0.52345361, -0.83387146,
                                              -0.3445482,   0.76082255, -0.54994144,
                                               0.92229704,  0.38359532, -0.0471465;
    
    Eigen::Matrix3d rotation_branch_actin_to_arp3;
    rotation_branch_actin_to_arp3 <<  0.19687615, -0.47932213, -0.85527193,
                                      0.80641304, -0.41697831,  0.41931742,
                                     -0.55761796, -0.77225604,  0.30443854;
    
    Eigen::Matrix3d rotation_arp2_from_arp_dimer_axis; //TODO
    rotation_arp2_from_arp_dimer_axis <<  0.81557928, -0.35510793,  0.45686846,
                                         -0.57175434, -0.37306342,  0.73069875,
                                         -0.08903601, -0.85715929, -0.5072973;
    
    Eigen::Matrix3d rotation_arp3_from_arp_dimer_axis; //TODO
    rotation_arp3_from_arp_dimer_axis <<  0.19687615, -0.47932213, -0.85527193,
                                          0.80641304, -0.41697831,  0.41931742,
                                         -0.55761796, -0.77225604,  0.30443854;
    
    Eigen::Matrix3d rotation_tub_from_tub_dimer_axis;
    rotation_tub_from_tub_dimer_axis <<  1,  0,  0,
                                         0,  0, -1,
                                         0,  1,  0;
    
    return OrientationDataMap {
        {"actin", {
            {MonomerType ("actin", {"any"}, -1), 
             OrientationData (
                 Eigen::Vector3d(1.453012, -3.27238, -2.330608), 
                 rotation_actin_to_prev_actin, 
                 zero_rotation) //rotation_barbed_from_actin_dimer_axis
            },
            {MonomerType ("actin", {"any"}, 1), 
             OrientationData (
                 Eigen::Vector3d(-3.809657, -1.078586, -1.60457), 
                 rotation_actin_to_next_actin, 
                 zero_rotation) //rotation_pointed_from_actin_dimer_axis
            },
            {MonomerType ("arp3", {"any"}, 101), 
             OrientationData (
                 Eigen::Vector3d(1.345954, -3.27222, 2.238695), 
                 rotation_branch_actin_to_arp3, 
                 zero_rotation)
            }
        }},
        {"arp3", {
            {MonomerType ("actin", {"branch"}, 101), 
             OrientationData (
                 Eigen::Vector3d(0.08125937, -3.388564, 2.458009), 
                 zero_rotation, 
                 zero_rotation)
            },
            {MonomerType ("arp2", {"any"}, 101), 
             OrientationData (
                 Eigen::Vector3d(2.390059, 1.559484, 3.054445), 
                 zero_rotation, 
                 zero_rotation) //rotation_arp3_from_arp_dimer_axis
            }
        }},
        {"arp2", {
            {MonomerType ("arp3", {"any"}, 101), 
             OrientationData (
                 Eigen::Vector3d(0,0,0), 
                 zero_rotation, 
                 zero_rotation) //rotation_arp2_from_arp_dimer_axis
            }
        }},
        {"cap", {
            {MonomerType ("actin", {"any"}, 101), 
             OrientationData (
                 Eigen::Vector3d(0,0,0), 
                 zero_rotation, 
                 zero_rotation)
            }
        }},
        {"tubulin", {
            {MonomerType ("site-protofilament1", {"any"}, 101), 
             OrientationData (
                 Eigen::Vector3d(0, 0, -1), 
                 zero_rotation, 
                 zero_rotation)
            },
            {MonomerType ("site-out", {"any"}, 101), 
             OrientationData (
                 Eigen::Vector3d(0, 1, 0), 
                 zero_rotation, 
                 zero_rotation)
            },
            {MonomerType ("tubulin", {"any"}, 101), 
             OrientationData (
                 Eigen::Vector3d(0, 0, 0), 
                 zero_rotation, 
                 rotation_tub_from_tub_dimer_axis)
            }
        }}
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

bool is_child_particle(
    std::string particleType
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

MonomerType getMonomerType(
    std::string readdyParticleType
);

int getMonomerN(
    int actualMonomerNumber,
    MonomerType monomerType
);

bool monomerTypeIsSatisfied(
    MonomerType referenceMonomerType,
    int referenceActualMonomerNumber,
    MonomerType testMonomerType,
    int testActualMonomerNumber
);

Eigen::Matrix3d eulerToMatrix( 
    Eigen::Vector3d rotation);

Eigen::Matrix3d getRandomOrientation();

Eigen::Matrix3d getErrorOrientation(
    float errValue);

std::string monomerTypeToString(
    MonomerType monomerType
);

std::vector<std::pair<MonomerType,OrientationData>> getOrientationDataForParticle(
    OrientationDataMap orientationData,
    MonomerType particleMonomerType
);

std::vector<std::pair<std::size_t,OrientationData>> getNeighborOrientationData(
    std::size_t particleID,
    MonomerType particleMonomerType,
    std::vector<std::pair<MonomerType,OrientationData>> particleOrientationData,
    ParticleH5List trajectoryFrame,
    TopologyH5List topologyFrame,
    std::unordered_map<std::size_t, std::size_t> idmappingFrame
);

Eigen::Matrix3d getCurrentRotation(
    ParticleData neighborParticle0,
    ParticleData currentParticle,
    ParticleData neighborParticle1
);

Eigen::Matrix3d getInitialRotation(
    std::vector<std::pair<std::size_t,OrientationData>> neighborOrientationData
);

Eigen::Vector3d getRandomPerpendicularVector(
    Eigen::Vector3d vector
);

Eigen::Matrix3d getRotationUsingAxis(
    ParticleData particle,
    ParticleData neighborParticle,
    Eigen::Matrix3d axisRotation
);

void calculateOrientations(
    const TopologyH5Info& topologyH5Info,
    const TrajectoryH5Info& trajectoryH5Info,
    RotationH5Info& outRotations,
    IdParticleMapping& particleLookup,
    OrientationDataMap& orientationData
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
            TrajectoryFileProperties ignore;
            this->LoadTrajectoryFile("/tmp/test.h5", ignore);
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

    void ReaDDyPkg::LoadTrajectoryFile(
        std::string file_path,
        TrajectoryFileProperties& fileProps
    )
    {
        if (last_loaded_file != file_path) {
            this->m_hasLoadedRunFile = false;
        } else {
            std::cout << "Using loaded file:  " << file_path << std::endl;
            auto& time = std::get<0>(this->m_trajectoryInfo);
            auto& traj = std::get<1>(this->m_trajectoryInfo);

            fileProps.numberOfFrames = traj.size();
            fileProps.timeStepSize = time.size() >= 2 ? time[1] - time[0] : 0;

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

            auto& time = std::get<0>(this->m_trajectoryInfo);
            auto& traj = std::get<1>(this->m_trajectoryInfo);

            fileProps.numberOfFrames = traj.size();
            fileProps.timeStepSize = time.size() >= 2 ? time[1] - time[0] : 0;
        }
    }

    double ReaDDyPkg::GetTime(std::size_t frameNumber)
    {
        return std::get<0>(this->m_trajectoryInfo).at(frameNumber);
    }

    std::size_t ReaDDyPkg::GetFrameNumber(double timeNs)
    {
        auto times = std::get<0>(this->m_trajectoryInfo);
        auto lower = std::lower_bound(times.begin(), times.end(), timeNs);
        auto frame = std::distance(times.begin(), lower);

        if(frame > 0) { frame = frame - 1; }
        return frame;
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
    auto orientationData = initOrientationData();

    calculateOrientations(
        std::get<1>(topologyInfo),
        std::get<1>(trajectoryInfo),
        rotationInfo,
        particleLookup,
        orientationData
    );

    std::cout << "Found trajectory for " << std::get<0>(trajectoryInfo).size() << " frames" << std::endl;
    std::cout << "Found topology for " << std::get<0>(topologyInfo).size() << " frames" << std::endl;

    file->close();
}

std::vector<std::string> childTypes { "_CHILD", "site-" };

bool is_child_particle(
    std::string particleType)
{
    for (std::size_t i = 0; i < childTypes.size(); ++i)
    {
        if (particleType.find(childTypes.at(i)) != std::string::npos) {
            return true;
        }
    }
    return false;
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

        // if this particle is a child particle, don't stream its position
        //  to stream child positions, we would need to correlate the children
        //  the owning parent from the trajectory/H5 file info
        if (is_child_particle(p.type))
        {
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

MonomerType getMonomerType(
    std::string readdyParticleType
)
{
    // ReaDDy particle types contain the main type as well as state info
    // i.e. ReaDDy particle type = [main type]#[state flags (delimited by "_")]
    
    // get the main type
    char delimiter = '#';
    auto pos = readdyParticleType.find(delimiter);
    std::string mainType = readdyParticleType.substr(0, pos);
    std::string flagStr;
    if (pos != std::string::npos)
    {
        flagStr = readdyParticleType.substr(pos + 1, readdyParticleType.size() - pos);
    }
    else
    {
        return MonomerType (mainType, {}, 101);
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
    int number = 101;
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
    int actualMonomerNumber,
    MonomerType monomerType
)
{   
    auto n = actualMonomerNumber + std::get<2>(monomerType);
    
    if (n > 3)
        n -= 3;
    if (n < 1)
        n += 3;
        
    return n;
}

bool monomerTypeIsSatisfied(
    MonomerType referenceMonomerType,
    int referenceActualMonomerNumber,
    MonomerType testMonomerType,
    int testActualMonomerNumber
)
{
    // check that the main types are the same
    if (std::get<0>(testMonomerType) != std::get<0>(referenceMonomerType))
    {
        return false;
    }
    
    // check that the monomer number is correct
    if (std::get<2>(referenceMonomerType) <= 100)
    {
        if (getMonomerN(testActualMonomerNumber, testMonomerType) != 
            getMonomerN(referenceActualMonomerNumber, referenceMonomerType))
        {
            return false;
        }
    }
    
    // check that all the required flags are present
    auto referenceFlags = std::get<1>(referenceMonomerType);
    auto testFlags = std::get<1>(testMonomerType);
    for (std::size_t i = 0; i < referenceFlags.size(); ++i)
    {
        if (referenceFlags[i] == "any")
        {
            return true;
        }
        if (std::find(testFlags.begin(), testFlags.end(), referenceFlags[i]) == testFlags.end())
        {
            return false;
        }
    }
    return true;
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

Eigen::Matrix3d getErrorOrientation(
    float errValue)
{
    // Assign an error orientation that can be checked to know if the rotation algorithm failed
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

std::vector<std::pair<MonomerType,OrientationData>> getOrientationDataForParticle(
    OrientationDataMap orientationData,
    MonomerType particleMonomerType
)
{
    // get the relevant orientation data for this particle
    bool found = false;
    std::vector<std::pair<MonomerType,OrientationData>> particleOrientationData {};
    for (std::size_t i = 0; i < orientationData.size(); ++i)
    {  
        if (std::get<0>(particleMonomerType) == orientationData.at(i).first)
        {
            particleOrientationData = orientationData.at(i).second;
            found = true;
            break;
        }
    }
    if (!found && verboseOrientation)
    {
        std::cout << "Failed to find orientation data for " 
            << monomerTypeToString(particleMonomerType) << std::endl;
    }
    return particleOrientationData;
}

std::vector<std::pair<std::size_t,OrientationData>> getNeighborOrientationData(
    std::size_t particleID,
    MonomerType particleMonomerType,
    std::vector<std::pair<MonomerType,OrientationData>> particleOrientationData,
    ParticleH5List trajectoryFrame,
    TopologyH5List topologyFrame,
    std::unordered_map<std::size_t, std::size_t> idmappingFrame
)
{
    // match actual neighbors to orientation data
    std::vector<std::pair<std::size_t,OrientationData>> neighborOrientationData {};
    auto neighborIds = getNeighbors(particleID, topologyFrame);
    for (std::size_t i = 0; i < particleOrientationData.size(); ++i)
    {
        for (std::size_t j = 0; j < neighborIds.size(); ++j)
        { 
            if (idmappingFrame.find(neighborIds.at(j)) == idmappingFrame.end())
            {
                if (verboseOrientation)
                {
                    std::cout << "neighbor ID not found in id mapping! " << neighborIds.at(j) << std::endl;
                }
                continue;
            }
            auto neighborID = idmappingFrame.at(neighborIds.at(j));
            auto neighborParticle = trajectoryFrame.at(neighborID);
            auto neighborMonomerType = getMonomerType(neighborParticle.type);

            if (monomerTypeIsSatisfied(
                    particleOrientationData.at(i).first,
                    std::get<2>(particleMonomerType), 
                    neighborMonomerType, 0))
            {
                neighborOrientationData.push_back({
                    neighborID, 
                    particleOrientationData.at(i).second
                });
            }
        }
    }
    return neighborOrientationData;
}

Eigen::Matrix3d getCurrentRotation(
    ParticleData neighborParticle0,
    ParticleData currentParticle,
    ParticleData neighborParticle1
)
{
    std::vector<Eigen::Vector3d> basisPositions {};
    auto rpos0 = neighborParticle0.position;
    basisPositions.push_back(Eigen::Vector3d(rpos0[0], rpos0[1], rpos0[2]));
    auto rpos1 = currentParticle.position;
    basisPositions.push_back(Eigen::Vector3d(rpos1[0], rpos1[1], rpos1[2]));
    auto rpos2 = neighborParticle1.position;
    basisPositions.push_back(Eigen::Vector3d(rpos2[0], rpos2[1], rpos2[2]));

    return aics::agentsim::mathutil::GetRotationMatrix(basisPositions);
}

Eigen::Matrix3d getInitialRotation(
    std::vector<std::pair<std::size_t,OrientationData>> neighborOrientationData
)
{
    std::vector<Eigen::Vector3d> basisPositions {};
    basisPositions.push_back(neighborOrientationData.at(0).second.localPosition);
    basisPositions.push_back(Eigen::Vector3d(0,0,0));
    basisPositions.push_back(neighborOrientationData.at(1).second.localPosition);

    return aics::agentsim::mathutil::GetRotationMatrix(basisPositions);
}

Eigen::Vector3d getRandomPerpendicularVector(
    Eigen::Vector3d vector
)
{
    if (vector[0] == 0 && vector[1] == 0)
    {
        if (vector[2] == 0)
        {
            if (verboseOrientation)
            {
                std::cout << "Failed to get perpendicular vector to zero vector! " << std::endl;
            }
            return Eigen::Vector3d(0, 0, 0);
        }
        return Eigen::Vector3d(0, 1, 0);
    }
    
    auto u = Eigen::Vector3d(-vector[1], vector[0], 0);
    u.normalize();
    auto r = Eigen::AngleAxisd(rand() % 2*M_PI, vector).toRotationMatrix();

    return r * u;
}

Eigen::Matrix3d getRotationUsingAxis(
    ParticleData particle,
    ParticleData neighborParticle,
    Eigen::Matrix3d axisRotation
)
{
    auto particlePosition = Eigen::Vector3d(
        particle.position[0], particle.position[1], particle.position[2]);
    auto neighborPosition = Eigen::Vector3d(
        neighborParticle.position[0], neighborParticle.position[1], neighborParticle.position[2]);
    
    Eigen::Vector3d axis = neighborPosition - particlePosition;
    auto normal = getRandomPerpendicularVector(axis);
    auto normalPos = particlePosition + normal;

    std::vector<Eigen::Vector3d> basisPositions {};
    basisPositions.push_back(neighborPosition);
    basisPositions.push_back(particlePosition);
    basisPositions.push_back(normalPos);

    Eigen::Matrix3d rotation = aics::agentsim::mathutil::GetRotationMatrix(basisPositions);
    return rotation * axisRotation;
}

void calculateOrientations(
    const TopologyH5Info& topologyH5Info,
    const TrajectoryH5Info& trajectoryH5Info,
    RotationH5Info& outRotations,
    IdParticleMapping& particleLookup,
    OrientationDataMap& orientationData
)
{
    auto numberOfFrames = topologyH5Info.size();
    outRotations.resize(numberOfFrames);

    for(std::size_t frameIndex = 0; frameIndex < numberOfFrames; ++frameIndex)
    {
        auto trajectoryFrame = trajectoryH5Info.at(frameIndex);
        auto topologyFrame = topologyH5Info.at(frameIndex);
        auto& rotationFrame = outRotations.at(frameIndex);
        auto& idmappingFrame = particleLookup.at(frameIndex);
        
        std::vector<Eigen::Matrix3d> orientationFrame {};
        std::vector<std::pair<std::size_t,std::pair<std::size_t,OrientationData>>> orientRelativeToNeighbor {};
        
        // match particles to orientation data and calculate orientation for particles with 0 or 2 neighbors
        for(std::size_t particleIndex = 0; particleIndex < trajectoryFrame.size(); ++particleIndex)
        {
            auto currentParticle = trajectoryFrame.at(particleIndex);
            if (is_child_particle(currentParticle.type))
            {
                orientationFrame.push_back(getErrorOrientation(0));
                continue;
            }
        
            auto particleMonomerType = getMonomerType(currentParticle.type);
            auto particleOrientationData = getOrientationDataForParticle(orientationData, particleMonomerType);
            auto neighborOrientationData = getNeighborOrientationData(
                currentParticle.id, particleMonomerType, particleOrientationData, 
                trajectoryFrame, topologyFrame, idmappingFrame);
            
            if (neighborOrientationData.size() < 1)
            {
                // no neighbors, use random rotation
                if (verboseOrientation)
                {
                    std::cout << frameIndex << ": Use random orientation for " 
                        << currentParticle.type << " " << currentParticle.id << std::endl;
                }
                orientationFrame.push_back(getRandomOrientation());
                continue;
            }
            
            if (neighborOrientationData.size() < 2)
            {
                // one neighbor, revisit after neighbor's orientation is calculated
                if (verboseOrientation)
                {
                    std::cout << frameIndex << ": Use one neighbor for orientation of " 
                        << currentParticle.type << " " << currentParticle.id << std::endl;
                }
                orientationFrame.push_back(getErrorOrientation(0));
                orientRelativeToNeighbor.push_back({particleIndex, neighborOrientationData.at(0)});
                continue;
            }
            
            // two+ neighbors, use vectors to neighbors to construct current rotation matrix
            // and subtract the initial rotation for the particle constructed from the ideal
            // relative positions of the neighbors
            Eigen::Matrix3d currentRotation = getCurrentRotation(
                trajectoryFrame.at(neighborOrientationData.at(0).first),
                currentParticle,
                trajectoryFrame.at(neighborOrientationData.at(1).first));
            Eigen::Matrix3d initialRotation = getInitialRotation(neighborOrientationData);
            orientationFrame.push_back(currentRotation.inverse() * initialRotation);
            if (verboseOrientation)
            {
                std::cout << frameIndex << ": Successfully oriented " 
                    << currentParticle.type << " " << currentParticle.id << std::endl;
            }
        }
        
        // go back and calculate orientation for particles with one neighbor
        // since their orientation is dependent on the neighbor's orientation
        for (std::size_t i = 0; i < orientRelativeToNeighbor.size(); ++i)
        {   
            auto particleID = orientRelativeToNeighbor.at(i).first;
            
            if (orientationFrame.at(particleID) != getErrorOrientation(0))
            {
                continue;
            }
            
            auto neighborID = orientRelativeToNeighbor.at(i).second.first;
            auto neighborOrientation = orientationFrame.at(neighborID);
            if (neighborOrientation == getErrorOrientation(0))
            {
                // neighbor hasn't been oriented, so use axis rotation instead
                orientationFrame.at(particleID) = getRotationUsingAxis(
                    trajectoryFrame.at(particleID),
                    trajectoryFrame.at(neighborID),
                    orientRelativeToNeighbor.at(i).second.second.axisRotation
                );
                if (verboseOrientation)
                {
                    std::cout << frameIndex << ": Successfully oriented " 
                        << particleID << " with axis rotation from " << neighborID << std::endl;
                }
                continue;
            }

            auto offsetRotation = orientRelativeToNeighbor.at(i).second.second.localRotation.inverse();

            orientationFrame.at(particleID) = neighborOrientation * offsetRotation;
            if (verboseOrientation)
            {
                std::cout << frameIndex << ": Successfully oriented " 
                    << particleID << " with one neighbor" << std::endl;
            }
        }
        
        // save all the orientations as euler angles
        for(std::size_t i = 0; i < orientationFrame.size(); ++i)
        {
            rotationFrame.push_back(orientationFrame[i].eulerAngles(0,1,2));
        }
    }
}