#include "agentsim/agents/agent.h"
#include <stdlib.h>
#include <time.h>
#include <iostream>

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

    Agent::Agent()
    {
        this->m_x = 0;
        this->m_y = 0;
        this->m_z = 0;

        this->m_xrot = 0;
        this->m_yrot = 0;
        this->m_zrot = 0;

        agents::GenerateLocalUUID(this->m_agentID);
    }

    void Agent::SetLocation(float x, float y, float z)
    {
        this->m_x = x;
        this->m_y = y;
        this->m_z = z;
    }

    void Agent::SetRotation(float xrot, float yrot, float zrot)
    {
        this->m_xrot = xrot;
        this->m_yrot = yrot;
        this->m_zrot = zrot;
    }


    std::size_t Agent::GetNumSubPoints()
    {
        return this->m_subPoints.size() / 3;
    }

    bool Agent::HasSubPoints() { return this->m_subPoints.size(); }

    void Agent::AddSubPoint(float x, float y, float z) {
        this->m_subPoints.push_back(x);
        this->m_subPoints.push_back(y);
        this->m_subPoints.push_back(z);
    }

    void Agent::UpdateSubPoint(std::size_t index, float x, float y, float z) {
        std::size_t nbSubPoints = this->GetNumSubPoints();
        if (index < 0 || index > nbSubPoints) {
            return;
        } else if (index < nbSubPoints) {
            this->m_subPoints[index * 3] = x;
            this->m_subPoints[index * 3 + 1] = y;
            this->m_subPoints[index * 3 + 2] = z;
        } else if (index == nbSubPoints) {
            AddSubPoint(x,y,z);
        }
    }

    std::vector<float> Agent::GetSubPoint(std::size_t index) {
        auto first = this->m_subPoints.begin() + index * 3;
        auto last = this->m_subPoints.begin() + index * 3 + 3;
        return std::vector<float>(first, last);
    }

} // namespace agentsim
} // namespace aics
