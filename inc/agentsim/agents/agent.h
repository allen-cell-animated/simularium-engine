#ifndef AGENT_H
#define AGENT_H

#include <vector>
#include "Eigen/Dense"

namespace aics {
namespace agentsim {

class Agent
{
public:
		Agent()
		{
				m_location << 0, 0, 0;
				m_rotation << 0, 0, 0;
		}

		void SetLocation(Eigen::Vector3d newLocation) { m_location = newLocation; }

		const Eigen::Vector3d GetLocation() const { return m_location; }
		const Eigen::Vector3d GetRotation() const { return m_rotation; }

private:
		Eigen::Vector3d m_location;
		Eigen::Vector3d m_rotation;

		float interaction_radius = 100.f;
		int m_agentID = -1;

		std::vector<Agent*> m_boundPartners;
};

} // namespace agentsim
} // namespace aics

#endif // AGENT_H
