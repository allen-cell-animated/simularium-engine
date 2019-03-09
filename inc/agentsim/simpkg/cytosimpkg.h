#ifndef AICS_CYTOSIMPKG_H
#define AICS_CYTOSIMPKG_H

#include "agentsim/simpkg/simpkg.h"

#include <vector>
#include <string>

namespace aics {
namespace agentsim {

class CytosimPkg : public SimPkg
{
public:
	CytosimPkg() {}
	virtual ~CytosimPkg() { }

	virtual void Setup() override;
	virtual void Shutdown() override;
	virtual void InitAgents(std::vector<std::shared_ptr<Agent>>& agents, Model& model) override;
	virtual void InitReactions(Model& model) override;

	virtual void RunTimeStep(
		float timeStep, std::vector<std::shared_ptr<Agent>>& agents) override;

	virtual void UpdateParameter(
		std::string param_name, float param_value
	) override;

	virtual void Run() override;
	virtual void GetNextFrame(
		std::vector<std::shared_ptr<Agent>>& agents) override;

	virtual bool IsFinished() override;
	virtual bool IsRunningLive() override { return false; }

private:
	bool m_hasAlreadyRun = false;
	bool m_hasFinishedStreaming = false;
	bool m_hasLoadedFrameReader = false;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_CYTOSIMPKG_H
