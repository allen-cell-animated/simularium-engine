#ifndef AICS_SIMPKG_H
#define AICS_SIMPKG_H

#include "agentsim/model/model.h"
#include <memory>
#include <string>
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
    class SimPkg {
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
            float timeStep, std::vector<std::shared_ptr<Agent>>& agents)
            = 0;

        /**
	* UpdateParameter
	*
	*	@param	name		the name of the parameter to update
	*	@param	val			the value to update the parameter to
	*/
        virtual void UpdateParameter(
            std::string param_name, float param_value)
            = 0;

        /**
	*	Run
	*
	* @param timeStep		the time to advance simulation each frame
	* @param nTimeSteps	the number of frames to calculate
	*
	* Executes a simulation in its entirety, and saves the results to disk
	*/
        virtual void Run(float timeStep, std::size_t nTimeStep) = 0;

        /**
	* GetNextFrame
	*
	*	Updates agent positions using the next frame of a trajectory file
	*/
        virtual void GetNextFrame(std::vector<std::shared_ptr<Agent>>& agents) = 0;

        /**
	*	IsFinished
	*
	*	Has the simulation this SimPKG is responsible for finished running?
	*	For a 'live' simulation or one without an end, this will likely
	*	return false in every case
	*/
        virtual bool IsFinished() = 0;

        virtual void LoadTrajectoryFile(std::string file_path) = 0;
    };

} // namespace agentsim
} // namespace aics

#endif // AICS_SIMPKG_H
