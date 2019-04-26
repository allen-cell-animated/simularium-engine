#include "agentsim/simulation.h"
#include "agentsim/simpkg/simpkg.h"
#include "agentsim/agents/agent.h"
#include <stdlib.h>
#include <time.h>
#include <cmath>
#include <iostream>

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
	for(std::size_t i = 0; i < simPkgs.size(); ++i)
	{
			this->m_SimPkgs.push_back(simPkgs[i]);
			this->m_SimPkgs[i]->Setup();
	}

	this->m_agents = agents;
	this->m_cache.SetCacheSize(100);
}

Simulation::~Simulation()
{
	for(std::size_t i = 0; i < this->m_SimPkgs.size(); ++i)
	{
			this->m_SimPkgs[i]->Shutdown();
	}
}

void Simulation::RunTimeStep(float timeStep)
{
	for(std::size_t i = 0; i < this->m_SimPkgs.size(); ++i)
	{
			this->m_SimPkgs[i]->RunTimeStep(timeStep, this->m_agents);
	}

	this->CacheCurrentAgents();
}

std::vector<AgentData> Simulation::GetData()
{
	if(this->IsPlayingFromCache())
	{
		return this->m_cache.GetCurrentFrame();
	}
	else
	{
		return this->m_cache.GetLatestFrame();
	}
}

void Simulation::Reset()
{
	for(std::size_t i = 0; i < this->m_SimPkgs.size(); ++i)
	{
			this->m_SimPkgs[i]->Shutdown();
			this->m_SimPkgs[i]->Setup();
	}

	this->m_agents.clear();
	InitAgents(this->m_agents, this->m_model);

	for(std::size_t i = 0; i < this->m_SimPkgs.size(); ++i)
	{
		this->m_SimPkgs[i]->InitAgents(this->m_agents, this->m_model);
		this->m_SimPkgs[i]->InitReactions(this->m_model);
	}
}

void Simulation::UpdateParameter(std::string name, float value)
{
	for(std::size_t i = 0; i < this->m_SimPkgs.size(); ++i)
	{
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
	for(std::size_t i = 0; i < this->m_SimPkgs.size(); ++i)
	{
		this->m_SimPkgs[i]->Run(timeStep, n_time_steps);
	}
}

bool Simulation::HasLoadedAllFrames()
{
	for(std::size_t i = 0; i < this->m_SimPkgs.size(); ++i)
	{
		if(!this->m_SimPkgs[i]->IsFinished())
		{
			return false;
		}
	}

	return true;
}

void Simulation::LoadNextFrame()
{
	for(std::size_t i = 0; i < this->m_SimPkgs.size(); ++i)
	{
		if(!this->m_SimPkgs[i]->IsFinished())
		{
			this->m_SimPkgs[i]->GetNextFrame(this->m_agents);
		}
	}

	this->CacheCurrentAgents();
}

void Simulation::PlayCacheFromFrame(std::size_t frame_number)
{
	this->m_cache.SetCurrentFrame(frame_number);
	this->m_isPlayingFromCache = true;
}

void Simulation::IncrementCacheFrame()
{
	this->m_cache.IncrementCurrentFrame();

	if(this->m_cache.CurrentIsLatestFrame())
	{
		this->m_isPlayingFromCache = false;
	}
}

void Simulation::CacheCurrentAgents()
{
	AgentDataFrame newFrame;
	for(std::size_t i = 0; i < this->m_agents.size(); ++i)
	{
		auto agent = this->m_agents[i];
		AppendAgentData(newFrame, agent);
	}

	this->m_cache.AddFrame(newFrame);
}

void Simulation::LoadTrajectoryFile(std::string file_path)
{
	this->Reset();

	for(std::size_t i = 0; i < this->m_SimPkgs.size(); ++i)
	{
		this->m_SimPkgs[i]->LoadTrajectoryFile(file_path);
	}
}

void AppendAgentData(
	std::vector<AgentData>& out,
	std::shared_ptr<Agent>& agent)
{

	if(agent->IsVisible())
	{
		AgentData ad;

		ad.type = agent->GetTypeID();
		ad.vis_type = static_cast<unsigned int>(agent->GetVisType());

		auto t = agent->GetGlobalTransform();
		ad.x = t(0,3);
		ad.y = t(1,3);
		ad.z = t(2,3);

		auto rotation = agent->GetRotation();
		ad.xrot = rotation[0];
		ad.yrot = rotation[1];
		ad.zrot = rotation[2];

		ad.collision_radius = agent->GetCollisionRadius();

		for(std::size_t i = 0; i < agent->GetNumSubPoints(); ++i)
		{
			auto sp = agent->GetSubPoint(i);
			ad.subpoints.push_back(sp[0]);
			ad.subpoints.push_back(sp[1]);
			ad.subpoints.push_back(sp[2]);
		}

		out.push_back(ad);
	}


	for(std::size_t i = 0; i < agent->GetNumChildAgents(); ++i)
	{
		auto child = agent->GetChildAgent(i);
		AppendAgentData(out, child);
	}
}

void InitAgents(std::vector<std::shared_ptr<Agent>>& out, Model& model)
{
	out.clear();

	int boxSize = pow(model.volume, 1.0/3.0);
	for(auto entry : model.agents)
	{
		std::string key = entry.first;
		std::size_t agent_count = 0;

		if(model.concentrations.count(key) != 0)
		{
			agent_count += model.concentrations[key] * model.volume;
		}
		else if (model.seed_counts.count(key) != 0)
		{
			agent_count += model.seed_counts[key];
		}
		else
		{
			continue;
		}

		auto agent_model_data = entry.second;
		for(std::size_t i = 0; i < agent_count; ++i)
		{
			std::shared_ptr<Agent> parent;
			parent.reset(new Agent());
			parent->SetName(agent_model_data.name);

			float x,y,z;
			x = rand() % boxSize - boxSize / 2;
			y = rand() % boxSize - boxSize / 2;
			z = rand() % boxSize - boxSize / 2;
			parent->SetLocation(Eigen::Vector3d(x,y,z));

			parent->SetDiffusionCoefficient(agent_model_data.diffusion_coefficient);
			parent->SetCollisionRadius(agent_model_data.collision_radius);
			parent->SetTypeID(agent_model_data.type_id);

			for(auto subagent : agent_model_data.children)
			{
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

} // namespace agentsim
} // namespace aics
