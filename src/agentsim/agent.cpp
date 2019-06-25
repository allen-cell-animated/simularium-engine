#include "agentsim/agents/agent.h"
#include <stdlib.h>
#include <time.h>

namespace aics {
namespace agentsim {

    namespace agents {
        void GenerateLocalUUID(std::string& uuid)
        {
            // Doesn't matter if time is seeded or not at this point
            //  just need relativley unique local identifiers for runtime
            char strUuid[128];
            sprintf(strUuid, "%x%x-%x-%x-%x-%x%x%x",
                rand(), rand(), // Generates a 64-bit Hex number
                rand(), // Generates a 32-bit Hex number
                ((rand() & 0x0fff) | 0x4000), // Generates a 32-bit Hex number of the form 4xxx (4 indicates the UUID version)
                rand() % 0x3fff + 0x8000, // Generates a 32-bit Hex number in the range [0x8000, 0xbfff]
                rand(), rand(), rand()); // Generates a 96-bit Hex number

            uuid = strUuid;
        }
    } // namespace agents

    namespace math_util {
        Eigen::Affine3d CreateRotationMatrix(double ax, double ay, double az)
        {
            Eigen::Affine3d rx = Eigen::Affine3d(Eigen::AngleAxisd(ax, Eigen::Vector3d(1, 0, 0)));
            Eigen::Affine3d ry = Eigen::Affine3d(Eigen::AngleAxisd(ay, Eigen::Vector3d(0, 1, 0)));
            Eigen::Affine3d rz = Eigen::Affine3d(Eigen::AngleAxisd(az, Eigen::Vector3d(0, 0, 1)));
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
        if (other->m_agentID == this->m_agentID) {
            printf("Agent.cpp: An Agent cannot be bound to itself.\n");
            return false;
        }

        this->m_boundPartners.push_back(other);
        return true;
    }

    bool Agent::AddChildAgent(std::shared_ptr<Agent> other)
    {
        if (other->m_agentID == this->m_agentID) {
            printf("Agent.cpp: An Agent cannot be parented to itself.\n");
            return false;
        }

        this->m_childAgents.push_back(other);
        other->UpdateParentTransform(this->GetGlobalTransform());
        return true;
    }

    std::shared_ptr<Agent> Agent::GetChildAgent(std::size_t index)
    {
        if (index < 0 || index > this->m_childAgents.size()) {
            return nullptr;
        }

        return this->m_childAgents[index];
    }

    std::shared_ptr<Agent> Agent::GetBoundPartner(std::size_t index)
    {
        if (index < 0 || index > this->m_boundPartners.size()) {
            return nullptr;
        }

        return this->m_boundPartners[index];
    }

    bool Agent::CanInteractWith(const Agent& other)
    {
        if (this->m_agentID == other.m_agentID) {
            return false;
        }

        float interaction_dist_squared = (this->m_interaction_distance + other.m_interaction_distance) * (this->m_interaction_distance + other.m_interaction_distance);

        float dist_squared = (this->m_location - other.m_location).squaredNorm();

        return dist_squared <= interaction_dist_squared;
    }

    bool Agent::IsCollidingWith(const Agent& other)
    {
        float dist_squared = (this->m_location - other.m_location).squaredNorm();
        float coll_dist_squared = (this->m_collision_radius + other.m_collision_radius) * (this->m_collision_radius + other.m_collision_radius);

        return dist_squared <= coll_dist_squared;
    }

    void Agent::AddTag(std::string tag)
    {
        if (std::find(this->m_tags.begin(), this->m_tags.end(), tag) != this->m_tags.end()) {
            return;
        }

        this->m_tags.push_back(tag);
    }

    const bool Agent::HasTag(std::string tag)
    {
        return std::find(this->m_tags.begin(), this->m_tags.end(), tag) != this->m_tags.end();
    }

    const void Agent::PrintDbg() const
    {
        printf("[Agent] %s\n", this->m_agentName.c_str());

        for (std::size_t i = 0; i < this->m_boundPartners.size(); ++i) {
            printf("[bound] %s\n", this->m_boundPartners[i]->GetName().c_str());
        }

        for (std::size_t i = 0; i < this->m_childAgents.size(); ++i) {
            printf("[child] %s\n", this->m_childAgents[i]->GetName().c_str());
        }
    }

    void Agent::UpdateParentTransform(Eigen::Matrix4d parentTransform)
    {
        this->m_parentTransform = parentTransform;
        for (std::size_t i = 0; i < this->m_childAgents.size(); ++i) {
            this->m_childAgents[i]->UpdateParentTransform(this->GetGlobalTransform());
        }
    }

} // namespace agentsim
} // namespace aics
