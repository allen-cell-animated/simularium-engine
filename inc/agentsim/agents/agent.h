#ifndef AICS_AGENT_H
#define AICS_AGENT_H

#include <string>
#include <vector>
#include "Eigen/Dense"

namespace aics {
namespace agentsim {

class AgentPattern;

class Agent
{
public:
	Agent();

	void SetLocation(Eigen::Vector3d newLocation) { m_location = newLocation; }
	void SetRotation(Eigen::Vector3d newRotation) { m_rotation = newRotation; }
	void SetInteractionDistance(float newDistance) { m_interaction_distance = newDistance; }
	void SetCollisionRadius(float newRadius) { m_collision_radius = newRadius; }
	void SetMass(float mass) { m_mass = mass; }
	void SetDiffusionCoefficient(float dc) { m_diffusion_coefficient = dc; }
	void SetName(std::string name) {m_agentName = name; }

	const Eigen::Vector3d GetLocation() const { return m_location; }
	const Eigen::Vector3d GetRotation() const { return m_rotation; }
	const float GetInteractionDistance() { return m_interaction_distance; }
	const float GetCollisionRadius() { return m_collision_radius; }
	const float GetMass() { return m_mass; }
	const float GetDiffusionCoefficient() { return m_diffusion_coefficient; }
	const std::string GetName() { return m_agentName; }

	void AddBoundPartner(Agent* other);
	void AddChildAgent(Agent* other);

	bool CanInteractWith(const Agent& other);
	bool IsCollidingWith(const Agent& other);

	const bool Matches(const AgentPattern& pattern) const;
	const bool FindChild(const AgentPattern& pattern, Agent*& outptr) const;

private:
	Eigen::Vector3d m_location;
	Eigen::Vector3d m_rotation;

	float m_diffusion_coefficient = 5.0f;
	float m_mass = 1.0f;
	float m_interaction_distance = 100.f;
	float m_collision_radius = 50.f;
	std::string m_agentID = "";
	std::string m_agentName = "";

	std::vector<Agent*> m_boundPartners;
	std::vector<Agent*> m_childAgents;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_AGENT_H
