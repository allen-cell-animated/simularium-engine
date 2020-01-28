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

    void Agent::AddSubPoint(float point[3]) {
        this->m_subPoints.push_back(point[0]);
        this->m_subPoints.push_back(point[1]);
        this->m_subPoints.push_back(point[2]);
    }

    void Agent::UpdateSubPoint(std::size_t index, float point[3]) {
        if (index < 0 || index > this->m_subPoints.size() / 3)
            return;
        if (index == this->m_subPoints.size()) {
            AddSubPoint(point);
        }

        this->m_subPoints[index * 3] = point[0];
        this->m_subPoints[index * 3 + 1] = point[1];
        this->m_subPoints[index * 3 + 2] = point[2];
    }

    std::vector<float> Agent::GetSubPoint(std::size_t index) {
        auto first = this->m_subPoints.begin() + index * 3;
        auto last = this->m_subPoints.begin() + index * 3 + 2;
        return std::vector<float>(first, last);
    }

} // namespace agentsim
} // namespace aics
