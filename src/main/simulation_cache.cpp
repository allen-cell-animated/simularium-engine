#include "simularium/simulation_cache.h"
#include "loguru/loguru.hpp"
#include "simularium/aws/aws_util.h"
#include "simularium/config/config.h"
#include "simularium/fileio/simularium_file_reader.h"
#include <algorithm>
#include <csignal>
#include <cstdio>
#include <fstream>
#include <iostream>
#include <iterator>
#include <json/json.h>
#include <memory>
#include <string>
#include <sys/stat.h>
#include <vector>

inline bool FileExists(const std::string& name)
{
    struct stat buffer;
    return (stat(name.c_str(), &buffer) == 0);
}

/**
*	Simulation API
*/
namespace aics {
namespace simularium {

    inline void CreateCacheFolder()
    {
        std::string cmd = "mkdir -p " + config::GetCacheFolder();
        int ignore = system(cmd.c_str());
    }
    inline void DeleteCacheFolder()
    {
        std::string cmd = "rm -rf " + config::GetCacheFolder();
        int ignore = system(cmd.c_str());
    }

    SimulationCache::SimulationCache()
    {
        DeleteCacheFolder();
        CreateCacheFolder();
    }

    SimulationCache::~SimulationCache()
    {
        DeleteCacheFolder();
    }

    void SimulationCache::AddFrame(std::string identifier, TrajectoryFrame frame)
    {
        fileio::SimulariumBinaryFile* file = this->GetBinaryFile(identifier);
        file->WriteFrame(frame);
    }

    BroadcastUpdate SimulationCache::GetBroadcastFrame(std::string identifier, std::size_t frameNumber)
    {
        if (!this->m_binaryFiles.count(identifier)) {
            LOG_F(ERROR, "Request for identifier %s, which is not in cache", identifier.c_str());
            return BroadcastUpdate();
        }

        std::size_t numFrames = this->GetNumFrames(identifier);
        if (frameNumber > numFrames || numFrames == 0) {
            LOG_F(ERROR, "Request for frame %zu of identifier %s, which is not in cache", frameNumber, identifier.c_str());
            return BroadcastUpdate();
        }

        return this->m_binaryFiles.at(identifier)->GetBroadcastFrame(frameNumber);
    }

    BroadcastUpdate SimulationCache::GetBroadcastUpdate(
        std::string identifier,
        std::size_t currentPosition,
        std::size_t bufferSize)
    {
        if (!this->m_binaryFiles.count(identifier)) {
            LOG_F(ERROR, "Request for identifier %s, which is not in cache", identifier.c_str());
            return BroadcastUpdate();
        }

        return this->m_binaryFiles.at(identifier)->GetBroadcastUpdate(currentPosition, bufferSize);
    }

    std::size_t SimulationCache::GetEndOfStreamPos(
        std::string identifier)
    {
        if (!this->m_binaryFiles.count(identifier)) {
            LOG_F(ERROR, "Request for identifier %s, which is not in cache", identifier.c_str());
            return 0;
        }

        return this->m_binaryFiles.at(identifier)->GetEndOfFilePos();
    }

    std::size_t SimulationCache::GetFramePos(
        std::string identifier,
        std::size_t frameNumber)
    {
        if (!this->m_binaryFiles.count(identifier)) {
            LOG_F(ERROR, "Request for identifier %s, which is not in cache", identifier.c_str());
            return 0;
        }

        return this->m_binaryFiles.at(identifier)->GetFramePos(frameNumber);
    }

    std::size_t SimulationCache::GetNumFrames(std::string identifier)
    {
        return this->m_binaryFiles.count(identifier) ? this->m_binaryFiles.at(identifier)->NumSavedFrames() : 0;
    }

    void SimulationCache::ClearCache(std::string identifier)
    {
        std::string filePath = this->GetLocalFilePath(identifier);
        std::remove(filePath.c_str());

        this->m_binaryFiles.erase(identifier);
        this->m_fileProps.erase(identifier);
    }

    void SimulationCache::Preprocess(std::string identifier)
    {
        this->ParseFileProperties(identifier);
    }

    bool SimulationCache::DownloadRuntimeCache(std::string identifier)
    {
        std::string awsFilePath = this->GetS3TrajectoryCachePath(identifier);
        bool isSimulariumFile = false;
        bool filesFound = true;

        LOG_F(INFO, "Downloading runtime cache for file %s", awsFilePath.c_str());
        std::string ext = identifier.substr(identifier.find_last_of(".") + 1);
        if (ext == "simularium") {
            isSimulariumFile = true;
        }

        // Otherwise, look for the binary cache file
        std::string fpropsFilePath = this->GetS3InfoCachePath(identifier);
        std::string fpropsDestination = this->GetLocalInfoFilePath(identifier);
        if (!aics::simularium::aws_util::Download(fpropsFilePath, fpropsDestination)) {
            LOG_F(WARNING, "Info file for %s not found on AWS S3", awsFilePath.c_str());
            filesFound = false;
        } else if (!this->IsFilePropertiesValid(identifier)) {
            LOG_F(WARNING, "Info file for %s is missing required fields", awsFilePath.c_str());
            filesFound = false;
        }

        LOG_F(INFO, "Downloading cache for %s from S3", awsFilePath.c_str());
        std::string destination = this->GetLocalFilePath(identifier);
        if (!aics::simularium::aws_util::Download(awsFilePath, destination)) {
            LOG_F(WARNING, "Cache file for %s not found on AWS S3", identifier.c_str());
            filesFound = false;
        }

        // @HACK: called to add the file to the 'list'
        if (filesFound) {
            auto ignore = this->GetBinaryFile(identifier);
        }

        return filesFound;
    }

    bool SimulationCache::FindSimulariumFile(std::string fileName)
    {
        std::string tmpkey = "tmp";
        std::string tmpFile = this->GetLocalFilePath(tmpkey);
        bool fileFound = false;

        // try replacing the file extension with .simularium (e.g. test.h5 -> test.simularium)
        //  then try appending .simularium (e.g. test.h5 -> test.h5.simularium)
        std::vector<std::string> pathsToTry = {
            fileName.substr(0, fileName.find_last_of(".")) + ".simularium",
            fileName + ".simularium"
        };

        for (auto path : pathsToTry) {
            if (!fileFound) {
                std::string awsPath = this->GetS3TrajectoryPath(path);

                if (!aics::simularium::aws_util::Download(awsPath, tmpFile)) {
                    LOG_F(INFO, "Simularium file %s not found on AWS S3", awsPath.c_str());
                } else {
                    LOG_F(INFO, "Simularium file %s found on AWS S3", awsPath.c_str());
                    fileFound = true;
                }
            }
        }

        if (!fileFound) {
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
        fileio::SimulariumBinaryFile* outFile = this->GetBinaryFile(fileName);

        Json::Value& spatialData = simJson["spatialData"];
        int nFrames = spatialData["bundleSize"].asInt();
        LOG_F(INFO, "%i frames found in simularium json file", nFrames);
        for (int i = 0; i < nFrames; i++) {
            TrajectoryFrame frame;
            if (simulariumFileReader.DeserializeFrame(simJson, i, frame)) {
                outFile->WriteFrame(frame);
            } else {
                LOG_F(ERROR, "Failed to deserialize frame from simularium JSON");
            }
        }

        std::remove(tmpFile.c_str());

        return true;
    }

    bool SimulationCache::FindFile(std::string fileName)
    {
        std::string rawPath = this->GetLocalRawTrajectoryFilePath(fileName);
        std::string awsPath = this->GetS3TrajectoryPath(fileName);

        // Download the file from AWS if it is not present locally
        if (!FileExists(rawPath)) {
            LOG_F(INFO, "%s doesn't exist locally, checking S3...", fileName.c_str());
            if (!aics::simularium::aws_util::Download(awsPath, rawPath)) {
                LOG_F(WARNING, "%s not found on AWS S3", fileName.c_str());
                return false;
            }
        }

        return true;
    }

    bool SimulationCache::FindFiles(std::vector<std::string> files)
    {
        for (std::string& file : files) {
            if (!FindFile(file))
                return false;
        }

        return true;
    }

    void SimulationCache::MarkTmpFiles(
        std::string identifier,
        std::vector<std::string> files)
    {
        this->m_tmpFiles[identifier] = files;
    }

    void SimulationCache::DeleteTmpFiles(std::string identifier)
    {
        auto files = this->m_tmpFiles[identifier];
        for (auto file : files) {
            if (std::remove(file.c_str()) != 0)
                LOG_F(WARNING, "Error deleting file %s", file.c_str());
            else
                LOG_F(INFO, "File %s succesfully deleted", file.c_str());
        }
    }

    void SimulationCache::WriteFilePropertiesToDisk(std::string identifier)
    {
        std::string filePropsPath = this->GetLocalInfoFilePath(identifier);
        std::string filePropsDest = this->GetS3InfoCachePath(identifier);
        std::ofstream propsFile;
        propsFile.open(filePropsPath);

        TrajectoryFileProperties tfp = this->GetFileProperties(identifier);
        Json::Value fprops;
        fprops["fileName"] = tfp.fileName;
        fprops["version"] = 1;
        fprops["totalSteps"] = static_cast<int>(tfp.numberOfFrames);
        fprops["timeStepSize"] = tfp.timeStepSize;
        fprops["spatialUnitFactorMeters"] = tfp.spatialUnitFactorMeters;

        Json::Value size;
        size["x"] = static_cast<float>(tfp.boxX);
        size["y"] = static_cast<float>(tfp.boxY);
        size["z"] = static_cast<float>(tfp.boxZ);
        fprops["size"] = size;

        Json::Value typeMapping;
        for (auto& entry : tfp.typeMapping) {
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
        std::string awsFilePath = this->GetS3TrajectoryCachePath(identifier);
        this->WriteFilePropertiesToDisk(identifier);

        std::string destination = awsFilePath;
        std::string source = this->GetLocalFilePath(identifier);
        LOG_F(INFO, "Uploading cache file for %s to S3", identifier.c_str());
        if (!aics::simularium::aws_util::Upload(source, destination)) {
            return false;
        }

        std::string filePropsPath = this->GetLocalInfoFilePath(identifier);
        std::string filePropsDest = this->GetS3InfoCachePath(identifier);

        LOG_F(INFO, "Uploading info file for %s to S3", identifier.c_str());
        if (!aics::simularium::aws_util::Upload(filePropsPath, filePropsDest)) {
            return false;
        }

        return true;
    }

    void SimulationCache::ParseFileProperties(std::string identifier)
    {
        std::string filePath = this->GetLocalInfoFilePath(identifier);
        LOG_F(INFO, "Loading info file %s from filepath %s", identifier.c_str(), filePath.c_str());

        std::ifstream is(filePath);
        Json::Value fprops;

        if (is.is_open()) {
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
        for (auto& id : ids) {
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
        tfp.spatialUnitFactorMeters = fprops["spatialUnitFactorMeters"].asFloat();

        this->m_fileProps[identifier] = tfp;
    }

    bool SimulationCache::IsFilePropertiesValid(std::string identifier)
    {
        std::string filePath = this->GetLocalInfoFilePath(identifier);
        std::ifstream is(filePath);
        Json::Value fprops;
        is >> fprops;

        std::vector<std::string> keys({ "version",
            "size",
            "typeMapping",
            "fileName",
            "totalSteps",
            "timeStepSize",
            "spatialUnitFactorMeters" });

        for (auto key : keys) {
            if (!fprops.isMember(key)) {
                LOG_F(WARNING, "File properties for identifier %s is missing key %s", identifier.c_str(), key.c_str());
                return false;
            }
        }

        return true;
    }

    std::string SimulationCache::GetLocalRawTrajectoryFilePath(std::string identifier)
    {
        return config::GetCacheFolder() + identifier;
    }

    std::string SimulationCache::GetLocalFilePath(std::string identifier)
    {
        return config::GetCacheFolder() + identifier + ".bin";
    }

    std::string SimulationCache::GetLocalInfoFilePath(std::string identifier)
    {
        return config::GetCacheFolder() + identifier + ".info";
    }

    std::string SimulationCache::GetS3TrajectoryPath(std::string identifier)
    {
        return config::GetS3Location() + identifier;
    }

    std::string SimulationCache::GetS3TrajectoryCachePath(std::string identifier)
    {
        return config::GetS3CacheLocation() + identifier + ".bin";
    }

    std::string SimulationCache::GetS3InfoPath(std::string identifier)
    {
        return config::GetS3Location() + identifier + ".info";
    }

    std::string SimulationCache::GetS3InfoCachePath(std::string identifier)
    {
        return config::GetS3CacheLocation() + identifier + ".info";
    }

    fileio::SimulariumBinaryFile* SimulationCache::GetBinaryFile(std::string identifier)
    {
        std::string path = this->GetLocalFilePath(identifier);

        if (!this->m_binaryFiles.count(identifier)) {
            this->m_binaryFiles[identifier] = std::make_shared<fileio::SimulariumBinaryFile>();

            if (FileExists(path)) {
                this->m_binaryFiles[identifier]->Open(path);
            } else {
                this->m_binaryFiles[identifier]->Create(path);
            }
        }

        return this->m_binaryFiles[identifier].get();
    }

} // namespace simularium
} // namespace aics
