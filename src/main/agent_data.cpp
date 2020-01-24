#include "agentsim/agent_data.h"
#include "json/json.h"

namespace aics {
namespace agentsim {

Json::Value Serialize(AgentDataFrame& adf) {
    std::vector<float> vals;
    for (std::size_t i = 0; i < adf.size(); ++i) {
        auto agentData = adf[i];
        vals.push_back(agentData.vis_type);
        vals.push_back(agentData.type);
        vals.push_back(agentData.x);
        vals.push_back(agentData.y);
        vals.push_back(agentData.z);
        vals.push_back(agentData.xrot);
        vals.push_back(agentData.yrot);
        vals.push_back(agentData.zrot);
        vals.push_back(agentData.collision_radius);
        vals.push_back(agentData.subpoints.size());

        for (std::size_t j = 0; j < agentData.subpoints.size(); ++j) {
            vals.push_back(agentData.subpoints[j]);
        }
    }

    // Copy values to json data array
    Json::Value dataArr = Json::Value(Json::arrayValue);
    for (int index = 0; index < vals.size(); ++index) {
        dataArr[index] = vals[index];
    }

    return dataArr;
}

} // namespace agentsim
} // namespace aics
