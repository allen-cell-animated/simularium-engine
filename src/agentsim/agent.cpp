#include "agentsim/agents/agent.h"
#include <stdlib.h>
#include <time.h>
#include "agentsim/pattern/agent_pattern.h"
#include "agentsim/common/logger.h"
#include <iostream>

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

const bool Agent::FindSubAgent(const AgentPattern& pattern, Agent*& outptr)
{
	bool matches = this->Matches(pattern);
	std::size_t child_count = this->m_childAgents.size();
	if(matches)
	{
		outptr = this;
		return true;
	}

	if(!matches && child_count > 0)
	{
		for(std::size_t i = 0; i < this->m_childAgents.size(); ++i)
		{
			if(this->m_childAgents[i]->FindSubAgent(pattern, outptr))
			{
				outptr = this->m_childAgents[i].get();
				return true;
			}
		}
	}

	// no matches and no children left to search
	return false;
}

const bool Agent::Matches(const AgentPattern& pattern) const
{
	if(pattern.Name != this->m_agentName)
	{
		return false;
	}

	if(pattern.State != this->m_agentState)
	{
		return false;
	}

	if(pattern.IsWildCardBound && this->m_boundPartners.size() == 0)
	{
		return false;
	}

	std::unordered_map<std::string, bool> ignore;
	for(std::size_t i = 0; i < pattern.ChildAgents.size(); ++i)
	{
		Agent* outptr = nullptr;
		if(!this->FindChildAgent(pattern.ChildAgents[i], outptr, ignore))
		{
			return false;
		}
		ignore[outptr->GetID()] = true;
	}

	// Above, we check for wild-card bond mismatch
	if(pattern.IsWildCardBound)
	{
		return true;
	}

	// assuming id collisions are rare
	//  for this reason, the ignore list is not being cleared
	for(std::size_t i = 0; i < pattern.BoundPartners.size(); ++i)
	{
		Agent* outptr = nullptr;
		if(!this->FindBoundPartner(pattern.ChildAgents[i], outptr, ignore))
		{
			return false;
		}
		ignore[outptr->GetID()] = true;
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

const bool Agent::FindBoundPartner(
	const AgentPattern& pattern,
	Agent*& outptr,
	std::unordered_map<std::string, bool> ignore) const
{
	for(std::size_t i = 0; i < this->m_boundPartners.size(); ++i)
	{
		if(ignore[this->m_boundPartners[i]->GetID()])
			continue;

		if(this->m_boundPartners[i]->Matches(pattern))
		{
			outptr = this->m_boundPartners[i].get();
			break;
		}
	}

	return outptr != nullptr;
}

bool Agent::CopyState(AgentPattern& pattern)
{
	this->m_agentState = pattern.State;

	std::unordered_map<std::string, bool> ignore;
	for(std::size_t i = 0; i < pattern.ChildAgents.size(); ++i)
	{
		Agent* outptr = nullptr;
		if(!this->FindChildAgent(pattern.ChildAgents[i], outptr, ignore))
		{
			PRINT_ERROR("Agent.cpp: a match could not be found for a child agent while copying state.\n")
			return false;
		}
		ignore[outptr->GetID()] = true;
		outptr->CopyState(pattern.ChildAgents[i]);
	}

	return true;
}

} // namespace agentsim
} // namespace aics
