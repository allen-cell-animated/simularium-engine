#ifndef AICS_TRAJECTORY_INFO_V1_H
#define AICS_TRAJECTORY_INFO_V1_H

#include "simularium/fileio/trajectory_info.h"
#include <array>
#include <string>
#include <unordered_map>
#include <json/json.h>

namespace aics {
namespace simularium {
namespace fileio {
    struct BoxSize {
      float x,y,z;
    };

    struct CameraPosition {
        CameraPosition()
        {
            position = { 0, 0, 120 };
            lookAtPoint = { 0, 0, 0 };
            upVector = { 0, 1, 0 };
            fovDegrees = 50;
        }

        std::array<float, 3> position;
        std::array<float, 3> lookAtPoint;
        std::array<float, 3> upVector;
        float fovDegrees;
    };

    class TrajectoryFileInfoV1: public TrajectoryInfo {
    public:
        virtual void ParseJSON(Json::Value& jsonRoot) override;
        virtual void UpdateFromJSON(Json::Value& update) override;
        virtual Json::Value GetJSON() override;

    private:
        std::string m_filename = "";
        std::size_t m_totalSteps = 0;
        double m_timeStepSize = 100;
        float m_spatialUnitFactorMeters = 1e-9;
        std::unordered_map<std::size_t, std::string> m_typeMapping;
        BoxSize m_size;
        CameraPosition m_cameraDefault;
    };

} // namespace fileio
} // namespace simularium
} // namespace aics

#endif // AICS_TRAJECTORY_INFO_V1_H
