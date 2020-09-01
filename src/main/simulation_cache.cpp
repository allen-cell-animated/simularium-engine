#include "agentsim/simulation_cache.h"
#include "agentsim/aws/aws_util.h"
#include "loguru/loguru.hpp"
#include <json/json.h>
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

        LOG_F(ERROR, "Failed to load frame %zu from cache %s", frameNumber, identifier.c_str());
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

        std::ifstream& is = this->GetIfstream(identifier);
        std::ofstream& os = this->GetOfstream(identifier);

        is.close();
        os.close();

        this->m_ofstreams.erase(identifier);
        this->m_ifstreams.erase(identifier);

        this->m_numFrames.erase(identifier);
        this->m_fileProps.erase(identifier);
    }

    void SimulationCache::Preprocess(std::string identifier)
    {
        this->ParseFileProperties(identifier);

        this->m_numFrames[identifier] = this->m_fileProps[identifier].numberOfFrames;
        LOG_F(INFO, "Number of frames in %s cache: %zu",
            identifier.c_str(), this->m_numFrames[identifier]
        );
    }

    bool SimulationCache::DownloadRuntimeCache(std::string awsFilePath, std::string identifier)
    {
        std::string fpropsFilePath = awsFilePath + "_info";
        std::string fpropsDestination = this->GetInfoFilePath(identifier);
        if(!aics::agentsim::aws_util::Download(fpropsFilePath, fpropsDestination))
        {
            LOG_F(WARNING, "Info file for %s not found on AWS S3", + awsFilePath.c_str());
            return false;
        }

        if(!this->IsFilePropertiesValid(identifier))
        {
            LOG_F(WARNING, "Info file for %s is missing required fields", + awsFilePath.c_str());
            return false;
        }

        LOG_F(INFO, "Downloading cache for %s from S3", awsFilePath.c_str());
        std::string destination = this->GetFilePath(identifier);
        std::string cacheFilePath = awsFilePath + "_cache";
        if (!aics::agentsim::aws_util::Download(cacheFilePath, destination)) {
            LOG_F(WARNING, "Cache file for %s not found on AWS S3", identifier.c_str());
            return false;
        }

        return true;
    }

    bool SimulationCache::UploadRuntimeCache(std::string awsFilePath, std::string identifier)
    {
        std::string destination = awsFilePath + "_cache";
        std::string source = this->GetFilePath(identifier);
        LOG_F(INFO, "Uploading cache file for %s to S3", identifier.c_str());
        if(!aics::agentsim::aws_util::Upload(source, destination)) {
            return false;
        }

        std::string filePropsPath = this->GetInfoFilePath(identifier);
        std::string filePropsDest = awsFilePath + "_info";
        std::ofstream propsFile;
        propsFile.open(filePropsPath);

        TrajectoryFileProperties tfp = this->GetFileProperties(identifier);
        Json::Value fprops;
        fprops["fileName"] = tfp.fileName;
        fprops["numberOfFrames"] = static_cast<int>(tfp.numberOfFrames);
        fprops["timeStepSize"] = tfp.timeStepSize;
        fprops["boxSizeX"] = static_cast<float>(tfp.boxX);
        fprops["boxSizeY"] = static_cast<float>(tfp.boxY);
        fprops["boxSizeZ"] = static_cast<float>(tfp.boxZ);

        Json::Value typeMapping;
        for(auto& entry : tfp.typeMapping)
        {
            std::string id = std::to_string(entry.first);
            std::string name = entry.second;

            typeMapping[id] = name;
        }
        fprops["typeMapping"] = typeMapping;
        propsFile << fprops;
        propsFile.close();

        LOG_F(INFO, "Uploading info file for %s to S3", identifier.c_str());
        if(!aics::agentsim::aws_util::Upload(filePropsPath, filePropsDest))
        {
            return false;
        }

        return true;
    }

    void SimulationCache::ParseFileProperties(std::string identifier)
    {
        std::string filePath = this->GetInfoFilePath(identifier);
        std::ifstream is(filePath);
        Json::Value fprops;
        is >> fprops;

        TrajectoryFileProperties trajFileProps;
        const Json::Value typeMapping = fprops["typeMapping"];
        std::vector<std::string> ids = typeMapping.getMemberNames();
        for(auto& id : ids)
        {
            std::size_t idKey = std::atoi(id.c_str());
            trajFileProps.typeMapping[idKey] =
                typeMapping[id].asString();
        }

        trajFileProps.boxX = fprops["boxSizeX"].asFloat();
        trajFileProps.boxY = fprops["boxSizeY"].asFloat();
        trajFileProps.boxZ = fprops["boxSizeZ"].asFloat();
        trajFileProps.fileName = fprops["fileName"].asString();
        trajFileProps.numberOfFrames = fprops["numberOfFrames"].asInt();
        trajFileProps.timeStepSize = fprops["timeStepSize"].asFloat();
        this->m_fileProps[identifier] = trajFileProps;
    }

    bool SimulationCache::IsFilePropertiesValid(std::string identifier)
    {
        std::string filePath = this->GetInfoFilePath(identifier);
        std::ifstream is(filePath);
        Json::Value fprops;
        is >> fprops;

        std::vector<std::string> keys({
            "boxSizeX",
            "boxSizeY",
            "boxSizeZ",
            "typeMapping",
            "fileName",
            "numberOfFrames",
            "timeStepSize"
        });

        for(auto key : keys) {
            if(!fprops.isMember(key)) {
                LOG_F(WARNING, "File properties for identifier %s is missing key %s", identifier.c_str(), key.c_str());
                return false;
            }
        }

        return true;
    }

    std::string SimulationCache::GetFilePath(std::string identifier)
    {
        return this->m_cacheFolder + identifier + ".bin";
    }

    std::string SimulationCache::GetInfoFilePath(std::string identifier)
    {
        return this->m_cacheFolder + identifier + ".json";
    }

    std::ofstream& SimulationCache::GetOfstream(std::string& identifier) {
        if(!this->m_ofstreams.count(identifier)) {
            std::ofstream& newStream = this->m_ofstreams[identifier];
            newStream.open(this->GetFilePath(identifier), this->m_ofstreamFlags);
            return newStream;
        } else if (!this->m_ofstreams[identifier]) {
            std::ofstream& badStream = this->m_ofstreams[identifier];
            badStream.close();
            badStream.open(this->GetFilePath(identifier), this->m_ofstreamFlags);
            return badStream;
        } else {
            std::ofstream& currentStream = this->m_ofstreams[identifier];
            return currentStream;
        }
    }

    std::ifstream& SimulationCache::GetIfstream(std::string& identifier) {
        if(!this->m_ifstreams.count(identifier)) {
            std::ifstream& newStream = this->m_ifstreams[identifier];
            newStream.open(this->GetFilePath(identifier), this->m_ifstreamFlags);
            return newStream;
        } else if(!this->m_ifstreams[identifier]) {
            std::ifstream& badStream = this->m_ifstreams[identifier];
            badStream.close();
            badStream.open(this->GetFilePath(identifier), this->m_ifstreamFlags);
            return badStream;
        } else {
            std::ifstream& currentStream = this->m_ifstreams[identifier];
            return currentStream;
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
    auto vals = aics::agentsim::Serialize(ad);
    for (auto val : vals) {
        os << val << delimiter;
    }
}

bool deserialize(std::ifstream& is, aics::agentsim::AgentData& ad)
{
    std::string line;
    std::vector<float> vals;
    for (std::size_t i = 0; i < 11; ++i) {
        if (std::getline(is, line, delimiter[0])) {
            vals.push_back(std::atof(line.c_str()));
        } else {
            return false;
        }
    }

    ad.vis_type = vals[0];
    ad.id = vals[1];
    ad.type = vals[2];
    ad.x = vals[3];
    ad.y = vals[4];
    ad.z = vals[5];
    ad.xrot = vals[6];
    ad.yrot = vals[7];
    ad.zrot = vals[8];
    ad.collision_radius = vals[9];

    std::size_t num_sp = vals[10];
    for (std::size_t i = 0; i < num_sp; ++i) {
        if (std::getline(is, line, delimiter[0])) {
            ad.subpoints.push_back(std::atof(line.c_str()));
        } else {
            return false;
        }
    }

    return true;
}
