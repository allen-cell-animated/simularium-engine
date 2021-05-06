#ifndef AICS_TRAJECTORY_PROPERTIES_H
#define AICS_TRAJECTORY_PROPERTIES_H

#include <string>
#include <unordered_map>

namespace aics {
namespace simularium {

    struct TrajectoryFileProperties {
        std::string fileName = "";
        std::size_t numberOfFrames = 0;
        double timeStepSize = 100;
        float spatialUnitFactorMeters = 1e-9;
        std::unordered_map<std::size_t, std::string> typeMapping;
        float boxX, boxY, boxZ;

        std::string Str()
        {
            return "TrajectoryFileProperties | File Name " + this->fileName + " | Number of Frames " + std::to_string(this->numberOfFrames) + " | TimeStep Size " + std::to_string(this->timeStepSize) + " | spatialUnitFactor " + std::to_string(this->spatialUnitFactorMeters) + " | Box Size [" + std::to_string(boxX) + "," + std::to_string(boxY) + "," + std::to_string(boxZ) + "]";
        }
    };

} // namespace simularium
} // namespace aics

#endif // AICS_TRAJECTORY_PROPERTIES_H
