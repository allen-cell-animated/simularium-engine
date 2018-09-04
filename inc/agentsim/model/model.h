#ifndef AICS_MODEL_H
#define AICS_MODEL_H

#include <memory>
#include <string>
#include <unordered_map>
#include <json/json.h>

namespace aics {
namespace agentsim {

struct SubAgentModelData {
	std::string Name;
	float x_offset, y_offset, z_offset;
};

struct AgentModelData
{
	std::string Name;
	std::vector<SubAgentModelData> Children;
};

struct AgentConcentrationData {
	std::string Name;
	float Value;
};

struct Model
{
public:
	std::string Name;
	float Volume;
	std::unordered_map<std::string, AgentModelData> Agents;
	std::unordered_map<std::string, float> Concentrations;
};

void parse_model(Json::Value& json_obj, aics::agentsim::Model& model);
void print_model(aics::agentsim::Model& model);

} // namespace agentsim
} // namespace aics

#endif // AICS_MODEL_H
