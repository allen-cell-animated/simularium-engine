#ifndef AICS_AGENT_H
#define AICS_AGENT_H

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

        void SetLocation(float x, float y, float z);
        void SetRotation(float xrot, float yrot, float zrot);
        void SetCollisionRadius(float newRadius) { m_collision_radius = newRadius; }
        void SetName(std::string name) { m_agentName = name; }
        void SetState(std::string state) { m_agentState = state; }
        void SetTypeID(unsigned int typeID) { m_typeId = typeID; }

        const std::vector<float> GetLocation() const { return { m_x, m_y, m_z }; }
        const std::vector<float> GetRotation() const { return { m_xrot, m_yrot, m_zrot }; }
        const float GetCollisionRadius() { return m_collision_radius; }
        const std::string GetName() const { return m_agentName; }
        const std::string GetState() const { return m_agentState; }
        const std::string GetID() { return m_agentID; }
        const unsigned int GetTypeID() { return m_typeId; }

        void SetVisibility(bool visibility) { this->m_visibility = visibility; }
        const bool IsVisible() { return this->m_visibility; }

        void AddSubPoint(float x, float y, float z);
        void UpdateSubPoint(std::size_t index, float x, float y, float z);
        bool HasSubPoints();

        std::size_t GetNumSubPoints();
        std::vector<float> GetSubPoint(std::size_t index);

        kVisType GetVisType() { return this->m_visType; }
        void SetVisType(kVisType newType) { this->m_visType = newType; }

        void SetUid(float uid) { this->m_uid = uid; }
        const float GetUid() const { return this->m_uid; }

    private:
        float m_x, m_y, m_z;
        float m_xrot, m_yrot, m_zrot;
        float m_uid;

        float m_collision_radius = 50.f;
        unsigned int m_typeId = 0;
        std::string m_agentID = "";
        std::string m_agentName = "";
        std::string m_agentState = "";
        bool m_visibility = true;

        std::vector<float> m_subPoints;
        kVisType m_visType = kVisType::vis_type_default;
    };

} // namespace agentsim
} // namespace aics

#endif // AICS_AGENT_H
