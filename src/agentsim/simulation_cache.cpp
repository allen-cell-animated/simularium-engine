#include "agentsim/simulation_cache.h"
#include <algorithm>
#include <csignal>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <iterator>

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
        this->DeleteCacheFolder();
        this->CreateCacheFolder();
    }

    SimulationCache::~SimulationCache()
    {
        this->CloseFileStreams();
        this->DeleteCacheFolder();
    }

    void SimulationCache::AddFrame(std::string identifier, AgentDataFrame data)
    {
        if(this->m_numFrames.count(identifier) == 0) {
            this->m_numFrames[identifier] = 0;
        } else {
            this->m_numFrames[identifier]++;
        }

        auto& fstream = this->GetOfstream(identifier);
        serialize(fstream, this->m_numFrames[identifier], data);
    }

    AgentDataFrame SimulationCache::GetFrame(std::string identifier, std::size_t frameNumber)
    {
        if(this->m_numFrames.count(identifier) == 0) {
            return AgentDataFrame(); // not in cache at all
        }

        std::size_t numFrames = this->m_numFrames.at(identifier);
        if (frameNumber > numFrames || numFrames == 0) {
            return AgentDataFrame();
        }

        std::ifstream& is = this->GetIfstream(identifier);
        if (is) {
            AgentDataFrame adf;
            deserialize(is, frameNumber, adf);
            return adf;
        }

        std::cout << "Failed to load frame " << frameNumber << " from cache" << std::endl;
        return AgentDataFrame();
    }

    std::size_t SimulationCache::GetNumFrames(std::string identifier)
    {
        return this->m_numFrames.count(identifier) ?
            this->m_numFrames.at(identifier) : 0;
    }

    void SimulationCache::ClearCache(std::string identifier)
    {
        std::string filePath = this->GetFilePath(identifier);
        std::remove(filePath.c_str());
    }

    void SimulationCache::Preprocess(std::string identifier)
    {
        std::size_t numFrames;
        std::string filePath = this->GetFilePath(identifier);

        std::ifstream& is = this->GetIfstream(identifier);
        std::string line;
        while (std::getline(is, line))
        {
            numFrames++;
            line = "";
        }

        this->m_numFrames[identifier] = numFrames;
        std::cout << "Number of frames in " << identifier
            << " runtime cache: " << numFrames << std::endl;
    }

    std::string SimulationCache::GetFilePath(std::string identifier)
    {
        return this->m_cacheFolder + identifier + ".bin";
    }

    std::ofstream& SimulationCache::GetOfstream(std::string& identifier) {
        if(!this->m_ofstreams.count(identifier)) {
            std::ofstream& newStream = this->m_ofstreams[identifier];
            std::string filePath = this->GetFilePath(identifier);
            newStream.open(filePath, this->m_ofstreamFlags);
            return newStream;
        } else {
            std::ofstream& fstream = this->m_ofstreams[identifier];
            return fstream;
        }
    }

    std::ifstream& SimulationCache::GetIfstream(std::string& identifier) {
        if(!this->m_ifstreams.count(identifier)) {
            std::ifstream& newStream = this->m_ifstreams[identifier];
            std::string filePath = this->GetFilePath(identifier);
            newStream.open(filePath, this->m_ifstreamFlags);
            return newStream;
        } else {
            std::ifstream& fstream = this->m_ifstreams[identifier];
            return fstream;
        }
    }

    void SimulationCache::CloseFileStreams() {
        for (auto& entry : this->m_ofstreams)
        {
            entry.second.close();
        }

        for (auto& entry : this->m_ifstreams)
        {
            entry.second.close();
        }
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
    is.seekg(0, is.beg); // go to beginning
    for (std::size_t i = 0; i < fno; ++i) {
        if (std::getline(is, line)) {
            line = "";
        } else {
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
    for (aics::agentsim::AgentData ad : adf) {
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
    if (!goto_frameno(is, frame_number, line)) {
        return false;
    }

    // Get the number of agents in this data frame
    std::getline(is, line, delimiter[0]);
    std::size_t num_agents = std::stoi(line);

    for (std::size_t i = 0; i < num_agents; ++i) {
        aics::agentsim::AgentData ad;
        if (deserialize(is, ad)) {
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

    for (auto val : ad.subpoints) {
        os << val << delimiter;
    }
}

bool deserialize(std::ifstream& is, aics::agentsim::AgentData& ad)
{
    std::string line;
    std::vector<float> vals;
    for (std::size_t i = 0; i < 10; ++i) {
        if (std::getline(is, line, delimiter[0])) {
            vals.push_back(std::atof(line.c_str()));
        } else {
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
    for (std::size_t i = 0; i < num_sp; ++i) {
        if (std::getline(is, line, delimiter[0])) {
            ad.subpoints.push_back(std::atof(line.c_str()));
        } else {
            return false;
        }
    }

    return true;
}
