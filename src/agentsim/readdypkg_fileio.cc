struct ParticleData;

void run_and_save_h5file(
	readdy::Simulation& sim,
	std::string file_name,
	float time_step,
	float n_time_steps,
	float write_stride
);

void read_h5file(
	std::string file_name,
	std::vector<std::vector<ParticleData>>& results
);

void copy_frame(
	readdy::Simulation& sim,
  std::vector<ParticleData>& particle_data,
  std::vector<std::shared_ptr<aics::agentsim::Agent>>& agents
);

namespace aics {
namespace agentsim {

std::vector<std::vector<ParticleData>> results;
std::size_t frame_no = 0;

void ReaDDyPkg::GetNextFrame(
	std::vector<std::shared_ptr<Agent>>& agents)
{
  if(this->m_hasFinishedStreaming)
		return;

	if(agents.size() == 0)
	{
		for(std::size_t i = 0; i < 5000; ++i)
		{
			std::shared_ptr<Agent> agent;
			agent.reset(new Agent());
			agent->SetVisibility(false);
			agent->SetCollisionRadius(1);
			agents.push_back(agent);
		}
	}

	if(!this->m_hasLoadedRunFile)
	{
		this->LoadTrajectoryFile("test.h5");
	}

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

void ReaDDyPkg::Run(float timeStep, std::size_t nTimeStep)
{
	if(this->m_hasAlreadyRun)
	 return;

	run_and_save_h5file(this->m_simulation, "test.h5", timeStep, nTimeStep, 10);
	this->m_hasAlreadyRun = true;
}

void ReaDDyPkg::LoadTrajectoryFile(std::string file_path)
{
	if(!this->m_hasLoadedRunFile)
	{
		std::cout << "Loading trajectory file " << file_path << std::endl;
		frame_no = 0;
		results.clear();
		read_h5file(file_path, results);
		this->m_hasLoadedRunFile = true;
	}
}

} // namespace agentsim
} // namespace aics

/**
*	File IO Functions
**/
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

void run_and_save_h5file(
	readdy::Simulation& sim,
	std::string file_name,
	float time_step,
	float n_time_steps,
	float write_stride
)
{
	auto file = readdy::File::create(file_name, h5rd::File::Flag::OVERWRITE);

	auto traj_ptr = sim.observe().flatTrajectory(1);
	traj_ptr->enableWriteToFile(*file.get(), "", write_stride);
	auto traj_handle = sim.registerObservable(std::move(traj_ptr));

	try{
		auto loop = sim.createLoop(time_step);
		loop.writeConfigToFile(*file);
		loop.run(n_time_steps);
	}
	catch (...) {
		std::raise(SIGABRT);
		std::cout << "Exception while running ReaDDy simulation" << std::endl;
	}

	file->close();
}

void read_h5file(
	std::string file_name,
	std::vector<std::vector<ParticleData>>& results
)
{
	// open the output file
	auto file = h5rd::File::open(file_name, h5rd::File::Flag::READ_ONLY);

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
	auto traj = file->getSubgroup("readdy/trajectory");

	// limits of length 2T containing [start_ix, end_ix] for each time step
	std::vector<std::size_t> limits;
	traj.read("limits", limits);

	// time of length T containing the tiem steps of recording the trajectory
	std::vector<readdy::time_step_type> time;
	traj.read("time", time);

	// records containing all particles for all times, slicing according to limits
	// yield particles for a particular time step
	std::vector<readdy::model::observables::TrajectoryEntry> entries;
	auto trajectoryEntryTypes = readdy::model::observables::util::getTrajectoryEntryTypes(file->ref());
	traj.read(
		"records", entries,
		&std::get<0>(trajectoryEntryTypes),
		&std::get<1>(trajectoryEntryTypes)
	);

	auto n_frames = limits.size() / 2;

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
}

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

			// check if this particle has a valid readdy location
      if(isnan(pos[0]) || isnan(pos[1]) || isnan(pos[2]))
      {
        continue;
      }

			// if this particle is a _CHILD particle, don't stream its position
			//  to stream child positions, we would need to correlate the children
			//  the owning parent from the trajectory/H5 file info
			if(p.type.find("_CHILD") != std::string::npos)
			{
				continue;
			}

			// check if we have counted past the avaliable agents provided
			//	this would mean we used every agent in the 'agents' already
      if(agent_index >= agents.size())
      {
        std::cout << "Not enough agents to represent Readdy file run: " << std::endl;
        readdy::log::info("\t type {}, id {}, pos ({}, {}, {})",
          p.type, p.id, p.position[0], p.position[1], p.position[2]);
        break;
      }

			// copy the position of the particle to an AgentViz agent
			auto currentAgent = agents[agent_index].get();
      currentAgent->SetLocation(Eigen::Vector3d(pos[0],pos[1],pos[2]));
			try { currentAgent->SetTypeID(particles.idOf(p.type)); }
			catch(...) { /* ignore */ }
      currentAgent->SetName(p.type);
      currentAgent->SetVisibility(true);
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
}
