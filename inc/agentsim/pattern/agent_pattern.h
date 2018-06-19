#ifndef AICS_AGENT_PATTERN_H
#define AICS_AGENT_PATTERN_H

#include <string>
#include <vector>

namespace aics {
namespace agentsim {

/**
*	Agent Pattern
*
*	An agent pattern is a set of traits matched to an agent
*	this object is used as a lightweight definition of agents
*	that need to be found or identified
*/
struct AgentPattern
{
public:
	std::string Name = "";
	std::string State = "";
	std::vector<AgentPattern> ChildAgents;
	std::vector<AgentPattern> BoundPartners;
	bool IsWildCardBound = false;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_AGENT_PATTERN_H
