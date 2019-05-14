#ifndef AICS_SIMULATION_CACHE_H
#define AICS_SIMULATION_CACHE_H

#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "agentsim/agent_data.h"

namespace aics {
namespace agentsim {

typedef std::vector<AgentData> AgentDataFrame;
typedef std::vector<AgentDataFrame> FrameList;

class SimulationCache
{
public:
	SimulationCache();
	~SimulationCache();

	void AddFrame(AgentDataFrame data);

	void SetCurrentFrame(std::size_t index);
	AgentDataFrame GetCurrentFrame();

	bool CurrentIsLatestFrame();

	void IncrementCurrentFrame();

	AgentDataFrame GetLatestFrame();

	std::size_t GetNumFrame();

	void ClearCache();

private:
	std::size_t m_current = 0;
	std::size_t m_frameCounter = 0;
	std::string m_cacheFileName = "runtime_cache.bin";
	std::vector<AgentDataFrame> m_runtimeCache;
};

}
}

#endif // AICS_SIMULATION_CACHE_H
