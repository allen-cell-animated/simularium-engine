#include <algorithm>
#include <iostream>
#include <fstream>
#include <memory>
#include <string>
#include <vector>
#include <cstdio>
#include <csignal>
#include "agentsim/simulation_cache.h"

/**
*	File Serialization Functions
*/
bool goto_frameno(std::ifstream& is, std::size_t fno, std::string& line);

void serialize(
	std::ofstream& os,
	std::size_t frame_number,
	aics::agentsim::AgentDataFrame& adf);
bool deserialize(
	std::ifstream& is,
	std::size_t frame_number,
	aics::agentsim::AgentDataFrame& adf);

void serialize(std::ofstream& os, aics::agentsim::AgentData& ad);
bool deserialize(std::ifstream& is, aics::agentsim::AgentData& ad);

std::ofstream tmp_cache_file;
std::string delimiter = "/";


/**
*	Simulation API
*/
namespace aics {
namespace agentsim {

SimulationCache::SimulationCache()
{
	std::remove(this->m_cacheFileName.c_str());
	tmp_cache_file.open(this->m_cacheFileName, std::ios::out | std::ios::app | std::ios::binary);
}

SimulationCache::~SimulationCache()
{
	tmp_cache_file.close();
	std::remove(this->m_cacheFileName.c_str());
}

void SimulationCache::AddFrame(AgentDataFrame data)
{
	if(this->m_runtimeCache.size() == 0)
	{
		this->m_runtimeCache.push_back(data);
	}
	else
	{
		this->m_runtimeCache[0] = data;
	}

	serialize(tmp_cache_file, this->m_frameCounter, data);
	this->m_frameCounter++;
}

void SimulationCache::SetCurrentFrame(std::size_t index)
{
	this->m_current = std::min(index, this->m_frameCounter);
}

AgentDataFrame SimulationCache::GetFrame(std::size_t frame_no)
{
	if(frame_no > this->m_frameCounter || this->m_frameCounter == 0)
	{
		return AgentDataFrame();
	}

	std::ifstream is(this->m_cacheFileName, std::ios::in | std::ios::binary);
	if(is)
	{
		AgentDataFrame adf;
		deserialize(is, frame_no, adf);
		is.close();
		return adf;
	}

	is.close();
	std::cout << "Failed to load frame" << frame_no << " from cache" << std::endl;
	return AgentDataFrame();
}

AgentDataFrame SimulationCache::GetCurrentFrame()
{
	return this->GetFrame(this->m_current);
 }

bool SimulationCache::CurrentIsLatestFrame()
{
	return this->m_current >= this->m_frameCounter;
}

void SimulationCache::IncrementCurrentFrame()
{
	this->m_current++;
	this->m_current = std::min(this->m_current, this->m_frameCounter);
 }

AgentDataFrame SimulationCache::GetLatestFrame()
{
	if(this->m_runtimeCache.size() > 0)
	{
		return this->m_runtimeCache[0];
	}

	return this->GetFrame(this->m_frameCounter - 1);
}

std::size_t SimulationCache::GetNumFrames()
{
	return this->m_frameCounter;
}

void SimulationCache::ClearCache()
{
	tmp_cache_file.close();
	std::remove(this->m_cacheFileName.c_str());
	tmp_cache_file.open(this->m_cacheFileName, std::ios::out | std::ios::app | std::ios::binary);

	this->m_runtimeCache.clear();
	this->m_current = 0;
	this->m_frameCounter = 0;
}

} // namespace agentsim
} // namespace aics

/**
*	File Serialization Functions
*/
bool goto_frameno(
	std::ifstream& is,
	std::size_t fno,
	std::string& line)
{
	for(std::size_t i = 0; i < fno; ++i)
	{
		if(std::getline(is, line))
		{
			line = "";
		}
		else
		{
			return false;
		}
	}

	std::getline(is, line, delimiter[0]);
	return true;
}

void serialize(
	std::ofstream& os,
	std::size_t frame_number,
	aics::agentsim::AgentDataFrame& adf)
{
	os << "F" << frame_number << delimiter
	<< adf.size() << delimiter;
	for(aics::agentsim::AgentData ad : adf)
	{
		serialize(os, ad);
	}
	os << std::endl;
}

bool deserialize(
	std::ifstream& is,
	std::size_t frame_number,
	aics::agentsim::AgentDataFrame& adf)
{
	std::string line;
	if(!goto_frameno(is, frame_number, line))
	{
		return false;
	}

	// Get the number of agents in this data frame
	std::getline(is, line, delimiter[0]);
	std::size_t num_agents = std::stoi(line);

	for(std::size_t i = 0; i < num_agents; ++i)
	{
		aics::agentsim::AgentData ad;
		if(deserialize(is, ad))
		{
			adf.push_back(ad);
		}
	}

	return true;
}

void serialize(std::ofstream& os, aics::agentsim::AgentData& ad)
{
	os << ad.x << delimiter
	<< ad.y << delimiter
	<< ad.z << delimiter
	<< ad.xrot << delimiter
	<< ad.yrot << delimiter
	<< ad.zrot << delimiter
	<< ad.collision_radius << delimiter
	<< ad.vis_type << delimiter
	<< ad.type << delimiter
	<< ad.subpoints.size() << delimiter;

	for(auto val : ad.subpoints)
	{
		os << val << delimiter;
	}
}

bool deserialize(std::ifstream& is, aics::agentsim::AgentData& ad)
{
	std::string line;
	std::vector<float> vals;
	for(std::size_t i = 0; i < 10; ++i)
	{
		if(std::getline(is, line, delimiter[0]))
		{
			vals.push_back(std::atof(line.c_str()));
		}
		else
		{
			return false;
		}
	}

	ad.x = vals[0];
	ad.y = vals[1];
	ad.z = vals[2];
	ad.xrot = vals[3];
	ad.yrot = vals[4];
	ad.zrot = vals[5];
	ad.collision_radius = vals[6];
	ad.vis_type = vals[7];
	ad.type = vals[8];

	std::size_t num_sp = vals[9];
	for(std::size_t i = 0; i < num_sp; ++i)
	{
		if(std::getline(is, line, delimiter[0]))
		{
			ad.subpoints.push_back(std::atof(line.c_str()));
		}
		else
		{
			return false;
		}
	}

	return true;
}
