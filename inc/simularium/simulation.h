#ifndef AICS_SIMULATION_H
#define AICS_SIMULATION_H

#include "simularium/agent_data.h"
#include "simularium/model/model.h"
#include "simularium/simulation_cache.h"
#include "simularium/network/trajectory_properties.h"
#include "simularium/network/net_message_ids.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

namespace aics {
namespace simularium {

    class Agent;
    class SimPkg;

    class Simulation {
    public:
        /**
	*   Constructor
	*
	*	@param 	simPkgs		a list of SimPKGs that this simulation object will own
	*	@param	agents		a list of Agents that this simulation object will own
	*/
        Simulation(
            std::vector<std::shared_ptr<SimPkg>> simPkgs,
            std::vector<std::shared_ptr<Agent>> agents);
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
	*   information to be streamed to a front-end
	*/
        std::vector<AgentData> GetData();

        /**
	*	GetDataFrame
	*
	*	@param	frame_no	the frame-number/index of the agent data frame to load
	*
  *   Returns a BufferUpdate object, containing data to be transmited
  *     and a new playback-position for the requesting streamer to save
	*/
        BroadcastUpdate GetBroadcastFrame(
            std::string identifier,
            std::size_t frame_no
        );

        /**
        *   GetBroadcastUpdate
        *
        *   @param    identifier        specifies the cache to be read
        *   @param    currentPosition   the position of the requesting trajectory
        *                                 playback streamer. Indicates a file
        *                                 position for the binary file being read/streamed
        *   @param    bufferSize        how many bits of data to include in this broadcast
        *                                 update
        *
        *   Returns a BufferUpdate object, containing data to be transmited
        *     and a new playback-position for the requesting streamer to save
        */
        BroadcastUpdate GetBroadcastUpdate(
          std::string identifier,
          std::size_t currentPosition,
          std::size_t bufferSize
        );

        std::size_t GetEndOfStreamPos(
          std::string identifier
        );

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
	*   Intended to run and save a simulation in some way
	*	GetNextFrame() should be used to retrieve results frame by frame
	*/
        void RunAndSaveFrames(
            float timeStep,
            std::size_t n_time_steps);

        /**
	*	HasLoadedAllFrames
	*
	*	returns true if every SimPKG has finished retrieving/saving data
	*   returns false if there is more to retrieve using GetNextFrame()
	*/
        bool HasLoadedAllFrames();

        /**
	*	LoadNextFrame
	*
	*	Loads the next simulation frames worth of information from a SimPKG
	*/
        void LoadNextFrame();

        /**
	*	CacheAgents
	*
	*	Saves an AgentFrame with the visualization data for the current
	*	state of agents owned by this simulation
	*/
        void CacheAgents(
          std::vector<std::shared_ptr<Agent>>& agents,
          std::size_t frameNumber,
          float time
        );

        /**
	*	LoadTrajectoryFile
	*
	*	@param	fileName		The name of the trajectory file to load
    *                           This function will modify to get the file path
	*							Currently, there is no validation for file <-> simPKG correctness
	*
	*	Loads a trajectory file to play back. Behavior will resemble live & pre-run playback.
	*/
        bool LoadTrajectoryFile(
            std::string fileName
        );

        /**
        *   SetPlaybackMode
        *
        *   @param  playbackMode    the mode the server is expected to operate in
        *                           this should be called every time the server switches 'function'
        *                           e.g. transitioning from running a live simulation to
        *                           streaming a trajectory file
        *
        *   Changes the mode the server is expected to operate in; this function
        *   may change internal properties of the server; e.g. creating agents
        *   for a specific mode to use, or resetting agents
        */
        void SetPlaybackMode(SimulationMode playbackMode);

        /**
        *   IsRunningLive
        *
        *   Is this simulation activley evaluating simulation trajectories
        *   while clients are streaming? Returns true if yes
        *   e.g. while streaming a pre-computed trajectory, this should return false
        */
        bool IsRunningLive()  {
            return this->m_playbackMode == SimulationMode::id_live_simulation;
        }

        /**
        *   UploadRuntimeCache
        *
        *   @param fileName   The name of the trajectory file to upload a cache
        *                     for to S3
        *
        *   Saves the runtime cache on S3
        *   The run-time cache is the version of a trajectory/simulation result
        *   that can be immediately streamed to a client with-out any processing
        */
        void UploadRuntimeCache(std::string fileName);

        /**
        *   DownloadRuntimeCache
        *
        *   @param  fileName    The name of the trajectory file to find a cache of (e.g. trajectory.h5)
        *                       this function will do any necessary modification to find the cache on S3
        *
        *   Returns true if an already processed runtime cache is found for a simulation
        */
        bool DownloadRuntimeCache(std::string fileName);

        /**
        *   PreprocessRuntimeCache
        *
        *   This function is for use after downloading an already processed run-time cache
        *   This function will evaluate the number of frames in the RunTime
        *   and implement any optimizations for finding individual frames
        */
        void PreprocessRuntimeCache(std::string identifier);

        bool FindSimulariumFile(std::string identifier)
          { return this->m_cache.FindSimulariumFile(identifier); }

        /**
        *   IsPlayingTrajectory
        *
        *   Is this simulation playing back a specified trajectory file?
        *   This is true for a specifically requested trajectory playback
        *   and false in 'live' mode or pre-run mode
        */
        bool IsPlayingTrajectory()  {
            return this->m_playbackMode == SimulationMode::id_traj_file_playback;
        }

        /**
        *   GetSimulationTimeAtFrame
        *
        *   @param  frameNumber     the frame to check simulation time at
        *
        *   This function returns the simulation time, in nano-seconds
        *   at a specified frame
        */
        double GetSimulationTimeAtFrame(
            std::string identifier,
            std::size_t frameNumber
        );

        /**
        *   GetClosestFrameNumberForTime
        *
        *   @param  simulationTimeNs    the simulation time in nano-seconds
        *
        *   This function returns the closest frame number for a simulation time
        *   specified in nano-seconds. e.g. if each time step is 500 nano seconds and
        *   time 505 ns is requested, the second frame number (index 1, time 500 ns) is returned
        */
        std::size_t GetClosestFrameNumberForTime(
            std::string identifier,
            double simulationTimeNs
        );

        bool HasFileInCache(std::string identifier) { return this->m_cache.HasIdentifier(identifier); }

        TrajectoryFileProperties GetFileProperties(std::string identifier)
            { return this->m_cache.GetFileProperties(identifier); }

        void SetFileProperties(std::string identifier, TrajectoryFileProperties tfp) {
            this->m_cache.SetFileProperties(identifier, tfp);
        }

        std::size_t GetNumFrames(std::string identifier)
            { return this->m_cache.GetNumFrames(identifier); }

        void SetSimId(std::string identifier) { this->m_simIdentifier = identifier; }
        std::string GetSimId() { return this->m_simIdentifier; }

        void CleanupTmpFiles(std::string identifier);
    private:
        std::vector<std::shared_ptr<Agent>> m_agents;
        std::vector<std::shared_ptr<SimPkg>> m_SimPkgs;
        Model m_model;
        SimulationCache m_cache;
        std::size_t m_playbackMode = SimulationMode::id_live_simulation;
        std::string m_simIdentifier = "runtime"; // identifier for currently running simulation

        std::size_t m_activeSimPkg = 0;
    };

}
}

#endif // AICS_SIMULATION_H
