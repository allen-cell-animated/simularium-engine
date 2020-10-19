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
#include <sys/stat.h>

inline bool FileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

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
        std::string filePath = this->GetLocalFilePath(identifier);
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

    bool SimulationCache::DownloadRuntimeCache(std::string identifier)
    {
        std::string awsFilePath = this->GetAwsFilePath(identifier);
        bool isSimulariumFile = false;
        bool filesFound = true;

        LOG_F(INFO, "Downloading runtime cache for file %s", awsFilePath.c_str());
        std::string ext = identifier.substr(identifier.find_last_of(".") + 1);
        if(ext == "simularium") {
          isSimulariumFile = true;
        }

        // Otherwise, look for the binary cache file
        std::string fpropsFilePath = this->GetAwsInfoFilePath(identifier);
        std::string fpropsDestination = this->GetLocalInfoFilePath(identifier);
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
        std::string destination = this->GetLocalFilePath(identifier);
        std::string cacheFilePath = awsFilePath + "_cache";
        if (!aics::agentsim::aws_util::Download(cacheFilePath, destination)) {
            LOG_F(WARNING, "Cache file for %s not found on AWS S3", identifier.c_str());
            filesFound = false;
        }

        return filesFound;
    }

    bool SimulationCache::FindSimulariumFile(std::string fileName) {
        std::string tmpkey = "tmp";
        std::string tmpFile = this->GetLocalFilePath(tmpkey);
        bool fileFound = false;

        // try replacing the file extension with .simularium (e.g. test.h5 -> test.simularium)
        //  then try appending .simularium (e.g. test.h5 -> test.h5.simularium)
        std::vector<std::string> pathsToTry = {
          fileName.substr(0, fileName.find_last_of(".")) + ".simularium",
          fileName + ".simularium"
        };

        for(auto path: pathsToTry) {
          if(!fileFound) {
            std::string awsPath = this->GetAwsFilePath(path);

            if(!aics::agentsim::aws_util::Download(awsPath, tmpFile)) {
              LOG_F(INFO, "Simularium file %s not found on AWS S3", awsPath.c_str());
            } else {
                LOG_F(INFO, "Simularium file %s found on AWS S3", awsPath.c_str());
                fileFound = true;
            }
          }
        }

        if(!fileFound) {
          LOG_F(ERROR, "Simularium file %s not found on AWS S3", fileName.c_str());
          return false;
        }

        // Parse the file to JSON
        std::ifstream is(tmpFile);
        Json::Value simJson;
        is >> simJson;
        is.close();

        // Create trajectory file properties
        LOG_F(INFO, "Processed simularium file will be assigned to identifier %s", fileName.c_str());
        Json::Value& trajectoryInfo = simJson["trajectoryInfo"];
        this->ParseFileProperties(trajectoryInfo, fileName);
        this->WriteFilePropertiesToDisk(fileName);

        // Convert the simularium file to a binary cache file
        fileio::SimulariumFileReader simulariumFileReader;
        std::ofstream& os = this->GetOfstream(fileName);

        Json::Value& spatialData = simJson["spatialData"];
        int nFrames = spatialData["bundleSize"].asInt();
        LOG_F(INFO, "%i frames found in simularium json file", nFrames);
        for(int i = 0; i < nFrames; i++) {
          AgentDataFrame adf;
          if(simulariumFileReader.DeserializeFrame(simJson, i, adf)) {
            this->m_binaryCacheWriter.SerializeFrame(os, i, adf);
          } else {
            LOG_F(ERROR, "Failed to deserialize frame from simularium JSON");
          }
        }

        std::remove(tmpFile.c_str());
        return true;
    }

    bool SimulationCache::FindFile(std::string fileName) {
        std::string rawPath = this->GetLocalRawTrajectoryFilePath(fileName);
        std::string awsPath = this->GetAwsFilePath(fileName);

        // Download the file from AWS if it is not present locally
        if (!FileExists(rawPath)) {
            LOG_F(INFO, "%s doesn't exist locally, checking S3...", fileName.c_str());
            if (!aics::agentsim::aws_util::Download(awsPath, rawPath)) {
                LOG_F(WARNING, "%s not found on AWS S3", fileName.c_str());
                return false;
            }
        }

        return true;
    }

    bool SimulationCache::FindFiles(std::vector<std::string> files) {
        for(std::string& file : files) {
            if(!FindFile(file)) return false;
        }

        return true;
    }

    void SimulationCache::MarkTmpFiles(
      std::string identifier,
      std::vector<std::string> files) {
      this->m_tmpFiles[identifier] = files;
    }

    void SimulationCache::DeleteTmpFiles(std::string identifier) {
      auto files = this->m_tmpFiles[identifier];
      for(auto file: files) {
        if(std::remove(file.c_str()) != 0)
          LOG_F(WARNING, "Error deleting file %s", file.c_str());
        else
          LOG_F(INFO, "File %s succesfully deleted", file.c_str());
      }
    }

    void SimulationCache::WriteFilePropertiesToDisk(std::string identifier) {
        std::string filePropsPath = this->GetLocalInfoFilePath(identifier);
        std::string filePropsDest = this->GetAwsInfoFilePath(identifier);
        std::ofstream propsFile;
        propsFile.open(filePropsPath);

        TrajectoryFileProperties tfp = this->GetFileProperties(identifier);
        Json::Value fprops;
        fprops["fileName"] = tfp.fileName;
        fprops["version"] = 1;
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

    bool SimulationCache::UploadRuntimeCache(std::string identifier)
    {
        std::string awsFilePath = this->GetAwsFilePath(identifier);
        this->WriteFilePropertiesToDisk(identifier);

        std::string destination = awsFilePath + "_cache";
        std::string source = this->GetLocalFilePath(identifier);
        LOG_F(INFO, "Uploading cache file for %s to S3", identifier.c_str());
        if(!aics::agentsim::aws_util::Upload(source, destination)) {
            return false;
        }

        std::string filePropsPath = this->GetLocalInfoFilePath(identifier);
        std::string filePropsDest = this->GetAwsInfoFilePath(identifier);

        LOG_F(INFO, "Uploading info file for %s to S3", identifier.c_str());
        if(!aics::agentsim::aws_util::Upload(filePropsPath, filePropsDest))
        {
            return false;
        }

        return true;
    }



    void SimulationCache::ParseFileProperties(std::string identifier)
    {
        std::string filePath = this->GetLocalInfoFilePath(identifier);
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
        std::string filePath = this->GetLocalInfoFilePath(identifier);
        std::ifstream is(filePath);
        Json::Value fprops;
        is >> fprops;

        std::vector<std::string> keys({
            "version",
            "size",
            "typeMapping",
            "fileName",
            "totalSteps",
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

    std::string SimulationCache::GetLocalRawTrajectoryFilePath(std::string identifier) {
      return this->kCacheFolder + identifier;
    }

    std::string SimulationCache::GetAwsFilePath(std::string identifier)
    {
      return this->kAwsPrefix + identifier;
    }

    std::string SimulationCache::GetAwsInfoFilePath(std::string identifier)
    {
      return this->kAwsPrefix + identifier + "_info";
    }

    std::string SimulationCache::GetLocalFilePath(std::string identifier)
    {
        return this->kCacheFolder + identifier + ".bin";
    }

    std::string SimulationCache::GetLocalInfoFilePath(std::string identifier)
    {
        return this->kCacheFolder + identifier + ".json";
    }

    std::ofstream& SimulationCache::GetOfstream(std::string& identifier) {
        if(!this->m_ofstreams.count(identifier)) {
            std::ofstream& newStream = this->m_ofstreams[identifier];
            newStream.open(this->GetLocalFilePath(identifier), this->m_ofstreamFlags);
            return newStream;
        } else if (!this->m_ofstreams[identifier]) {
            std::ofstream& badStream = this->m_ofstreams[identifier];
            badStream.close();
            badStream.open(this->GetLocalFilePath(identifier), this->m_ofstreamFlags);
            return badStream;
        } else {
            std::ofstream& currentStream = this->m_ofstreams[identifier];
            return currentStream;
        }
    }

    std::ifstream& SimulationCache::GetIfstream(std::string& identifier) {
        if(!this->m_ifstreams.count(identifier)) {
            std::ifstream& newStream = this->m_ifstreams[identifier];
            newStream.open(this->GetLocalFilePath(identifier), this->m_ifstreamFlags);
            return newStream;
        } else if(!this->m_ifstreams[identifier]) {
            std::ifstream& badStream = this->m_ifstreams[identifier];
            badStream.close();
            badStream.open(this->GetLocalFilePath(identifier), this->m_ifstreamFlags);
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
