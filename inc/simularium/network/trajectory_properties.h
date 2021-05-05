#ifndef AICS_TRAJECTORY_PROPERTIES_H
#define AICS_TRAJECTORY_PROPERTIES_H

#include <string>
#include <array>
#include <unordered_map>

namespace aics {
namespace simularium {

  struct CameraPosition {
    CameraPosition() {
      position = { 0, 0, 120 };
      lookAt = { 0, 0, 0 };
      upVector = { 0, 1, 0 };
      fov = 50;
    }
    
    std::array<float, 3> position;
    std::array<float, 3> lookAt;
    std::array<float, 3> upVector;
    float fov;
  };
  
struct TrajectoryFileProperties{
    std::string fileName = "";
    std::size_t numberOfFrames = 0;
    double timeStepSize = 100;
    float spatialUnitFactorMeters = 1e-9;
    std::unordered_map<std::size_t, std::string> typeMapping;
    float boxX, boxY, boxZ;
    CameraPosition cameraDefault;

        std::string Str()
        {
            return "TrajectoryFileProperties | File Name " + this->fileName + " | Number of Frames " + std::to_string(this->numberOfFrames) + " | TimeStep Size " + std::to_string(this->timeStepSize) + " | spatialUnitFactor " + std::to_string(this->spatialUnitFactorMeters) + " | Box Size [" + std::to_string(boxX) + "," + std::to_string(boxY) + "," + std::to_string(boxZ) + "]";
        }
    };

} // namespace simularium
} // namespace aics

#endif // AICS_TRAJECTORY_PROPERTIES_H
