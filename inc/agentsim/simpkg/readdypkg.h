#ifndef AICS_READDYPKG_H
#define AICS_READDYPKG_H

#include "agentsim/simpkg/simpkg.h"
#include "readdy/readdy.h"

#include <vector>
#include <string>

namespace aics {
namespace agentsim {

class ReaDDyPkg : public SimPkg
{
public:
	ReaDDyPkg() : m_simulation("SingleCPU") {}
	virtual ~ReaDDyPkg() { }

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
		std::vector<std::shared_ptr<Agent>>& agents) override { };

	virtual bool IsFinished() override { return false; }
	virtual void Run() override { return; }
	virtual bool IsRunningLive() override { return true; }

private:
	readdy::Simulation m_simulation;
	bool m_agents_initialized = false;
	bool m_reactions_initialized = false;
	int m_timeStepCount = 0;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_READDYPKG_H
