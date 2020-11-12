#ifndef AICS_MATH_UTIL_H
#define AICS_MATH_UTIL_H

#include <Eigen/Dense>
#include <Eigen/Geometry>
#include <vector>

namespace aics {
namespace simularium {
namespace mathutil {
    
    Eigen::Matrix3d eulerToMatrix( 
        Eigen::Vector3d rotation
    );

    Eigen::Matrix3d GetRotationMatrix(
        std::vector<Eigen::Vector3d> basisPositions
    );
    
    Eigen::Matrix3d getRandomOrientation();
    
    Eigen::Matrix3d getErrorOrientation(
        float errValue
    );
    
    Eigen::Vector3d getRandomPerpendicularVector(
        Eigen::Vector3d vector
    );

} // namespace mathutil
} // namespace simularium
} // namespace aics

#endif // AICS_MATH_UTIL_H
