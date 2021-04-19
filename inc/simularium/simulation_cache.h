#ifndef AICS_SIMULATION_CACHE_H
#define AICS_SIMULATION_CACHE_H

#include "simularium/agent_data.h"
#include "simularium/network/trajectory_properties.h"
#include "simularium/fileio/simularium_binary_file.h"
#include <json/json.h>
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

namespace aics {
namespace simularium {

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
        void AddFrame(std::string identifier, TrajectoryFrame frame);

        /**
        *   GetFrame
        *
        *   @param      identifier      a name used to specify the cache to be read
        *   @param      frameNumber     the frame number to retrieve
        *
        *   Returns a single trajectory frame from a cache specified by the
        *   'identifier' parameter. The frame is returned as a broadcastable data
        *   buffer
        */
        BroadcastUpdate GetBroadcastFrame(std::string identifier, std::size_t frameNumber);

        /**
        *   GetBroadcastUpdate
        *
        *   @param    identifier        specifies the cache to be read
        *   @param    currentPosition   the position of the requesting trajectory
        *                                 playback streamer. Indicates a file
        *                                 position for the binary file being read/streamed
        *   @param    bufferSize        how many bits of data to include in this broadcast
        *                                 update
        *
        *   Returns a BroadcastUpdate object, containing data to be transmited
        *     and a new playback-position for the requesting streamer to save
        */
        BroadcastUpdate GetBroadcastUpdate(
          std::string identifier,
          std::size_t currentPosition,
          std::size_t bufferSize
        );

        std::size_t GetEndOfStreamPos(
          std::string identifier
        );

        std::size_t GetFramePos(
          std::string identifier,
          std::size_t frameNumber
        );

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
        std::string GetS3TrajectoryPath(std::string identifier);
        std::string GetS3TrajectoryCachePath(std::string identifier);
        std::string GetS3InfoPath(std::string identifier);
        std::string GetS3InfoCachePath(std::string identifer);

        fileio::SimulariumBinaryFile* GetBinaryFile(std::string identifier);

        void ParseFileProperties(std::string identifier);
        void ParseFileProperties(Json::Value& jsonRoot, std::string identifier);
        bool IsFilePropertiesValid(std::string identifier);

        std::unordered_map<std::string, TrajectoryFileProperties> m_fileProps;
        std::unordered_map<std::string, std::vector<std::string>> m_tmpFiles;
        std::unordered_map<std::string, std::shared_ptr<fileio::SimulariumBinaryFile>> m_binaryFiles;
    };
}
}

#endif // AICS_SIMULATION_CACHE_H
