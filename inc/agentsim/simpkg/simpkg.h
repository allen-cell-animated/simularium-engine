#ifndef AICS_SIMPKG_H
#define AICS_SIMPKG_H

#include <string>
#include <memory>
#include <vector>
#include "agentsim/model/model.h"

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
	virtual void InitAgents(std::vector<std::shared_ptr<Agent>>& agents, Model& model) = 0;
	virtual void InitReactions(Model& model) = 0;

	/**
	* RunTimeStep
	*
	*	@param timeStep		the time to advance simulation, in seconds
	* @param agents			a list of Agents to be manipulated
	*/
	virtual void RunTimeStep(
		float timeStep, std::vector<std::shared_ptr<Agent>>& agents) = 0;

	/**
	* UpdateParameter
	*
	*	@param	name		the name of the parameter to update
	*	@param	val			the value to update the parameter to
	*/
	virtual void UpdateParameter(
		std::string param_name, float param_value
	) = 0;

	/**
	*	Run
	*
	* Executes a simulation in its entirety
	*/
	virtual void Run() = 0;

	virtual void GetNextFrame(std::vector<std::shared_ptr<Agent>>& agents) = 0;

	/**
	*	IsFinished
	*
	*	Has the simulation this SimPKG is responsible for finished running?
	*	For a 'live' simulation or one without an end, this will likely
	*	return false in every case
	*/
	virtual bool IsFinished() = 0;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_SIMPKG_H
