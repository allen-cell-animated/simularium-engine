#ifndef AICS_SIMPKG_H
#define AICS_SIMPKG_H

#include <string>
#include <memory>
#include <vector>

namespace aics {
namespace agentsim {

class Agent;

/**
* SimPkg
*
* A Simulation Package (SimPkg) is any class/entity that may
* alter simulation agents, or evaluate thier behavior
*
* For instance, a third party library that evalutates Simulation
* will have it's results interpreted by a SimPkg subclass,
* to return a list of aics::agentsim::Agent entitites
*/
class SimPkg
{
public:
	virtual ~SimPkg() {}
	virtual void Setup() = 0;
	virtual void Shutdown() = 0;

	/**
	* RunTimeStep
	*
	*	@param timeStep		the time to advance simulation, in seconds
	* @param agents			a list of Agents to be manipulated
	*/
	virtual void RunTimeStep(
		float timeStep, std::vector<std::shared_ptr<Agent>>& agents) = 0;

	virtual void UpdateParameter(
		std::string param_name, float param_value
	) = 0;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_SIMPKG_H
