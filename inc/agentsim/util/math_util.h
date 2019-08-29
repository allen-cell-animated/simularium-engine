#ifndef AICS_MATH_UTIL_H
#define AICS_MATH_UTIL_H

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <vector>

namespace aics {
namespace agentsim {
namespace mathutil {

    void OrthonormalizeMatrix(Eigen::Matrix3d& m);

    Eigen::Matrix3d GetRotationMatrix(
        std::vector<Eigen::Vector3d> basisPositions);

} // namespace mathutil
} // namespace agentsim
} // namespace aics

#endif // AICS_MATH_UTIL_H
