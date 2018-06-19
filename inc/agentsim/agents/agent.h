#ifndef AICS_AGENT_H
#define AICS_AGENT_H

#include <string>
#include <vector>
#include "Eigen/Dense"

namespace aics {
namespace agentsim {

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

		const Eigen::Vector3d GetLocation() const { return m_location; }
		const Eigen::Vector3d GetRotation() const { return m_rotation; }
		const float GetInteractionDistance() { return m_interaction_distance; }
		const float GetCollisionRadius() { return m_collision_radius; }
		const float GetMass() { return m_mass; }
		const float GetDiffusionCoefficient() { return m_diffusion_coefficient; }

		void AddBoundPartner(Agent* other);

		bool CanInteractWith(const Agent& other);
		bool IsCollidingWith(const Agent& other);

private:
		Eigen::Vector3d m_location;
		Eigen::Vector3d m_rotation;

		float m_diffusion_coefficient = 5.0f;
		float m_mass = 1.0f;
		float m_interaction_distance = 100.f;
		float m_collision_radius = 50.f;
		std::string m_agentID = "";

		std::vector<Agent*> m_boundPartners;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_AGENT_H
