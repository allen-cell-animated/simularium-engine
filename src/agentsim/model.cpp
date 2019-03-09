#include "agentsim/model/model.h"
#include <iostream>
#include <algorithm>

bool parse(Json::Value& obj, std::string key, int& out);
bool parse(Json::Value& obj, std::string key, float& out);
bool parse(Json::Value& obj, std::string key, std::string& out);
bool parse(Json::Value& obj, std::string key, bool& out);

aics::agentsim::Reaction reverse(
	aics::agentsim::Reaction& rx);

namespace aics {
namespace agentsim {

void parse_model(Json::Value& json_obj, aics::agentsim::Model& model)
{
	parse(json_obj, "name", model.name);

	auto params = json_obj["parameters"];
	parse(params, "volume", model.volume);
	parse(params, "min-timestep", model.min_time_Step);
	parse(params, "max-timestep", model.max_time_step);
	parse(params, "fusion-bond-force-constant", model.fusion_bond_force_constant);
	parse(params, "fusion-angle-force-constant", model.fusion_angle_force_constant);
	parse(params, "parent-child-bond-force-constant", model.parent_child_bond_force_constant);
	parse(params, "parent-child-angle-force-constant", model.parent_child_angle_force_constant);
	parse(params, "child-diffusion-coefficient", model.child_diffusion_coefficient);

	auto agents = json_obj["agents"];
	int id_counter = 0;
	for(Json::Value::iterator it=agents.begin(); it!=agents.end(); ++it)
	{
		AgentModelData amd;
		parse(*it, "name", amd.name);

		if(!parse(*it, "diffusion-coefficient", amd.diffusion_coefficient))
		{
			amd.diffusion_coefficient = 0;
		}

		if(!parse(*it, "bound-diffusion-coefficient", amd.bound_diffusion_coefficient))
		{
				amd.bound_diffusion_coefficient = amd.diffusion_coefficient;
		}

		parse(*it, "collision-radius", amd.collision_radius);
		amd.type_id = id_counter++;

		auto children = (*it)["children"];
		for(Json::Value::iterator itc=children.begin(); itc!=children.end(); ++itc)
		{
			SubAgentModelData sad;
			parse(*itc, "name", sad.name);
			parse(*itc, "x_offset", sad.x_offset);
			parse(*itc, "y_offset", sad.y_offset);
			parse(*itc, "z_offset", sad.z_offset);
			parse(*itc, "visible", sad.visible);
			amd.children.push_back(sad);
		}

		model.agents[amd.name] = amd;
	}

	auto complex_definitions = json_obj["complexes"];
	for(auto complex : complex_definitions)
	{
		ComplexModelData cmd;
		parse(complex, "name", cmd.name);

		auto agents = complex["agents"];
		for(auto agent : agents)
		{
			SubComplexModelData smd;
			parse(agent, "name", smd.name);
			parse(agent, "max-count", smd.max_count);
			parse(agent, "min-count", smd.min_count);
			cmd.children.push_back(smd);
		}
		model.complexes[cmd.name] = cmd;
	}

	auto concentrations = json_obj["seed-concentrations"];
	for(Json::Value::iterator it=concentrations.begin(); it != concentrations.end(); ++it)
	{
		std::string name;
		float concentration;
		parse(*it, "name", name);
		parse(*it, "concentration", concentration);

		model.concentrations[name] = concentration;
	}

	auto counts = json_obj["seed-counts"];
	for(Json::Value::iterator it=counts.begin(); it != counts.end(); ++it)
	{
		std::string name;
		float count;
		parse(*it, "name", name);
		parse(*it, "count", count);

		model.seed_counts[name] = count;
	}

	auto dynamics = json_obj["dynamics"];
	for(Json::Value::iterator it=dynamics.begin(); it != dynamics.end(); ++it)
	{
		DynamicsConstraintConfig dcc;
		float param_value;

		parse(*it, "type", dcc.type);
		if(parse(*it, "force-constant", param_value)) dcc.parameters["force-constant"] = param_value;
		if(parse(*it, "multiplicity", param_value)) dcc.parameters["multiplicity"] = param_value;
		if(parse(*it, "angle", param_value)) dcc.parameters["angle"] = param_value;
		if(parse(*it, "radius", param_value)) dcc.parameters["radius"] = param_value;

		dcc.agents.push_back((*it)["agents"][0].asString());
		dcc.agents.push_back((*it)["agents"][1].asString());
		dcc.agents.push_back((*it)["agents"][2].asString());
		dcc.agents.push_back((*it)["agents"][3].asString());

		model.constraints.push_back(dcc);
	}

	auto reactions = json_obj["reactions"];
	for(auto reaction : reactions)
	{
		Reaction r;
		parse(reaction, "name", r.name);
		parse(reaction, "type", r.type);
		parse(reaction, "description", r.description);
		parse(reaction, "rate", r.rate);
		parse(reaction, "reverse-rate", r.reverse_rate);
		parse(reaction, "distance", r.distance);
		parse(reaction, "product", r.product);

		auto complexes = reaction["complexes"];
		for(auto complex : complexes)
		{
			rxComplex _complex;
			parse(complex, "name", _complex.name);

			auto parents = complex["parents"];
			for(auto parent : parents)
			{
					rxParentState _parent;
					parse(parent, "name", _parent.name);

					auto state = parent["state"];
					auto children = state["children"];
					for(auto child : children)
					{
						rxChildState _child;
						parse(child, "name", _child.name);
						parse(child, "state", _child.state);
						parse(child, "isBound", _child.isBound);
						_parent.children.push_back(_child);
					}

					_complex.parents.push_back(_parent);
			}

			r.complexes.push_back(_complex);
		}

		auto centers = reaction["centers"];
		for(auto center : centers)
		{
			rxCenter _center;
			_center.parent_1 = center["parents"][0].asInt();
			_center.parent_2 = center["parents"][1].asInt();
			_center.child_1 = center["children"][0].asString();
			_center.child_2 = center["children"][1].asString();
			r.centers.push_back(_center);
		}

		model.reactions[r.name] = r;

		if(r.reverse_rate > 0.0f)
		{
			auto rev_r = reverse(r);
			model.reactions[rev_r.name] = rev_r;
		}
	}
}

void print_model(aics::agentsim::Model& model)
{
	std::cout << "\nBEGIN MODEL\n";
	std::cout << "Name: " << model.name << "\n";
	std::cout << "Volume: " << model.volume << "\n";
	std::cout << "Timestep: [" << model.min_time_Step << "-" << model.max_time_step<< "]\n";
	std::cout << "Parent-Child Bond Force Constant: " << model.parent_child_bond_force_constant << "\n";
	std::cout << "Parent-Child Angle Force Constant: " << model.parent_child_angle_force_constant << "\n";
	std::cout << "Fusion Bond Force Constant: " << model.fusion_bond_force_constant << "\n";
	std::cout << "Fusion Angle Force Constant: " << model.fusion_angle_force_constant << "\n";
	std::cout << "Child Diffusion Coefficient: " << model.child_diffusion_coefficient << "\n";

	std::cout << "\nAGENTS: \n";
	for(auto entry : model.agents)
	{
		auto agent_model_data = entry.second;

		std::cout << "Agent: " << agent_model_data.name << " \n";
		std::cout << "> diff coeff: " << agent_model_data.diffusion_coefficient << " \n";
		std::cout << "> bound diff coeff: " << agent_model_data.bound_diffusion_coefficient << " \n";
		std::cout << "> coll radius: " << agent_model_data.collision_radius << " \n";
		for(auto subagent : agent_model_data.children)
		{
			std::cout << "> Sub Agent: " << subagent.name << " Offset:("
				<< subagent.x_offset << "," << subagent.y_offset << ","
				<< subagent.z_offset << ")\n";
		}
	}

	std::cout << "\nMETA-AGENTS\n";
	for(auto entry : model.complexes)
	{
		auto complex_model_data = entry.second;
		std::cout << "Complex :" << complex_model_data.name << "\n";
		for(auto subagent : complex_model_data.children)
		{
			std::cout << "> Sub-Agent: " << subagent.name
				<< " [" << subagent.min_count
				<< "-" << subagent.max_count << "]\n";
		}
	}

	std::cout << "\nDYNAMICS\n";
	for(auto entry : model.constraints)
	{
		std::cout << "Constraint\n";

		for(auto param : entry.parameters)
		{
			std::cout << param.first << ": " << param.second << "\n";
		}

		for(std::size_t i = 0; i < entry.agents.size(); ++i)
		{
			std::cout << "> Agent " << i << ": " << entry.agents[i] << "\n";
		}
	}


	std::cout << "\nAGENT CONCENTRATIONS: \n";
	for(auto entry : model.concentrations)
	{
		auto concentration = entry.second;
		std::cout << concentration << " concentration of " << entry.first << "\n";
	}

	std::cout << "\nREACTIONS: \n";
	for(auto entry : model.reactions)
	{
		auto reaction_model_data = entry.second;
		std::cout << "\nReaction: " << entry.first << "\n";
		std::cout << "> type: " << reaction_model_data.type << "\n";
		std::cout << "> description: " << reaction_model_data.description << "\n";
		std::cout << "> rate: " << reaction_model_data.rate << "\n";
		std::cout << "> reverse-rate: " << reaction_model_data.reverse_rate << "\n";
		std::cout << "> distance: " << reaction_model_data.distance << "\n";

		for(auto complex : reaction_model_data.complexes)
		{
			std::cout << "> complex: " << complex.name << "\n";
			for(auto parent : complex.parents)
			{
				std::cout << ">> parent: " << parent.name << "\n";
				for(auto child : parent.children)
				{
					std::cout << ">>> child : " << child.name << " bound[" << child.isBound << "]\n";
				}
			}
		}

		for(auto center : reaction_model_data.centers)
		{
			std::cout << "> rx center :"
			<< " parent 1: " << center.parent_1
			<< " parent 2: " << center.parent_2
			<< " child 1: " << center.child_1
			<< " child 2: " << center.child_2 << "\n";
		}

		std::cout << "> product: " << reaction_model_data.product << "\n";
	}
	std::cout << "\nEND MODEL\n\n";
}

} // namespace agentsim
} // namespace aics

bool parse(Json::Value& obj, std::string key, int& out)
{
		if(obj.isMember(key))
		{
			out = obj[key].asInt();
			return true;
		}
		else
		{
			printf("Parameter %s not found in json model data.\n", key.c_str());
			return false;
		}
}

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

bool parse(Json::Value& obj, std::string key, bool& out)
{
		if(obj.isMember(key))
		{
			out = obj[key].asBool();
			return true;
		}
		else
		{
			printf("Parameter %s not found in json model data.\n", key.c_str());
			return false;
		}
}

aics::agentsim::Reaction reverse(
	aics::agentsim::Reaction& rx)
{
	aics::agentsim::Reaction out;
	out.name = "rev_" + rx.name;
	out.rate = rx.reverse_rate;
	out.reverse_rate = rx.rate;

	out.type = "fission";
	out.distance = rx.distance;

	out.complexes = rx.complexes;
	out.centers = rx.centers;

	std::size_t parent_count_1 = rx.complexes[0].parents.size();
	for(auto& center : out.centers)
	{
		auto& children1 = out.complexes[0].parents[center.parent_1].children;
		auto& children2 = out.complexes[1].parents[center.parent_2].children;

		auto& child1 = *std::find_if(
			children1.begin(), children1.end(),
			[&center](auto& child)
			{
				return child.name == center.child_1;
			});

		auto& child2 = *std::find_if(
			children2.begin(), children2.end(),
			[&center](auto& child)
			{
				return child.name == center.child_2;
			});

		child1.isBound = true;
		child2.isBound = true;

		center.parent_2 += parent_count_1;
	}

	for(auto parent : out.complexes[1].parents)
	{
		out.complexes[0].parents.push_back(parent);
	}
	out.complexes.erase(out.complexes.begin() + 1);
	out.complexes[0].name = rx.product;
	out.product = rx.complexes[0].name + "_" + rx.complexes[1].name;
	return out;
}
