#ifndef AICS_BINARY_CACHE_WRITER_H
#define AICS_BINARY_CACHE_WRITER_H

#include "agentsim/agent_data.h"
#include <string>
#include <fstream>
#include <iostream>

namespace aics {
namespace agentsim {
namespace fileio {

class BinaryCacheWriter {
public:
    void SerializeFrame(
      std::ofstream& os,
      std::size_t frame_number,
      AgentDataFrame& adf
    );

private:
    void SerializeAgent(
      std::ofstream& os,
      AgentData& ad
    );

    std::string m_delimiter = "/";
};

} // namespace fileio
} // namespace agentsim
} // namespace aics

#endif // AICS_BINARY_CACHE_WRITER_H
