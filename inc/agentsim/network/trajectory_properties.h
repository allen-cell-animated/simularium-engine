#ifndef AICS_TRAJECTORY_PROPERTIES_H
#define AICS_TRAJECTORY_PROPERTIES_H

#include <string>
#include <unordered_map>

namespace aics {
namespace agentsim {

struct TrajectoryFileProperties{
    std::string fileName = "";
    std::size_t numberOfFrames = 0;
    double timeStepSize = 100;
    std::unordered_map<std::size_t, std::string> typeMapping;

    std::string Str() {
        return "TrajectoryFileProperties | File Name " + this->fileName +
            " | Number of Frames " + std::to_string(this->numberOfFrames) +
            " | TimeStep Size " + std::to_string(this->timeStepSize);
    }
};

} // namespace agentsim
} // namespace aics

#endif // AICS_TRAJECTORY_PROPERTIES_H
