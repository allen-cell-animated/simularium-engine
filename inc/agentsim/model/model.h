#ifndef AICS_MODEL_H
#define AICS_MODEL_H

#include <memory>
#include <tuple>
#include <string>
#include <unordered_map>
#include <json/json.h>
#include "agentsim/reactions/reaction.h"

namespace aics {
namespace agentsim {

struct SubComplexModelData {
	std::string name;
	int min_count, max_count;
};

struct ComplexModelData {
	std::string name;
	std::vector<SubComplexModelData> children;
};

struct SubAgentModelData {
	std::string name;
	float x_offset, y_offset, z_offset;
	bool visible;
};

struct AgentModelData
{
	std::string name;
	std::string complex_name;
	std::vector<SubAgentModelData> children;
	float diffusion_coefficient;
	float bound_diffusion_coefficient;
	float collision_radius;
	int type_id;
};

struct AgentConcentrationData {
	std::string name;
	float value;
};

struct DynamicsConstraintConfig {
	std::string type;
	std::vector<std::string> agents;
	std::unordered_map<std::string, float> parameters;
};

struct Model
{
public:
	float min_time_Step, max_time_step;
	float fusion_bond_force_constant, fusion_angle_force_constant;
	float parent_child_bond_force_constant, parent_child_angle_force_constant;
	float child_diffusion_coefficient;

	std::string name;
	float volume;
	std::unordered_map<std::string, AgentModelData> agents;
	std::unordered_map<std::string, ComplexModelData> complexes;
	std::unordered_map<std::string, float> concentrations;
	std::unordered_map<std::string, float> seed_counts;
	std::unordered_map<std::string, Reaction> reactions;
	std::vector<DynamicsConstraintConfig> constraints;
};

void parse_model(Json::Value& json_obj, aics::agentsim::Model& model);
void print_model(aics::agentsim::Model& model);

} // namespace agentsim
} // namespace aics

#endif // AICS_MODEL_H
