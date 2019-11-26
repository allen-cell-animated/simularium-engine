#ifndef AICS_SIMULATION_CACHE_H
#define AICS_SIMULATION_CACHE_H

#include "agentsim/agent_data.h"
#include <algorithm>
#include <iostream>
#include <memory>
#include <string>
#include <vector>
#include <fstream>
#include <unordered_map>

namespace aics {
namespace agentsim {

    typedef std::vector<AgentData> AgentDataFrame;
    typedef std::vector<AgentDataFrame> FrameList;

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

        bool DownloadRuntimeCache(std::string filePath, std::string identifier);
        void UploadRuntimeCache(std::string filePath, std::string identifier);

    private:
        std::string GetFilePath(std::string identifier);
        inline void CreateCacheFolder() { system("mkdir -p /tmp/aics/simularium/"); }
        inline void DeleteCacheFolder() { system("rm -rf /tmp/aics/"); }

        std::ofstream& GetOfstream(std::string& identifier);
        std::ifstream& GetIfstream(std::string& identifier);

        void CloseFileStreams();

        std::string m_cacheFolder = "/tmp/aics/simularium/";
        std::ios_base::openmode m_ofstreamFlags = std::ios::out | std::ios::app | std::ios::binary;
        std::ios_base::openmode m_ifstreamFlags = std::ios::in | std::ios::binary;
        std::unordered_map<std::string, std::ofstream> m_ofstreams;
        std::unordered_map<std::string, std::ifstream> m_ifstreams;
        std::unordered_map<std::string, std::size_t> m_numFrames;
    };

}
}

#endif // AICS_SIMULATION_CACHE_H
