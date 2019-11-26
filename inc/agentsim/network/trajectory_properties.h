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
};

} // namespace agentsim
} // namespace aics

#endif // AICS_TRAJECTORY_PROPERTIES_H
