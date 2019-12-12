#include "agentsim/simulation.h"
#include "agentsim/agents/agent.h"
#include "agentsim/network/net_message_ids.h"
#include "agentsim/simpkg/simpkg.h"
#include "agentsim/aws/aws_util.h"
#include <cmath>
#include <iostream>
#include <stdlib.h>
#include <time.h>
#include <sys/stat.h>

inline bool FileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

bool FindFile(std::string& filePath)
{
    // Download the file from AWS if it is not present locally
    if (!FileExists(filePath)) {
        std::cout << filePath << " doesn't exist locally, checking S3..." << std::endl;
        if (!aics::agentsim::aws_util::Download(filePath, filePath)) {
            std::cout << filePath << " not found on AWS S3" << std::endl;
            return false;
        }
    }

    // Modifies the file-path  to a format that H5rd can reliably load
    // H5rd is a library written by the ReaDDy developers to load H5 files
    if (FileExists("/" + filePath)) {
        filePath = "/" + filePath;
        std::cout << "file name modified to " << filePath << std::endl;
    } else if (FileExists("./" + filePath)) {
        filePath = "./" + filePath;
        std::cout << "file name modified to " << filePath << std::endl;
    }

    return true;
}

namespace aics {
namespace agentsim {

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

        this->CacheCurrentAgents();
    }

    std::vector<AgentData> Simulation::GetDataFrame(
        std::string identifier,
        std::size_t frame_no
    )
    {
        return this->m_cache.GetFrame(identifier, frame_no);
    }

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

        this->m_cache.ClearCache(this->m_simIdentifier);
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
        for (std::size_t i = 0; i < this->m_SimPkgs.size(); ++i) {
            this->m_SimPkgs[i]->Run(timeStep, n_time_steps);
        }
    }

    bool Simulation::HasLoadedAllFrames()
    {
        for (std::size_t i = 0; i < this->m_SimPkgs.size(); ++i) {
            if (!this->m_SimPkgs[i]->IsFinished()) {
                return false;
            }
        }

        return true;
    }

    void Simulation::LoadNextFrame()
    {
        for (std::size_t i = 0; i < this->m_SimPkgs.size(); ++i) {
            if (!this->m_SimPkgs[i]->IsFinished()) {
                this->m_SimPkgs[i]->GetNextFrame(this->m_agents);
            }
        }

        this->CacheCurrentAgents();
    }

    void Simulation::CacheCurrentAgents()
    {
        AgentDataFrame newFrame;
        for (std::size_t i = 0; i < this->m_agents.size(); ++i) {
            auto agent = this->m_agents[i];
            AppendAgentData(newFrame, agent);
        }

        this->m_cache.AddFrame(this->m_simIdentifier, newFrame);
    }

    bool Simulation::LoadTrajectoryFile(std::string fileName)
    {
        std::string filePath = "trajectory/" + fileName;
        TrajectoryFileProperties tfp;
        for (std::size_t i = 0; i < this->m_SimPkgs.size(); ++i) {
            auto simPkg = this->m_SimPkgs[i];

            if(simPkg->CanLoadFile(fileName)) {
                if(!FindFile(filePath)) {
                    std::cout << "File not found" << std::endl;
                    return false;
                }

                simPkg->LoadTrajectoryFile(filePath, tfp);
                this->m_cache.SetFileProperties(fileName, tfp);
                this->m_simIdentifier = fileName;
                return true;
            }
        }

        std::cout << "No sim pkg found that can load " << fileName << std::endl;
        return false;
    }

    void Simulation::SetPlaybackMode(SimulationMode playbackMode)
    {
        this->m_playbackMode = playbackMode;
    }

    void Simulation::UploadRuntimeCache()
    {
        std::string awsFilePath = "trajectory/" + this->m_simIdentifier;
        this->m_cache.UploadRuntimeCache(awsFilePath, this->m_simIdentifier);
    }

    bool Simulation::DownloadRuntimeCache(std::string fileName)
    {
        std::string awsFilePath = "trajectory/" + fileName;
        return this->m_cache.DownloadRuntimeCache(awsFilePath, fileName);
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

            ad.type = agent->GetTypeID();
            ad.vis_type = static_cast<unsigned int>(agent->GetVisType());

            auto t = agent->GetGlobalTransform();
            ad.x = t(0, 3);
            ad.y = t(1, 3);
            ad.z = t(2, 3);

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

        for (std::size_t i = 0; i < agent->GetNumChildAgents(); ++i) {
            auto child = agent->GetChildAgent(i);
            AppendAgentData(out, child);
        }
    }

    void InitAgents(std::vector<std::shared_ptr<Agent>>& out, Model& model)
    {
        out.clear();

        int boxSize = pow(model.volume, 1.0 / 3.0);
        for (auto entry : model.agents) {
            std::string key = entry.first;
            std::size_t agent_count = 0;

            if (model.concentrations.count(key) != 0) {
                agent_count += model.concentrations[key] * model.volume;
            } else if (model.seed_counts.count(key) != 0) {
                agent_count += model.seed_counts[key];
            } else {
                continue;
            }

            auto agent_model_data = entry.second;
            for (std::size_t i = 0; i < agent_count; ++i) {
                std::shared_ptr<Agent> parent;
                parent.reset(new Agent());
                parent->SetName(agent_model_data.name);

                float x, y, z;
                x = rand() % boxSize - boxSize / 2;
                y = rand() % boxSize - boxSize / 2;
                z = rand() % boxSize - boxSize / 2;
                parent->SetLocation(Eigen::Vector3d(x, y, z));

                parent->SetDiffusionCoefficient(agent_model_data.diffusion_coefficient);
                parent->SetCollisionRadius(agent_model_data.collision_radius);
                parent->SetTypeID(agent_model_data.type_id);

                for (auto subagent : agent_model_data.children) {
                    std::shared_ptr<Agent> child;
                    child.reset(new Agent());
                    child->SetName(subagent.name);
                    child->SetLocation(
                        Eigen::Vector3d(
                            subagent.x_offset, subagent.y_offset, subagent.z_offset));

                    auto subagent_model_data = model.agents[subagent.name];
                    child->SetTypeID(subagent_model_data.type_id);
                    child->SetCollisionRadius(subagent_model_data.collision_radius);
                    child->SetDiffusionCoefficient(subagent_model_data.diffusion_coefficient);
                    child->SetVisibility(subagent.visible);

                    parent->AddChildAgent(child);
                }
                out.push_back(parent);
            }
        }
    }

    double Simulation::GetSimulationTimeAtFrame(
        std::string identifier, std::size_t frameNumber
    )
    {
        auto tfp = this->GetFileProperties(identifier);

        double time = 0.0;
        if(this->m_SimPkgs.size() > 0)
        {
            time = this->m_SimPkgs[0]->GetSimulationTimeAtFrame(frameNumber);
        }

        float nearlyZero = 1e-9;
        if(static_cast<double>(tfp.numberOfFrames * frameNumber) < nearlyZero
            && time < nearlyZero)
        {
            if(frameNumber != 0) // presumably, only the first frame may have a time of '0' ns
            {
                std::cout << "Both the cached time and the live-calculated time are zero, a " <<
                "dev error may have been made" << std::endl;
                return frameNumber; // this will allow client to function properly
                // a client may reasonably assume that frames are sequential
            }
        }

        return std::max( // one of the below is expected to be 0.0
            static_cast<double>(tfp.numberOfFrames * frameNumber), // non-zero if cache info was set
            time // non-zero if local processing or a live simulation happened
        ); // if both were zero, a dev error was made
    }

    std::size_t Simulation::GetClosestFrameNumberForTime(
        std::string identifier, double simulationTimeNs
    )
    {
        auto tfp = this->GetFileProperties(identifier);

        // If theres is cached meta-data for the simulation,
        //  assume we are running using a cache pulled down from the network
        if(tfp.numberOfFrames != 0)
        {
            // Integer division performed to get nearest frames
            // e.g. 8 ns / 3 ns = use frame 2 (time - 6 ns)
            return static_cast<int>(simulationTimeNs) / static_cast<int>(tfp.numberOfFrames);
        }

        if(this->m_SimPkgs.size() > 0)
        {
            return this->m_SimPkgs[0]->GetClosestFrameNumberForTime(simulationTimeNs);
        }

        return 0;
    }

} // namespace agentsim
} // namespace aics
