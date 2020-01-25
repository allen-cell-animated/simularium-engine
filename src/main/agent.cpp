#include "agentsim/agents/agent.h"
#include <stdlib.h>
#include <time.h>
#include "Eigen/Dense"
#include "Eigen/Geometry"

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

        agents::GenerateLocalUUID(this->m_agentID);
    }

    void Agent::SetLocation(Eigen::Vector3d newLocation)
    {
        this->m_location = newLocation;
    }

    void Agent::SetRotation(Eigen::Vector3d newRotation)
    {
        this->m_rotation = newRotation;
    }

    const Eigen::Matrix4d Agent::GetTransform()
    {
        return math_util::CreateTransform(this->m_location, this->m_rotation);
    }
    
} // namespace agentsim
} // namespace aics
