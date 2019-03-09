#ifndef AICS_SIMULATION_H
#define AICS_SIMULATION_H

#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "agentsim/model/model.h"

namespace aics {
namespace agentsim {

class Agent;
class SimPkg;

struct AgentData
{
	float type = 0;
	float x = 0, y = 0, z = 0;
	float xrot = 0, yrot = 0, zrot = 0;
	float collision_radius = 0;
	std::vector<float> subpoints;
	float vis_type = 0;
};

typedef std::vector<AgentData> AgentDataFrame;
typedef std::vector<AgentDataFrame> FrameList;

class SimulationCache
{
public:
	void AddFrame(AgentDataFrame data) {
		this->m_frames.push_back(data);

		if(this->m_frames.size() > this->m_cacheSize)
		{
			this->m_frames.erase(this->m_frames.begin());
		}
	}

	AgentDataFrame GetLatestFrame() {
		return this->m_frames[this->m_frames.size() - 1];
	}

	void SetCacheSize(std::size_t size) {
		this->m_cacheSize = size;
		this->m_frames.clear();
		this->m_frames.reserve(size);
	}

	std::size_t GetCacheSize() { return m_cacheSize; }

private:
	std::size_t m_cacheSize = 0;
	FrameList m_frames;
};

class Simulation
{
public:
	Simulation(
		std::vector<std::shared_ptr<SimPkg>> simPkgs,
		std::vector<std::shared_ptr<Agent>> agents
	);
	~Simulation();

	void RunTimeStep(float timeStep);
	std::vector<AgentData> GetData();

	void Reset();

	void SetModel(Model simModel);
	void UpdateParameter(std::string name, float val);

	void Run();
	bool IsFinished();
	void GetNextFrame();

	void CacheCurrentAgents();

private:
	std::vector<std::shared_ptr<Agent>> m_agents;
	std::vector<std::shared_ptr<SimPkg>> m_SimPkgs;
	Model m_model;
	SimulationCache m_cache;
};

}
}

#endif // AICS_SIMULATION_H
