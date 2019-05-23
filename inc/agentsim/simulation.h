#ifndef AICS_SIMULATION_H
#define AICS_SIMULATION_H

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "agentsim/model/model.h"
#include "agentsim/simulation_cache.h"
#include "agentsim/agent_data.h"

namespace aics {
namespace agentsim {

class Agent;
class SimPkg;

class Simulation
{
public:
	/**
	*
	*
	*	@param 	simPkgs		a list of SimPKGs that this simulation object will own
	*	@param	agents		a list of Agents that this simulation object will own
	*/
	Simulation(
		std::vector<std::shared_ptr<SimPkg>> simPkgs,
		std::vector<std::shared_ptr<Agent>> agents
	);
	~Simulation();

	/**
	*	RunTimeStep
	*
	*	@param timeStep		the time to advance simulation, in seconds
	*
	*	Advance the SimPKGs by the amount of time specified, in
	*	a 'live' manner. Generally will run a single time-step
	*/
	void RunTimeStep(float timeStep);

	/**
	*	GetData
	*
	*	Get a list of data corresponding to the agents owned by this simulation object
	*	Includes information regarding position, type, and other relevant visualization
	* information to be streamed to a front-end
	*/
	std::vector<AgentData> GetData();

	/**
	*	GetDataFrame
	*
	*	@param	frame_no	the frame-number/index of the agent data frame to load
	*
	*	Get a list of data corresponding to the agents owned by this simulation object
	*	Includes information regarding position, type, and other relevant visualization
	* information to be streamed to a front-end
	*/
	std::vector<AgentData> GetDataFrame(std::size_t frame_no);

	/**
	*	Reset
	*
	*	Reset the simulation to an 'un-run' state
	*/
	void Reset();

	/**
	*	SetModel
	*
	*	@param	simModel		a Model object containing information about the simulation to run
	*
	*
	*/
	void SetModel(Model simModel);

	/*
	*	UpdateParameter
	*
	*	@param	name		the name of the parameter to update
	*	@param	val			the value to update the parameter to
	*
	*	Updates a simulation relevant parameter
	*/
	void UpdateParameter(std::string name, float val);

	/**
	*	RunAndSaveFrames
	*
	*	Runs every owned SimPKG from start to completion
	* Intended to run and save a simulation in some way
	*	GetNextFrame() should be used to retrieve results frame by frame
	*/
	void RunAndSaveFrames(
		float timeStep,
		std::size_t n_time_steps
	);

	/**
	*	HasLoadedAllFrames
	*
	*	returns true if every SimPKG has finished retrieving/sid_live_simulationaving data
	* returns false if there is more to retrieve using GetNextFrame()
	*/
	bool HasLoadedAllFrames();

	/**
	*	LoadNextFrame
	*
	*	Loads the next simulation frames worth of information from a SimPKG
	*/
	void LoadNextFrame();

	/**
	*	PlayCacheFromFrame
	*
	*	@param	frame_number	The index used to tell the cache where to start
	*												playing a simulation from. 0 represents the
	*												beginning (start at the first indexed frame)
	*/
	void PlayCacheFromFrame(std::size_t frame_number);

	/**
	*	IncrementCacheFrame
	*
	*	Increment the current cache frame (e.g. move from frame 4 -> frame 5)
	*/
	void IncrementCacheFrame();

	/**
	*	CacheCurrentAgents
	*
	*	Saves an AgentFrame with the visualization data for the current
	*	state of agents owned by this simulation
	*/
	void CacheCurrentAgents();

	/**
	*	IsPlayingFromCache
	*
	*	Is this simulation object currently streaming from cached visualization information?
	*/
	bool IsPlayingFromCache() { return this->m_isPlayingFromCache; }

	/**
	*	LoadTrajectoryFile
	*
	*	@param	file_path		The location of the trajectory file to load
	*											Currently, there is no validation for file <-> simPKG correctness
	*
	*	Loads a trajectory file to play back. Behavior will resemble live & pre-run playback.
	*/
	void LoadTrajectoryFile(std::string file_path);

	void SetPlaybackMode(std::size_t playback_mode);

	std::size_t GetNumFrames() { return m_cache.GetNumFrames(); }

private:
	std::vector<std::shared_ptr<Agent>> m_agents;
	std::vector<std::shared_ptr<SimPkg>> m_SimPkgs;
	Model m_model;
	SimulationCache m_cache;
	bool m_isPlayingFromCache = false;
	std::size_t m_playbackMode = 0; // live simulation
};

}
}

#endif // AICS_SIMULATION_H
