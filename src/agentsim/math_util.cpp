#include "agentsim/util/math_util.h"
#include <iostream>

namespace aics {
namespace agentsim {
namespace mathutil {
    
    Eigen::Matrix3d GetRotationMatrix(
        std::vector<Eigen::Vector3d> basisPositions)
    {
        auto v1 = (basisPositions[0] - basisPositions[1]).normalized();
        auto v2 = (basisPositions[2] - basisPositions[1]).normalized();
        v2 = (v2 - v2.dot(v1) / v1.dot(v1) * v1).normalized();
        auto v3 = v2.cross(v1);
        
//        std::cout << "[" << v1[0] << ", " << v1[1] << ", " << v1[2] << "], [" 
//                  << "[" << v2[0] << ", " << v2[1] << ", " << v2[2] << "], [" 
//                  << "[" << v3[0] << ", " << v3[1] << ", " << v3[2] << "]" << std::endl;
        
        Eigen::Matrix3d m;
        m << v2[0], v2[1], v2[2],
             v1[0], v1[1], v1[2],
             v3[0], v3[1], v3[2];
        
        return m;
    }

} // namespace mathutil
} // namespace agentsim
} // namespace aics
