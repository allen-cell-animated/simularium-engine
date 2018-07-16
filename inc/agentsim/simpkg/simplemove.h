#ifndef AICS_SIMPLEMOVE_H
#define AICS_SIMPLEMOVE_H

#include "agentsim/simpkg/simpkg.h"
#include <random>
#include "Eigen/Dense"

namespace aics {
namespace agentsim {

class SimpleMove : public SimPkg
{
public:
	virtual ~SimpleMove() {}

	virtual void Setup() override;
	virtual void Shutdown() override;

	/**
	* RunTimeStep
	*
	*	@param timeStep		the time to advance simulation
	*	@param agents			a list of Agents to be moved
	*
	* description				this function will randomly place agents,
	*										avoiding spherical collision
	*/
	virtual void RunTimeStep(
		float timeStep, std::vector<std::shared_ptr<Agent>>& agents) override;

	virtual void UpdateParameter(
		std::string param_name, float param_value
	) override { }

	void SampleDiffusionStep(
		float timeStep, Agent* agent, Eigen::Vector3d& newLocation);

private:
	std::default_random_engine m_rng;
	std::exponential_distribution<double> m_exp_dist;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_SIMPKG_H
