struct ParticleData;

void copy_frame(
	readdy::Simulation& sim,
  std::vector<ParticleData>& particle_data,
  std::vector<std::shared_ptr<aics::agentsim::Agent>>& agents
);

struct ParticleData {
	ParticleData(std::string type, std::string flavor, const std::array<readdy::scalar, 3> &pos,
		readdy::model::Particle::id_type id, readdy::time_step_type t)
		: type(std::move(type)), flavor(std::move(flavor)), position(pos), id(id), t(t) {}

		std::string type;
		std::string flavor;
		std::array<readdy::scalar, 3> position;
		readdy::model::Particle::id_type id;
		readdy::time_step_type t;
};

namespace aics {
namespace agentsim {

std::shared_ptr<h5rd::File> file;
std::vector<readdy::model::observables::TrajectoryEntry> entries;
std::vector<std::vector<ParticleData>> results;
std::size_t frame_no = 0;

void ReaDDyPkg::GetNextFrame(
	std::vector<std::shared_ptr<Agent>>& agents)
{
  if(this->m_hasFinishedStreaming)
		return;

	if(!this->m_hasLoadedRunFile)
	{
    // open the output file
		file = h5rd::File::open("readdy_cached_run.h5", h5rd::File::Flag::READ_ONLY);

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
    for (const auto &type : types) {
      typeMapping[type.type_id] = std::string(type.name);
    }

    // read back trajectory
    auto traj = file->getSubgroup("readdy/trajectory/mytraj");

    // limits of length 2T containing [start_ix, end_ix] for each time step
    std::vector<std::size_t> limits;
    traj.read("limits", limits);

    // time of length T containing the tiem steps of recording the trajectory
    std::vector<readdy::time_step_type> time;
    traj.read("time", time);

    // records containing all particles for all times, slicing according to limits
    // yield particles for a particular time step
    entries.clear();
    auto trajectoryEntryTypes = readdy::model::observables::util::getTrajectoryEntryTypes(file->ref());
    traj.read(
      "records", entries,
      &std::get<0>(trajectoryEntryTypes),
      &std::get<1>(trajectoryEntryTypes)
    );

    auto n_frames = limits.size() / 2;
    std::cout << "Loading " << n_frames << " from cached ReaDDy run" << std::endl;

    // mapping the read-back data to the ParticleData struct
    results.clear();
    results.reserve(n_frames);

    auto timeIt = time.begin();
    for(std::size_t frame = 0; frame < limits.size(); frame += 2, ++timeIt)
    {
      auto begin = limits[frame];
      auto end = limits[frame + 1];
      results.emplace_back();

      auto &currentFrame = results.back();
      currentFrame.reserve(end - begin);

      for (auto it = entries.begin() + begin; it != entries.begin() + end; ++it)
      {
        currentFrame.emplace_back(
          typeMapping[it->typeId],
          readdy::model::particleflavor::particleFlavorToString(it->flavor),
          it->pos.data, it->id, *timeIt);
      }
    }

    file->close();
		this->m_hasLoadedRunFile = true;
	}

  /*
  // ouputting parts of what was stored in the file
  for(auto i=0U; i < results.size(); ++i)
  {
    const auto &particles = results[i];
    readdy::log::info("---- T={}, N={} ----", i, particles.size());
    for(const auto &p : particles)
    {
      readdy::log::info("\t type {}, id {}, pos ({}, {}, {})",
        p.type, p.id, p.position[0], p.position[1], p.position[2]);
    }
  }
  */

  if(frame_no >= results.size())
  {
    this->m_hasFinishedStreaming = true;
  }
  else
  {
      copy_frame(this->m_simulation, results[frame_no], agents);
      frame_no++;
  }
}

bool ReaDDyPkg::IsFinished()
{
	 return this->m_hasFinishedStreaming;
}

void ReaDDyPkg::Run()
{
	if(this->m_hasAlreadyRun)
	 return;

	auto file = readdy::File::create("readdy_cached_run.h5", h5rd::File::Flag::OVERWRITE);

	auto traj_ptr = this->m_simulation.observe().flatTrajectory(1);
	traj_ptr->enableWriteToFile(*file.get(), "mytraj", 1e1);
	auto handle = this->m_simulation.registerObservable(std::move(traj_ptr));

	try{
		auto loop = this->m_simulation.createLoop(1e-7);
		loop.writeConfigToFile(*file);
		loop.run(1e3);
	}
	catch (...) {
		std::raise(SIGABRT);
		std::cout << "Exception while running ReaDDy simulation" << std::endl;
	}

	this->m_hasAlreadyRun = true;
}

} // namespace agentsim
} // namespace aics

/**
*	File IO Functions
**/
void copy_frame(
	readdy::Simulation& sim,
  std::vector<ParticleData>& particle_data,
  std::vector<std::shared_ptr<aics::agentsim::Agent>>& agents
)
{
  std::size_t agent_index = 0;
  std::size_t ignore_count = 0;
  std::size_t bad_topology_count = 0;
  std::size_t good_topology_count = 0;

	auto &particles = sim.context().particleTypes();

  for(auto& p : particle_data)
  {
      auto pos =  p.position;
      if(isnan(pos[0]) || isnan(pos[1]) || isnan(pos[2]))
      {
				if(p.flavor == "TOPOLOGY")
        {
          bad_topology_count++;
        }

        ignore_count++;
        continue;
      }

			if(p.flavor == "TOPOLOGY")
      {
        good_topology_count++;
      }


			if(p.type.find("_CHILD") != std::string::npos)
			{
				ignore_count++;
				continue;
			}

      if(agent_index >= agents.size())
      {
        std::cout << "Not enough agents to represent Readdy file run: " << std::endl;
        readdy::log::info("\t type {}, id {}, pos ({}, {}, {})",
          p.type, p.id, p.position[0], p.position[1], p.position[2]);
        break;
      }

      agents[agent_index]->SetLocation(Eigen::Vector3d(pos[0],pos[1],pos[2]));
      agents[agent_index]->SetTypeID(particles.idOf(p.type));
      agents[agent_index]->SetName(p.type);
      agents[agent_index]->SetVisibility(true);
      agent_index++;
  }

  for(auto i = agent_index; i < agents.size(); ++i)
  {
    if(!agents[i]->IsVisible())
    {
      break;
    }

    agents[i]->SetVisibility(false);
  }

  std::cout
	<< "Frame Number " << aics::agentsim::frame_no << " "
  << "Agents retrieved from ReaDDy: " << agent_index
  << " Agents Ignored: " << ignore_count
  << " of " << particle_data.size()
	<< " Good|Bad Topology particles: "
	<< good_topology_count << " | " << bad_topology_count
	<< std::endl;
}
