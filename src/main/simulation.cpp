#include "simularium/simulation.h"
#include "loguru/loguru.hpp"
#include "simularium/agents/agent.h"
#include "simularium/aws/aws_util.h"
#include "simularium/network/net_message_ids.h"
#include "simularium/simpkg/simpkg.h"
#include <cmath>
#include <iostream>
#include <stdlib.h>
#include <sys/stat.h>
#include <time.h>

namespace aics {
namespace simularium {

    void AppendAgentData(
        std::vector<AgentData>& out,
        std::shared_ptr<Agent>& agent);

    void InitAgents(
        std::vector<std::shared_ptr<Agent>>& out,
        Model& model);

    Simulation::Simulation(
        std::vector<std::shared_ptr<SimPkg>> simPkgs,
        std::vector<std::shared_ptr<Agent>> agents)
    {
        for (std::size_t i = 0; i < simPkgs.size(); ++i) {
            this->m_SimPkgs.push_back(simPkgs[i]);
            this->m_SimPkgs[i]->Setup();
        }

        this->m_agents = agents;
    }

    Simulation::~Simulation()
    {
        for (std::size_t i = 0; i < this->m_SimPkgs.size(); ++i) {
            this->m_SimPkgs[i]->Shutdown();
        }
    }

    void Simulation::RunTimeStep(float timeStep)
    {
        for (std::size_t i = 0; i < this->m_SimPkgs.size(); ++i) {
            this->m_SimPkgs[i]->RunTimeStep(timeStep, this->m_agents);
        }

        auto frameNumber = this->GetNumFrames(this->m_simIdentifier);
        float time = frameNumber;
        this->CacheAgents(this->m_agents, frameNumber, time);
    }

    BroadcastUpdate Simulation::GetBroadcastFrame(
        std::string identifier,
        std::size_t frame_no)
    {
        return this->m_cache.GetBroadcastFrame(identifier, frame_no);
    }

    BroadcastUpdate Simulation::GetBroadcastUpdate(
        std::string identifier,
        std::size_t currentPosition,
        std::size_t bufferSize)
    {
        return this->m_cache.GetBroadcastUpdate(
            identifier,
            currentPosition,
            bufferSize);
    }

    std::size_t Simulation::GetEndOfStreamPos(
        std::string identifier) { return this->m_cache.GetEndOfStreamPos(identifier); }

    std::size_t Simulation::GetFramePos(
        std::string identifier,
        std::size_t frameNumber) { return this->m_cache.GetFramePos(identifier, frameNumber); }

    void Simulation::Reset()
    {
        for (std::size_t i = 0; i < this->m_SimPkgs.size(); ++i) {
            this->m_SimPkgs[i]->Shutdown();
            this->m_SimPkgs[i]->Setup();
        }

        this->m_agents.clear();
        if (this->m_playbackMode == 0 || this->m_playbackMode == 1) {
            InitAgents(this->m_agents, this->m_model);

            for (std::size_t i = 0; i < this->m_SimPkgs.size(); ++i) {
                this->m_SimPkgs[i]->InitAgents(this->m_agents, this->m_model);
                this->m_SimPkgs[i]->InitReactions(this->m_model);
            }
        }

        // Allow trajectory file cache to persist for efficiency
        //  Assumption: live and pre-run have no use for outdated cache files
        if (this->m_playbackMode != SimulationMode::id_traj_file_playback) {
            this->m_cache.ClearCache(this->m_simIdentifier);
        }
    }

    void Simulation::UpdateParameter(std::string name, float value)
    {
        for (std::size_t i = 0; i < this->m_SimPkgs.size(); ++i) {
            this->m_SimPkgs[i]->UpdateParameter(name, value);
        }
    }

    void Simulation::SetModel(Model simModel)
    {
        this->m_model = simModel;
        this->Reset();
    }

    void Simulation::RunAndSaveFrames(
        float timeStep,
        std::size_t n_time_steps)
    {
        this->m_activeSimPkg = 0; // @TODO, selecting active sim-pkg for pre-run and live mode
        this->m_SimPkgs[this->m_activeSimPkg]->Run(timeStep, n_time_steps);
    }

    bool Simulation::HasLoadedAllFrames()
    {
        return this->m_SimPkgs[this->m_activeSimPkg]->IsFinished();
    }

    void Simulation::LoadNextFrame()
    {
        // This function is only called in the context of trajectory
        //  file loading
        //  Assumption: Only 1 SimPKG is needed in the use case
        auto simPkg = this->m_SimPkgs[this->m_activeSimPkg];
        if (!simPkg->IsFinished()) {
            simPkg->GetNextFrame(this->m_agents);

            auto frameNumber = this->GetNumFrames(this->m_simIdentifier);
            auto time = simPkg->GetSimulationTimeAtFrame(frameNumber);
            this->CacheAgents(this->m_agents, frameNumber, time);
        }
    }

    void Simulation::CacheAgents(
        std::vector<std::shared_ptr<Agent>>& agents,
        std::size_t frameNumber,
        float time)
    {
        TrajectoryFrame newFrame;
        for (std::size_t i = 0; i < this->m_agents.size(); ++i) {
            auto agent = this->m_agents[i];
            AppendAgentData(newFrame.data, agent);
        }

        newFrame.frameNumber = frameNumber;
        newFrame.time = time;

        this->m_cache.AddFrame(this->m_simIdentifier, newFrame);
    }

    bool Simulation::LoadTrajectoryFile(std::string fileName)
    {
        TrajectoryFileProperties tfp;
        tfp.fileName = fileName;
        for (std::size_t i = 0; i < this->m_SimPkgs.size(); ++i) {
            auto simPkg = this->m_SimPkgs[i];

            if (simPkg->CanLoadFile(fileName)) {
                this->m_activeSimPkg = i;

                std::vector<std::string> files = simPkg->GetFileNames(fileName);
                for (auto file : files) {
                    LOG_F(INFO, "File to load: %s", file.c_str());
                }

                if (!this->m_cache.FindFiles(files)) {
                    LOG_F(ERROR, "%s | File not found", fileName.c_str());
                    return false;
                }

                std::vector<std::string> rawFilePaths;
                for (auto file : files) {
                    rawFilePaths.push_back(
                        this->m_cache.GetLocalRawTrajectoryFilePath(file));
                }
                this->m_cache.MarkTmpFiles(fileName, rawFilePaths);

                std::string filePath = this->m_cache.GetLocalRawTrajectoryFilePath(fileName);
                simPkg->LoadTrajectoryFile(filePath, tfp);
                this->m_cache.SetFileProperties(fileName, tfp);
                this->m_simIdentifier = fileName;
                return true;
            }
        }

        LOG_F(WARNING, "No Sim PKG can load %s", fileName.c_str());
        return false;
    }

    void Simulation::CleanupTmpFiles(std::string identifier)
    {
        this->m_cache.DeleteTmpFiles(identifier);
    }

    void Simulation::SetPlaybackMode(SimulationMode playbackMode)
    {
        this->m_playbackMode = playbackMode;
        this->m_agents.clear();
    }

    void Simulation::UploadRuntimeCache(std::string fileName)
    {
        this->m_cache.UploadRuntimeCache(fileName);
    }

    bool Simulation::DownloadRuntimeCache(std::string fileName)
    {
        return this->m_cache.DownloadRuntimeCache(fileName);
    }

    void Simulation::PreprocessRuntimeCache(std::string identifier)
    {
        this->m_cache.Preprocess(identifier);
    }

    void AppendAgentData(
        std::vector<AgentData>& out,
        std::shared_ptr<Agent>& agent)
    {
        if (agent->IsVisible()) {
            AgentData ad;

            ad.id = agent->GetUid();
            ad.type = agent->GetTypeID();
            ad.vis_type = static_cast<unsigned int>(agent->GetVisType());

            auto location = agent->GetLocation();
            ad.x = location[0];
            ad.y = location[1];
            ad.z = location[2];

            auto rotation = agent->GetRotation();
            ad.xrot = rotation[0];
            ad.yrot = rotation[1];
            ad.zrot = rotation[2];

            ad.collision_radius = agent->GetCollisionRadius();

            for (std::size_t i = 0; i < agent->GetNumSubPoints(); ++i) {
                auto sp = agent->GetSubPoint(i);
                ad.subpoints.push_back(sp[0]);
                ad.subpoints.push_back(sp[1]);
                ad.subpoints.push_back(sp[2]);
            }

            out.push_back(ad);
        }
    }

    void InitAgents(std::vector<std::shared_ptr<Agent>>& out, Model& model)
    {
        //@TODO Implement when model definition better specified
        LOG_F(ERROR, "Model parsing is not currently implemented.");
    }

    double Simulation::GetSimulationTimeAtFrame(
        std::string identifier, std::size_t frameNumber)
    {
        if (this->m_playbackMode == SimulationMode::id_live_simulation) {
            // @TODO Handle variable frame-rate for 'live' mode
            //      is this needed functionality?
            return frameNumber;
        }

        if (frameNumber == 0) {
            return 0;
        } // Assumption: the first frame is at 0

        auto tfpJSON = this->GetFileProperties(identifier)->GetJSON();
        auto timeStepSize = tfpJSON["timeStepSize"].asFloat();
        double time = 0.0;
        time = static_cast<double>(timeStepSize * frameNumber);
        if (time > 0.0) {
            return time;
        }

        if (this->m_SimPkgs.size() > 0 && this->m_SimPkgs[this->m_activeSimPkg]->CanLoadFile(identifier)) {
            time = this->m_SimPkgs[this->m_activeSimPkg]->GetSimulationTimeAtFrame(frameNumber);
            if (time > 0.0) {
                return time;
            }
        }

        LOG_F(ERROR, "Both the cached time and the live-calculated time are zero, a dev error may have been made");
        return frameNumber;
    }

    std::size_t Simulation::GetClosestFrameNumberForTime(
        std::string identifier, double simulationTimeNs)
    {
        // Return the first frame for a negative time
        //  the assumption is that simulation time starts at a non-negative value
        if (simulationTimeNs < 0) {
            return 0;
        }

        if (this->m_playbackMode == SimulationMode::id_live_simulation) {
            // @TODO Handle variable frame-rate for 'live' mode
            //      is this needed functionality?
            std::size_t numFrames = this->m_cache.GetNumFrames(identifier);
            return std::min(
                static_cast<std::size_t>(simulationTimeNs),
                numFrames - 1);
        }

        auto tfpJSON = this->GetFileProperties(identifier)->GetJSON();
        auto timeStepSize = tfpJSON["timeStepSize"].asFloat();
        std::size_t totalNumberOfFrames = tfpJSON["totalSteps"].asInt();

        // If there is cached meta-data for the simulation,
        //  assume we are running using a cache pulled down from the network
        if (totalNumberOfFrames != 0) {
            // If the requested time is past the end,
            //  return the last frame avaliable
            auto totalDuration = totalNumberOfFrames * timeStepSize;
            float epsilon = 1e-15;
            if (simulationTimeNs >= totalDuration + epsilon) {
                return totalNumberOfFrames - 1;
            }

            // Return the nearest frame based on a fixed time-step size
            //  e.g. timestep = 2, requestedTime = 5.1,
            //   round(5.1/2) = round(2.55) = frame 3
            std::size_t frameNum = std::round(simulationTimeNs / timeStepSize);
            return std::min(
              frameNum,
              totalNumberOfFrames - 1
            );
        }

        if (this->m_SimPkgs.size() > 0 && this->m_SimPkgs[this->m_activeSimPkg]->CanLoadFile(identifier)) {
            return this->m_SimPkgs[this->m_activeSimPkg]->GetClosestFrameNumberForTime(simulationTimeNs);
        }

        return 0;
    }

} // namespace simularium
} // namespace aics
