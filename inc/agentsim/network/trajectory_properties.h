#ifndef AICS_TRAJECTORY_PROPERTIES_H
#define AICS_TRAJECTORY_PROPERTIES_H

#include <string>

namespace aics {
namespace agentsim {

struct TrajectoryFileProperties{
    std::string fileName = "";
    std::size_t numberOfFrames = 0;
    double timeStepSize = 100;
};

} // namespace agentsim
} // namespace aics

#endif // AICS_TRAJECTORY_PROPERTIES_H
