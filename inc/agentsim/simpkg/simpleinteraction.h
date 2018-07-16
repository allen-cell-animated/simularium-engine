#ifndef AICS_SIMPLEINTERACTION_H
#define AICS_SIMPLEINTERACTION_H

#include "agentsim/simpkg/simpkg.h"
#include <vector>

namespace aics {
namespace agentsim {

class Agent;

class SimpleInteraction : public SimPkg
{
public:
	struct InteractionEvent
	{
			Agent* a1;
			Agent* a2;
	};

	virtual ~SimpleInteraction() {}
	virtual void Setup() override;
	virtual void Shutdown() override;

	/**
	* RunTimeStep
	*
	*	@param timeStep		the time to advance simulation
	* @param agents			a list of Agents to be moved
	*
	* description				this function will evaluate interaction
	*/
	virtual void RunTimeStep(
		float timeStep, std::vector<std::shared_ptr<Agent>>& agents) override;

	virtual void UpdateParameter(
		std::string param_name, float param_value
	) override { }

	void EvaluateInteractions(
		std::vector<std::shared_ptr<Agent>>& agents,
		std::vector<SimpleInteraction::InteractionEvent>& interactions);

private:
	std::vector<InteractionEvent> m_interactions;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_SIMPLEINTERACTION_H
