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
		void SetMass(double mass) { m_mass = mass; }
		void SetDiffusionCoefficient(double dc) { m_diffusion_coefficient = dc; }

		const Eigen::Vector3d GetLocation() const { return m_location; }
		const Eigen::Vector3d GetRotation() const { return m_rotation; }
		const double GetInteractionDistance() { return m_interaction_distance; }
		const double GetCollisionRadius() { return m_collision_radius; }
		const double GetMass() { return m_mass; }
		const double GetDiffusionCoefficient() { return m_diffusion_coefficient; }

		void AddBoundPartner(Agent* other);

		bool CanInteractWith(const Agent& other);
		bool IsCollidingWith(const Agent& other);

private:
		Eigen::Vector3d m_location;
		Eigen::Vector3d m_rotation;

		double m_diffusion_coefficient = 5.0f;
		double m_mass = 1.0f;
		double m_interaction_distance = 100.f;
		double m_collision_radius = 50.f;
		std::string m_agentID = "";

		std::vector<Agent*> m_boundPartners;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_AGENT_H
