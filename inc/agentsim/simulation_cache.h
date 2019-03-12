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
	void AddFrame(AgentDataFrame data);

	void SetCurrentFrame(std::size_t index);
	AgentDataFrame GetCurrentFrame();

	bool CurrentIsLatestFrame();

	void IncrementCurrentFrame();

	AgentDataFrame GetLatestFrame();

	std::size_t GetNumFrame();

	void SetCacheSize(std::size_t size);

	std::size_t GetCacheSize();

private:
	std::size_t m_cacheSize = 0;
	std::size_t m_current = 0;
	FrameList m_frames;
};

}
}

#endif // AICS_SIMULATION_CACHE_H
