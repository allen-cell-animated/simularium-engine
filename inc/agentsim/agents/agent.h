#ifndef AICS_AGENT_H
#define AICS_AGENT_H

#include <string>
#include <vector>
#include <memory>
#include <unordered_map>
#include "Eigen/Dense"
#include "Eigen/Geometry"

namespace aics {
namespace agentsim {

class AgentPattern;

class Agent
{
public:
	Agent();

	void SetLocation(Eigen::Vector3d newLocation);
	void SetRotation(Eigen::Vector3d newRotation);
	void SetInteractionDistance(float newDistance) { m_interaction_distance = newDistance; }
	void SetCollisionRadius(float newRadius) { m_collision_radius = newRadius; }
	void SetMass(float mass) { m_mass = mass; }
	void SetDiffusionCoefficient(float dc) { m_diffusion_coefficient = dc; }
	void SetName(std::string name) { m_agentName = name; }
	void SetState(std::string state) { m_agentState = state; }

	const Eigen::Matrix4d GetGlobalTransform();
	const Eigen::Matrix4d GetTransform();
	const Eigen::Vector3d GetLocation() const { return m_location; }
	const Eigen::Vector3d GetRotation() const { return m_rotation; }
	const float GetInteractionDistance() { return m_interaction_distance; }
	const float GetCollisionRadius() { return m_collision_radius; }
	const float GetMass() { return m_mass; }
	const float GetDiffusionCoefficient() { return m_diffusion_coefficient; }
	const std::string GetName() const { return m_agentName; }
	const std::string GetState() const { return m_agentState; }
	const std::string GetID() { return m_agentID; }

	bool AddBoundPartner(std::shared_ptr<Agent> other);
	bool AddChildAgent(std::shared_ptr<Agent> other);

	bool CanInteractWith(const Agent& other);
	bool IsCollidingWith(const Agent& other);

	const bool FindSubAgent(const AgentPattern& pattern, Agent*& outptr);
	const bool Matches(const AgentPattern& pattern) const;
	const bool FindChildAgent(
		const AgentPattern& pattern,
		Agent*& outptr,
		std::unordered_map<std::string, bool> ignore = std::unordered_map<std::string, bool>()) const;
	const bool FindBoundPartner(
		const AgentPattern& pattern,
		Agent*& outptr,
		std::unordered_map<std::string, bool> ignore = std::unordered_map<std::string, bool>()) const;
	bool CopyState(AgentPattern& oldState, AgentPattern& newState);

private:
	void UpdateParentTransform(Eigen::Matrix4d parentTransform);
	Eigen::Vector3d m_location;
	Eigen::Vector3d m_rotation;
	Eigen::Affine3d m_localLocation;
	Eigen::Affine3d m_localRotation;
	Eigen::Matrix4d m_parentTransform;

	float m_diffusion_coefficient = 5.0f;
	float m_mass = 1.0f;
	float m_interaction_distance = 100.f;
	float m_collision_radius = 50.f;
	std::string m_agentID = "";
	std::string m_agentName = "";
	std::string m_agentState = "";

	std::vector<std::shared_ptr<Agent>> m_boundPartners;
	std::vector<std::shared_ptr<Agent>> m_childAgents;

};

} // namespace agentsim
} // namespace aics

#endif // AICS_AGENT_H
