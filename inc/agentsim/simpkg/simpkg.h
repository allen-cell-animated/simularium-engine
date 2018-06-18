#ifndef AICS_SIMPKG_H
#define AICS_SIMPKG_H

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
	virtual void Setup() = 0;
	virtual void Shutdown() = 0;

	/**
	* RunTimeStep
	*
	*	@param timeStep		the time to advance simulation, in picoseconds
	* @param agents			a list of Agents to be moved
	*/
	virtual void RunTimeStep(float timeStep, std::vector<Agent*>& agents) = 0;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_SIMPKG_H
