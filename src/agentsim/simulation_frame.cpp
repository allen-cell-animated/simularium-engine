#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include "agentsim/simulation_frame.h"


namespace aics {
namespace agentsim {

void SimulationCache::AddFrame(AgentDataFrame data) {
	this->m_frames.push_back(data);
	if(this->m_frames.size() > this->m_cacheSize)
	{
		this->m_frames.erase(this->m_frames.begin());
	}
}

void SimulationCache::SetCurrentFrame(std::size_t index) { this->m_current = index; }
AgentDataFrame SimulationCache::GetCurrentFrame() {
	return this->m_frames[this->m_current];
 }

bool SimulationCache::CurrentIsLatestFrame() {
	return this->m_current >= this->m_frames.size();
}

void SimulationCache::IncrementCurrentFrame() {
	this->m_current++;
	this->m_current = std::min(this->m_current, this->m_cacheSize);
	this->m_current = std::min(this->m_current, this->m_frames.size());
 }

AgentDataFrame SimulationCache::GetLatestFrame() {
	return this->m_frames[this->m_frames.size() - 1];
}

std::size_t SimulationCache::GetNumFrame() { return m_frames.size(); }

void SimulationCache::SetCacheSize(std::size_t size) {
	this->m_cacheSize = size;
	this->m_frames.clear();
	this->m_frames.reserve(size);
}

std::size_t SimulationCache::GetCacheSize() { return m_cacheSize; }

} // namespace agentsim
} // namespace aics
