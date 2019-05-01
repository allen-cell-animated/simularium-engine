#ifndef AICS_READDYPKG_H
#define AICS_READDYPKG_H

#include "agentsim/simpkg/simpkg.h"
#include "readdy/model/observables/io/TrajectoryEntry.h"
#include "readdy/model/observables/io/Types.h"
#include <fstream>
#include "readdy/readdy.h"

#include <vector>
#include <string>

namespace aics {
namespace agentsim {

class ReaDDyPkg : public SimPkg
{
public:
	ReaDDyPkg()
	{
		this->m_simulation = new readdy::Simulation("SingleCPU");
		this->m_bloscFilter.registerFilter();
	}
	virtual ~ReaDDyPkg() {
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
		std::string param_name, float param_value
	) override;

	// Cached Sim API
	virtual void GetNextFrame(
		std::vector<std::shared_ptr<Agent>>& agents) override;

	virtual bool IsFinished() override;
	virtual void Run(float timeStep, std::size_t nTimeStep) override;

	virtual void LoadTrajectoryFile(std::string file_path) override;
private:
	readdy::Simulation* m_simulation;
	bool m_agents_initialized = false;
	bool m_reactions_initialized = false;
	int m_timeStepCount = 0;

	bool m_hasAlreadyRun = false;
	bool m_hasLoadedRunFile = false;
	bool m_hasFinishedStreaming = false;
	readdy::io::BloscFilter m_bloscFilter;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_READDYPKG_H
