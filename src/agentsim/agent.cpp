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

enum MatchOptions {
	IgnoreBonds = 0x01 // matches bound partners by number of bonds instead of a search
};
} // namespace agents

namespace math_util
{
Eigen::Affine3d CreateRotationMatrix(double ax, double ay, double az) {
  Eigen::Affine3d rx =
      Eigen::Affine3d(Eigen::AngleAxisd(ax, Eigen::Vector3d(1, 0, 0)));
  Eigen::Affine3d ry =
      Eigen::Affine3d(Eigen::AngleAxisd(ay, Eigen::Vector3d(0, 1, 0)));
  Eigen::Affine3d rz =
      Eigen::Affine3d(Eigen::AngleAxisd(az, Eigen::Vector3d(0, 0, 1)));
  return rz * ry * rx;
}

Eigen::Matrix4d CreateTransform(Eigen::Vector3d loc, Eigen::Vector3d rot)
{
	return (Eigen::Translation3d(loc) * CreateRotationMatrix(rot[0], rot[1], rot[2])).matrix();
}

} // namespace math_util

Agent::Agent()
{
	this->m_location << 0, 0, 0;
	this->m_rotation << 0, 0, 0;
	this->m_parentTransform.setIdentity();

	agents::GenerateLocalUUID(this->m_agentID);
}

void Agent::SetLocation(Eigen::Vector3d newLocation)
{
	this->m_location = newLocation;
	this->UpdateParentTransform(this->m_parentTransform); // triggers transform updates in children
}

void Agent::SetRotation(Eigen::Vector3d newRotation)
{
	this->m_rotation = newRotation;
	this->UpdateParentTransform(this->m_parentTransform); // triggers transform updates in children
}

const Eigen::Matrix4d Agent::GetTransform()
{
	return math_util::CreateTransform(this->m_location, this->m_rotation);
}

const Eigen::Matrix4d Agent::GetGlobalTransform()
{
	return this->m_parentTransform * this->GetTransform();
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
	other->UpdateParentTransform(this->GetGlobalTransform());
	return true;
}

std::shared_ptr<Agent> Agent::GetChildAgent(std::size_t index)
{
	if(index < 0 || index > this->m_childAgents.size())
	{
		return nullptr;
	}

	return this->m_childAgents[index];
}

std::shared_ptr<Agent> Agent::GetBoundPartner(std::size_t index)
{
	if(index < 0 || index > this->m_boundPartners.size())
	{
		return nullptr;
	}

	return this->m_boundPartners[index];
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

const bool Agent::FindSubAgent(const AgentPattern& pattern, Agent*& outptr,
	std::unordered_map<std::string, bool> ignore)
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
			if(ignore[this->m_childAgents[i]->GetID()] == true)
			{
				continue;
			}

			if(this->m_childAgents[i]->FindSubAgent(pattern, outptr))
			{
				return true;
			}
		}
	}

	// no matches and no children left to search
	outptr = nullptr;
	return false;
}

const bool Agent::Matches(const AgentPattern& pattern, unsigned char flags)
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

	if(pattern.BoundPartners.size() > this->m_boundPartners.size())
	{
		return false;
	}

	if(!(flags & agents::MatchOptions::IgnoreBonds)
		&& pattern.BoundPartners.size() == 0
		&& pattern.IsWildCardBound == false
		&& this->m_boundPartners.size() > 0)
	{
		return false;
	}

	std::unordered_map<std::string, bool> ignore;
	for(std::size_t i = 0; i < pattern.ChildAgents.size(); ++i)
	{
		Agent* outptr = nullptr;
		if(!this->FindSubAgent(pattern.ChildAgents[i], outptr, ignore))
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

	if(flags & agents::MatchOptions::IgnoreBonds)
	{
		// This agent is being matched without considering bonds
	}
	else
	{
		ignore.clear();
		for(std::size_t i = 0; i < pattern.BoundPartners.size(); ++i)
		{
			Agent* outptr = nullptr;
			if(!this->FindBoundPartner(pattern.BoundPartners[i], outptr, ignore))
			{
				return false;
			}
			ignore[outptr->GetID()] = true;
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

const bool Agent::FindChildAgent(
	const AgentPattern& pattern,
	std::shared_ptr<Agent>& outptr,
	std::unordered_map<std::string, bool> ignore) const
{
	outptr = nullptr;
	for(std::size_t i = 0; i < this->m_childAgents.size(); ++i)
	{
		if(ignore[this->m_childAgents[i]->GetID()])
			continue;

		if(this->m_childAgents[i]->Matches(pattern))
		{
			outptr = this->m_childAgents[i];
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
	unsigned char options = agents::MatchOptions::IgnoreBonds;
	outptr = nullptr;
	for(std::size_t i = 0; i < this->m_boundPartners.size(); ++i)
	{
		if(ignore[this->m_boundPartners[i]->GetID()])
			continue;

		if(this->m_boundPartners[i]->Matches(pattern, options))
		{
			outptr = this->m_boundPartners[i].get();
			break;
		}
	}

	return outptr != nullptr;
}

bool Agent::CopyState(AgentPattern& oldState, AgentPattern& newState)
{
	std::size_t ap_child_count = oldState.ChildAgents.size();
	std::size_t this_child_count = this->m_childAgents.size();

	if(ap_child_count == 0 && this->Matches(oldState))
	{
		this->m_agentState = newState.State;
		return true;
	}

	if(ap_child_count > this_child_count)
	{
		PRINT_ERROR("Agent::CopyStateChanges: this agent pattern has more levels that this agent.\n")
	}

	Agent* outptr = nullptr;
	for(std::size_t i = 0; i < oldState.ChildAgents.size(); ++i)
	{
		if(this->FindSubAgent(oldState.ChildAgents[i],outptr))
		{
			if(!outptr->CopyState(oldState.ChildAgents[i], newState.ChildAgents[i]))
			{
				PRINT_ERROR("Agent::CopyStateChanges: a match could not be found for a child agent while copying state.\n")
				return false;
			}
		}
	}

	return true;
}

const int Agent::GetSubTreeDepth() const
{
	if(this->m_childAgents.size() == 0)
	{
		return 0;
	}

	return this->m_childAgents[0]->GetSubTreeDepth() + 1;
}

void Agent::UpdateParentTransform(Eigen::Matrix4d parentTransform)
{
	this->m_parentTransform = parentTransform;
	for(std::size_t i = 0; i < this->m_childAgents.size(); ++i)
	{
		this->m_childAgents[i]->UpdateParentTransform(this->GetGlobalTransform());
	}
}

} // namespace agentsim
} // namespace aics
