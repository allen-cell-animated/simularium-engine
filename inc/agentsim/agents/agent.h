#ifndef AICS_AGENT_H
#define AICS_AGENT_H

#include "Eigen/Dense"
#include "Eigen/Geometry"
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

/*
*	Agent type flags for visualization
*	defined starting at 1000 to allow simPkgs to start identifying agent ids from 0 w/o issue
*	only 'first-class' visualization objects need to be defined here
*/
enum kVisType {
    vis_type_default = 1000,
    vis_type_fiber = 1001
};

namespace aics {
namespace agentsim {

    class Agent {
    public:
        Agent();

        void SetLocation(Eigen::Vector3d newLocation);
        void SetRotation(Eigen::Vector3d newRotation);
        void SetCollisionRadius(float newRadius) { m_collision_radius = newRadius; }
        void SetName(std::string name) { m_agentName = name; }
        void SetState(std::string state) { m_agentState = state; }
        void SetTypeID(unsigned int typeID) { m_typeId = typeID; }

        const Eigen::Matrix4d GetTransform();
        const Eigen::Vector3d GetLocation() const { return m_location; }
        const Eigen::Vector3d GetRotation() const { return m_rotation; }
        const float GetInteractionDistance() { return m_interaction_distance; }
        const float GetCollisionRadius() { return m_collision_radius; }
        const std::string GetName() const { return m_agentName; }
        const std::string GetState() const { return m_agentState; }
        const std::string GetID() { return m_agentID; }
        const unsigned int GetTypeID() { return m_typeId; }

        void SetVisibility(bool visibility) { this->m_visibility = visibility; }
        const bool IsVisible() { return this->m_visibility; }

        void AddSubPoint(Eigen::Vector3d newPoint) { m_subPoints.push_back(newPoint); }
        void UpdateSubPoint(std::size_t index, Eigen::Vector3d updatedPoint)
        {
            if (index < 0 || index > this->m_subPoints.size())
                return;
            if (index == this->m_subPoints.size()) {
                AddSubPoint(updatedPoint);
            }

            this->m_subPoints[index] = updatedPoint;
        }
        bool HasSubPoints() { return this->m_subPoints.size() > 0; }
        std::size_t GetNumSubPoints() { return this->m_subPoints.size(); }
        Eigen::Vector3d GetSubPoint(std::size_t index) { return this->m_subPoints[index]; }

        kVisType GetVisType() { return this->m_visType; }
        void SetVisType(kVisType newType) { this->m_visType = newType; }

    private:
        Eigen::Vector3d m_location;
        Eigen::Vector3d m_rotation;

        float m_interaction_distance = 100.f;
        float m_collision_radius = 50.f;
        unsigned int m_typeId = 0;
        std::string m_agentID = "";
        std::string m_agentName = "";
        std::string m_agentState = "";
        bool m_visibility = true;

        std::vector<Eigen::Vector3d> m_subPoints;
        kVisType m_visType = kVisType::vis_type_default;
    };

} // namespace agentsim
} // namespace aics

#endif // AICS_AGENT_H
