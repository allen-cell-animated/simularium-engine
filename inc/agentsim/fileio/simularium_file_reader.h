#ifndef AICS_SIMULARIUM_FILE_READER_H
#define AICS_SIMULARIUM_FILE_READER_H

#include "agentsim/agent_data.h"
#include "agentsim/network/trajectory_properties.h"
#include <json/json.h>

namespace aics {
namespace agentsim {
namespace fileio {

class SimulariumFileReader {
public:
    bool DeserializeFrame(
      Json::Value& jsonRoot,
      std::size_t frameNumber,
      AgentDataFrame& adf
    );
};

} // namespace fileio
} // namespace agentsim
} // namespace aics

#endif // AICS_SIMULARIUM_FILE_READER_H
