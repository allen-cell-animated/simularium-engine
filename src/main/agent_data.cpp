#include "simularium/agent_data.h"
#include "json/json.h"

namespace aics {
namespace simularium {

Json::Value Serialize(AgentDataFrame& adf) {
    std::vector<float> vals;
    
    // Serialize agents to float sequence
    for (std::size_t i = 0; i < adf.size(); ++i) {
        auto agentData = adf[i];
        std::vector<float> agentValArr = Serialize(agentData);

        for (auto v : agentValArr) {
            vals.push_back(v);
        }
    }

    // Copy values to json data array
    Json::Value dataArr = Json::Value(Json::arrayValue);
    for (int index = 0; index < vals.size(); ++index) {
        dataArr[index] = vals[index];
    }

    return dataArr;
}


std::vector<float> Serialize(AgentData& agentData) {
    std::vector<float> vals;
    vals.push_back(agentData.vis_type);
    vals.push_back(agentData.id);
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

    return vals;
}

} // namespace simularium
} // namespace aics
