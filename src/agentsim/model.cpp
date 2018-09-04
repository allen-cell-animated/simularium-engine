#include "agentsim/model/model.h"
#include <iostream>

bool parse(Json::Value& obj, std::string key, float& out);
bool parse(Json::Value& obj, std::string key, std::string& out);

namespace aics {
namespace agentsim {

void parse_model(Json::Value& json_obj, aics::agentsim::Model& model)
{
	parse(json_obj, "name", model.Name);

	auto params = json_obj["parameters"];
	parse(params, "volume", model.Volume);

	auto agents = json_obj["agents"];
	for(Json::Value::iterator it=agents.begin(); it!=agents.end(); ++it)
	{
		AgentModelData ad;
		parse(*it, "name", ad.Name);

		auto children = (*it)["children"];
		for(Json::Value::iterator itc=children.begin(); itc!=children.end(); ++itc)
		{
			SubAgentModelData sad;
			parse(*itc, "name", sad.Name);
			parse(*itc, "x_offset", sad.x_offset);
			parse(*itc, "y_offset", sad.y_offset);
			parse(*itc, "z_offset", sad.z_offset);
			ad.Children.push_back(sad);
		}

		model.Agents[ad.Name] = ad;
	}

	auto concentrations = json_obj["seed-concentrations"];
	for(Json::Value::iterator it=concentrations.begin(); it != concentrations.end(); ++it)
	{
		std::string name;
		float concentration;
		parse(*it, "name", name);
		parse(*it, "concentration", concentration);

		model.Concentrations[name] = concentration;
	}
}

void print_model(aics::agentsim::Model& model)
{
	std::cout << "Model: " << model.Name << "\n";
	std::cout << "Volume: " << model.Volume << "\n";

	for(auto entry : model.Agents)
	{
		std::string key = entry.first;
		std::cout << "Agent: " << key << " \n";
		for(auto subagent : entry.second.Children)
		{
			std::cout << "Sub Agent: " << subagent.Name << " Offset:("
				<< subagent.x_offset << "," << subagent.y_offset << ","
				<< subagent.z_offset << ")\n";
		}
	}

	std::cout << "Initial Conditions: \n";
	for(auto entry : model.Concentrations)
	{
		std::string key = entry.first;
		std::cout << entry.second << " concentration of " << entry.first << "\n";
	}
}

} // namespace agentsim
} // namespace aics

bool parse(Json::Value& obj, std::string key, float& out)
{
		if(obj.isMember(key))
		{
			out = obj[key].asFloat();
			return true;
		}
		else
		{
			printf("Parameter %s not found in json model data.\n", key.c_str());
			return false;
		}
}

bool parse(Json::Value& obj, std::string key, std::string& out)
{
		if(obj.isMember(key))
		{
			out = obj[key].asString();
			return true;
		}
		else
		{
			printf("Parameter %s not found in json model data.\n", key.c_str());
			return false;
		}
}
