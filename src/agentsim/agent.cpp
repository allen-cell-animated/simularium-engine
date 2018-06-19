#include "agentsim/agents/agent.h"
#include <stdlib.h>
#include <time.h>
#include "agentsim/pattern/agent_pattern.h"
#include "agentsim/common/logger.h"

namespace aics {
namespace agentsim {

namespace agents
{
void GenerateLocalUUID(std::string& uuid)
{
	// Doesn't matter if time is seeded or not at this point
	//  just need relativley unique local identifiers for runtime
	char strUuid[128];
	sprintf(strUuid, "%x%x-%x-%x-%x-%x%x%x",
	rand(), rand(),                 // Generates a 64-bit Hex number
	rand(),                         // Generates a 32-bit Hex number
	((rand() & 0x0fff) | 0x4000),   // Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
	rand() % 0x3fff + 0x8000,       // Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
	rand(), rand(), rand());        // Generates a 96-bit Hex number

	uuid = strUuid;
}
} // namespace agents

Agent::Agent()
{
	this->m_location << 0, 0, 0;
	this->m_rotation << 0, 0, 0;

	agents::GenerateLocalUUID(this->m_agentID);
}

bool Agent::AddBoundPartner(std::shared_ptr<Agent> other)
{
	if(other->m_agentID == this->m_agentID)
	{
		PRINT_WARNING("Agent.cpp: An Agent cannot be bound to itself.\n")
		return false;
	}

	this->m_boundPartners.push_back(other);
	return true;
}

bool Agent::AddChildAgent(std::shared_ptr<Agent> other)
{
	if(other->m_agentID == this->m_agentID)
	{
		PRINT_WARNING("Agent.cpp: An Agent cannot be parented to itself.\n")
		return false;
	}

	this->m_childAgents.push_back(other);
	return true;
}

bool Agent::CanInteractWith(const Agent& other)
{
	if(this->m_agentID == other.m_agentID)
	{
			return false;
	}

	float interaction_dist_squared =
		(this->m_interaction_distance + other.m_interaction_distance) *
		(this->m_interaction_distance + other.m_interaction_distance);

	float dist_squared = (this->m_location - other.m_location).squaredNorm();

	return dist_squared <= interaction_dist_squared;
}

bool Agent::IsCollidingWith(const Agent& other)
{
	float dist_squared = (this->m_location - other.m_location).squaredNorm();
	float coll_dist_squared =
		(this->m_collision_radius + other.m_collision_radius) *
		(this->m_collision_radius + other.m_collision_radius);

	return dist_squared <= coll_dist_squared;
}

const bool Agent::Matches(const AgentPattern& pattern) const
{
	if(pattern.Name != this->m_agentName)
	{
		return false;
	}

	for(std::size_t i = 0; i < pattern.ChildAgents.size(); ++i)
	{
		Agent* outptr = nullptr;
		if(!this->FindChildAgent(pattern.ChildAgents[i], outptr))
		{
			return false;
		}
	}

	return true;
}

const bool Agent::FindChildAgent(
	const AgentPattern& pattern,
	Agent*& outptr,
	std::unordered_map<std::string, bool> ignore) const
{
	for(std::size_t i = 0; i < this->m_childAgents.size(); ++i)
	{
		if(ignore[this->m_childAgents[i]->GetID()])
			continue;

		if(this->m_childAgents[i]->Matches(pattern))
		{
			outptr = this->m_childAgents[i].get();
			break;
		}
	}

	return outptr != nullptr;
}

} // namespace agentsim
} // namespace aics
