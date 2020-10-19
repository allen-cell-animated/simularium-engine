#ifndef AICS_SIMULATION_CACHE_H
#define AICS_SIMULATION_CACHE_H

#include "agentsim/agent_data.h"
#include "agentsim/network/trajectory_properties.h"
#include "agentsim/fileio/binary_cache_reader.h"
#include "agentsim/fileio/binary_cache_writer.h"
#include <json/json.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

namespace aics {
namespace agentsim {

    class SimulationCache {
    public:
        SimulationCache();
        ~SimulationCache();

        /**
        *   AddFrame
        *
        *   @param  identifier      a name used to reference the trajectory being saved to a cache
        *                           frames can be retrieved using the same identifier specified here
        *   @param  data            the trajectory frame to be stored in a cache
        *
        *   Stores data for a single trajectory frame in a cache that can be
        *   retrieved using the 'identifier' parameter
        */
        void AddFrame(std::string identifier, AgentDataFrame data);

        /**
        *   GetFrame
        *
        *   @param      identifier      a name used to specify the cache to be read
        *   @param      frameNumber     the frame number to retrieve
        *
        *   Returns a single trajectory frame from a cache specified by the
        *   'identifier' parameter
        */
        AgentDataFrame GetFrame(std::string identifier, std::size_t frameNumber);

        std::size_t GetNumFrames(std::string identifier);

        /**
        *   ClearCache
        *
        *   Deletes a single cache specified by the 'identifier' parameter
        */
        void ClearCache(std::string identifier);

        void Preprocess(std::string identifier);

        bool DownloadRuntimeCache(std::string identifier);
        bool UploadRuntimeCache(std::string identifier);

        bool HasIdentifier(std::string identifier) { return this->m_fileProps.count(identifier); }

        TrajectoryFileProperties GetFileProperties(std::string identifier) {
            return this->m_fileProps.count(identifier) ?
                this->m_fileProps[identifier] : TrajectoryFileProperties();
        }

        void SetFileProperties(std::string identifier, TrajectoryFileProperties tfp) {
            this->m_fileProps[identifier] = tfp;
        }

        std::string GetLocalRawTrajectoryFilePath(std::string identifier);

        bool FindFiles(std::vector<std::string> files);

        /**
        *   FindSimulariumFile
        *
        *   @param  fileName    the name of the file to find on S3,
        *                       the file extension will be replaced with ".simularium"
        *                       for the search
        *
        *   This function will download, convert, and upload a cache for a
        *   .simularium file to S3, identifiable by the fileName passed in
        */
        bool FindSimulariumFile(std::string fileName);

        void MarkTmpFiles(std::string identifier, std::vector<std::string> files);
        void DeleteTmpFiles(std::string identifier);

    private:
        bool FindFile(std::string file);
        void WriteFilePropertiesToDisk(std::string identifier);

        // Given how files are searched for in this app, changing any of the
        //  below functions will cause file processing to re-occur
        //  outdated cache-files will need to be manually removed from S3
        std::string GetLocalFilePath(std::string identifier);
        std::string GetLocalInfoFilePath(std::string identifier);
        std::string GetAwsFilePath(std::string identifier);
        std::string GetAwsInfoFilePath(std::string identifier);

        inline void CreateCacheFolder() { int ignore = system("mkdir -p /tmp/aics/simularium"); }
        inline void DeleteCacheFolder() { int ignore = system("rm -rf /tmp/aics/simularium"); }

        std::ofstream& GetOfstream(std::string& identifier);
        std::ifstream& GetIfstream(std::string& identifier);

        void CloseFileStreams();

        void ParseFileProperties(std::string identifier);
        void ParseFileProperties(Json::Value& jsonRoot, std::string identifier);
        bool IsFilePropertiesValid(std::string identifier);

        const std::string kCacheFolder = "/tmp/aics/simularium/";
        const std::string kAwsPrefix = "trajectory/";

        std::ios_base::openmode m_ofstreamFlags = std::ios::out | std::ios::app | std::ios::binary;
        std::ios_base::openmode m_ifstreamFlags = std::ios::in | std::ios::binary;
        std::unordered_map<std::string, std::ofstream> m_ofstreams;
        std::unordered_map<std::string, std::ifstream> m_ifstreams;
        std::unordered_map<std::string, std::size_t> m_numFrames;
        std::unordered_map<std::string, TrajectoryFileProperties> m_fileProps;
        std::unordered_map<std::string, std::vector<std::string>> m_tmpFiles;

        fileio::BinaryCacheWriter m_binaryCacheWriter;
        fileio::BinaryCacheReader m_binaryCacheReader;
    };

}
}

#endif // AICS_SIMULATION_CACHE_H
