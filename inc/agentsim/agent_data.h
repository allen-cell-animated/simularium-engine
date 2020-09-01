#ifndef AICS_AGENT_DATA_H
#define AICS_AGENT_DATA_H

#include "json/json-forwards.h"
#include <vector>

namespace aics {
namespace agentsim {
    struct AgentData;

    typedef std::vector<aics::agentsim::AgentData> AgentDataFrame;
    typedef std::vector<aics::agentsim::AgentDataFrame> AgentDataFrameBundle;
} // namespace agentsim
} // namespace aics


namespace aics {
namespace agentsim {

    struct AgentData {
        float vis_type = 0;
        float id = 0;
        float type = 0;
        float x = 0;
        float y = 0;
        float z = 0;
        float xrot = 0;
        float yrot = 0;
        float zrot = 0;
        float collision_radius = 0;
        std::vector<float> subpoints;
    };

    Json::Value Serialize(AgentDataFrame& adf);
    std::vector<float> Serialize(AgentData& ad);
}
}

#endif // AICS_AGENT_DATA_H
