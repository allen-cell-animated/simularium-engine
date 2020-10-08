#include "agentsim/simulation_cache.h"
#include "agentsim/aws/aws_util.h"
#include "agentsim/fileio/simularium_file_reader.h"
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
        this->m_binaryCacheWriter.SerializeFrame(
          fstream,
          this->m_numFrames[identifier],
          data
        );
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
            this->m_binaryCacheReader.DeserializeFrame(
              is,
              frameNumber,
              adf
            );
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
        bool isSimulariumFile = false;
        bool filesFound = true;

        LOG_F(INFO, "Downloading runtime cache for file %s", awsFilePath.c_str());
        std::string ext = identifier.substr(identifier.find_last_of(".") + 1);
        if(ext == "simularium") {
          isSimulariumFile = true;
        }

        // Otherwise, look for the binary cache file
        std::string fpropsFilePath = awsFilePath + "_info";
        std::string fpropsDestination = this->GetInfoFilePath(identifier);
        if(!aics::agentsim::aws_util::Download(fpropsFilePath, fpropsDestination))
        {
            LOG_F(WARNING, "Info file for %s not found on AWS S3", awsFilePath.c_str());
            filesFound = false;
        }
        else if(!this->IsFilePropertiesValid(identifier))
        {
            LOG_F(WARNING, "Info file for %s is missing required fields", awsFilePath.c_str());
            filesFound = false;
        }

        LOG_F(INFO, "Downloading cache for %s from S3", awsFilePath.c_str());
        std::string destination = this->GetFilePath(identifier);
        std::string cacheFilePath = awsFilePath + "_cache";
        if (!aics::agentsim::aws_util::Download(cacheFilePath, destination)) {
            LOG_F(WARNING, "Cache file for %s not found on AWS S3", identifier.c_str());
            filesFound = false;
        }

        if(filesFound) {
          return true;
        } else if (isSimulariumFile) {
            LOG_F(INFO, "%s is an unprocessed simularium JSON file", identifier.c_str());
            std::string simulariumFilePath = this->m_cacheFolder + identifier;
            if(!aics::agentsim::aws_util::Download(awsFilePath, simulariumFilePath)) {
              LOG_F(WARNING, "Simularium file %s not found on AWS S3", awsFilePath.c_str());
              return false;
            }

            LOG_F(INFO, "Simularium file %s found on AWS S3", awsFilePath.c_str());

            // Parse the file to JSON
            std::ifstream is(simulariumFilePath);
            Json::Value simJson;
            is >> simJson;

            // Create trajectory file properties
            Json::Value& trajectoryInfo = simJson["trajectoryInfo"];
            this->ParseFileProperties(trajectoryInfo, identifier);
            this->WriteFilePropertiesToDisk(awsFilePath, identifier);

            // Convert the simularium file to a binary cache file
            fileio::SimulariumFileReader simulariumFileReader;
            std::ofstream& os = this->GetOfstream(identifier);

            Json::Value& spatialData = simJson["spatialData"];
            int nFrames = spatialData["bundleSize"].asInt();
            LOG_F(INFO, "%i frames found in simularium json file", nFrames);
            for(int i = 0; i < nFrames; i++) {
              AgentDataFrame adf;
              if(simulariumFileReader.DeserializeFrame(simJson, i, adf)) {
                this->m_binaryCacheWriter.SerializeFrame(os, i, adf);
              } else {
                //@TODO: Delete invalid file
              }
            }

            return true;
        } else {
          return false;
        }
    }

    void SimulationCache::WriteFilePropertiesToDisk(std::string awsFilePath, std::string identifier) {
        std::string filePropsPath = this->GetInfoFilePath(identifier);
        std::string filePropsDest = awsFilePath + "_info";
        std::ofstream propsFile;
        propsFile.open(filePropsPath);

        TrajectoryFileProperties tfp = this->GetFileProperties(identifier);
        Json::Value fprops;
        fprops["fileName"] = tfp.fileName;
        fprops["totalSteps"] = static_cast<int>(tfp.numberOfFrames);
        fprops["timeStepSize"] = tfp.timeStepSize;

        Json::Value size;
        size["x"] = static_cast<float>(tfp.boxX);
        size["y"] = static_cast<float>(tfp.boxY);
        size["z"] = static_cast<float>(tfp.boxZ);
        fprops["size"] = size;

        Json::Value typeMapping;
        for(auto& entry : tfp.typeMapping)
        {
            std::string id = std::to_string(entry.first);
            std::string name = entry.second;

            Json::Value newEntry;
            newEntry["name"] = name;

            typeMapping[id] = newEntry;
        }
        fprops["typeMapping"] = typeMapping;
        propsFile << fprops;
        propsFile.close();
    }

    bool SimulationCache::UploadRuntimeCache(std::string awsFilePath, std::string identifier)
    {
        this->WriteFilePropertiesToDisk(awsFilePath, identifier);

        std::string destination = awsFilePath + "_cache";
        std::string source = this->GetFilePath(identifier);
        LOG_F(INFO, "Uploading cache file for %s to S3", identifier.c_str());
        if(!aics::agentsim::aws_util::Upload(source, destination)) {
            return false;
        }

        std::string filePropsPath = this->GetInfoFilePath(identifier);
        std::string filePropsDest = awsFilePath + "_info";
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
        LOG_F(INFO, "Loading file %s from filepath %s", identifier.c_str(), filePath.c_str());

        std::ifstream is(filePath);
        Json::Value fprops;

        if(is.is_open()) {
          is >> fprops;
          this->ParseFileProperties(fprops, identifier);
        } else {
          LOG_F(ERROR, "Failed to open file %s", filePath.c_str());
        }
    }

    void SimulationCache::ParseFileProperties(Json::Value& fprops, std::string identifier)
    {
        TrajectoryFileProperties tfp;

        const Json::Value typeMapping = fprops["typeMapping"];
        std::vector<std::string> ids = typeMapping.getMemberNames();
        for(auto& id : ids)
        {
            std::size_t idKey = std::atoi(id.c_str());
            const Json::Value entry = typeMapping[id];
            tfp.typeMapping[idKey] = entry["name"].asString();
        }

        const Json::Value& size = fprops["size"];
        tfp.boxX = size["x"].asFloat();
        tfp.boxY = size["y"].asFloat();
        tfp.boxZ = size["z"].asFloat();

        tfp.fileName = fprops["fileName"].asString();
        tfp.numberOfFrames = fprops["totalSteps"].asInt();
        tfp.timeStepSize = fprops["timeStepSize"].asFloat();

        this->m_fileProps[identifier] = tfp;
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
