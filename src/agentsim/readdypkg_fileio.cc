#include "agentsim/aws/aws_util.h"

inline bool file_exists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

bool get_file_path(std::string& name);

void run_and_save_h5file(
    readdy::Simulation* sim,
    std::string file_name,
    float time_step,
    float n_time_steps,
    float write_stride);

void read_h5file(
    std::string file_name,
    TrajectoryH5Info& trajectoryInfo,
    TimeTopologyH5Info& topologyInfo);

void copy_frame(
    readdy::Simulation* sim,
    const std::vector<ParticleData>& particle_data,
    std::vector<std::shared_ptr<aics::agentsim::Agent>>& agents);

TrajectoryH5Info readTrajectory(
    const std::shared_ptr<h5rd::File>& file,
    h5rd::Group& group);

TimeTopologyH5Info readTopologies(
    h5rd::Group& group,
    std::size_t from,
    std::size_t to,
    std::size_t stride);

namespace aics {
namespace agentsim {

    std::size_t frame_no = 0;
    std::string last_loaded_file = "";

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

        if (frame_no >= this->m_trajectoryInfo.size()) {
            this->m_hasFinishedStreaming = true;
        } else {
            copy_frame(this->m_simulation, this->m_trajectoryInfo[frame_no], agents);
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
            this->m_trajectoryInfo.clear();
            read_h5file(file_path, this->m_trajectoryInfo, this->m_topologyInfo);
            this->m_hasLoadedRunFile = true;
            last_loaded_file = file_path;
            std::cout << "Finished loading trajectory file: " << file_path << std::endl;
        }
    }

    TopologyH5List& ReaDDyPkg::GetFileTopologies(std::size_t frameNumber)
    {
        return std::get<1>(this->m_topologyInfo).at(frameNumber);
    }

    ParticleH5List& ReaDDyPkg::GetFileParticles(std::size_t frameNumber)
    {
        return this->m_trajectoryInfo.at(frameNumber);
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
    std::vector<std::vector<ParticleData>>& trajectoryInfo,
    TimeTopologyH5Info& topologyInfo)
{
    if (!get_file_path(file_name)) {
        return;
    }

    // open the output file
    auto file = h5rd::File::open(file_name, h5rd::File::Flag::READ_ONLY);

    // read back trajectory
    auto traj = file->getSubgroup("readdy/trajectory");
    trajectoryInfo = readTrajectory(file, traj);

    auto topGroup = file->getSubgroup("readdy/observables/topologies");
    topologyInfo = readTopologies(topGroup, 0, std::numeric_limits<std::size_t>::max(), 1);

    std::cout << "Found trajectory for " << trajectoryInfo.size() << " frames" << std::endl;
    std::cout << "Found topology for " << std::get<0>(topologyInfo).size() << " frames" << std::endl;

    file->close();
}

void copy_frame(
    readdy::Simulation* sim,
    const std::vector<ParticleData>& particle_data,
    std::vector<std::shared_ptr<aics::agentsim::Agent>>& agents)
{
    std::size_t agent_index = 0;
    std::size_t ignore_count = 0;
    std::size_t bad_topology_count = 0;
    std::size_t good_topology_count = 0;

    auto& particles = sim->context().particleTypes();

    for (auto& p : particle_data) {
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

TrajectoryH5Info readTrajectory(
    const std::shared_ptr<h5rd::File>& file,
    h5rd::Group& group)
{
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

    auto timeIt = time.begin();
    for (std::size_t frame = 0; frame < limits.size(); frame += 2, ++timeIt) {
        auto begin = limits[frame];
        auto end = limits[frame + 1];
        results.emplace_back();

        auto& currentFrame = results.back();
        currentFrame.reserve(end - begin);

        for (auto it = entries.begin() + begin; it != entries.begin() + end; ++it) {
            currentFrame.emplace_back(
                typeMapping[it->typeId],
                readdy::model::particleflavor::particleFlavorToString(it->flavor),
                it->pos.data, it->id, it->typeId, *timeIt);
        }
    }

    return results;
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
