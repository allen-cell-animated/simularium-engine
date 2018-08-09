#include "agentsim/simulation.h"
#include "agentsim/simpkg/simpkg.h"
#include "agentsim/agents/agent.h"

namespace aics {
namespace agentsim {

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
}

std::vector<AgentData> Simulation::GetData()
{
	std::vector<AgentData> out;
	for(std::size_t i = 0; i < this->m_agents.size(); ++i)
	{
		auto agent = this->m_agents[i];
		AgentData ad;

		ad.type = agent->GetTypeID();
		auto location = agent->GetLocation();
		ad.x = location[0];
		ad.y = location[1];
		ad.z = location[2];

		auto rotation = agent->GetRotation();
		ad.xrot = rotation[0];
		ad.yrot = rotation[1];
		ad.zrot = rotation[2];

		out.push_back(ad);
	}

	return out;
}

void Simulation::Reset()
{
	for(std::size_t i = 0; i < this->m_SimPkgs.size(); ++i)
	{
			this->m_SimPkgs[i]->Shutdown();
			this->m_SimPkgs[i]->Setup();
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
}

} // namespace agentsim
} // namespace aics
