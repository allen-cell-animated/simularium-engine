#include "simularium/util/math_util.h"
#include "loguru/loguru.hpp"
#include <iostream>

namespace aics {
namespace simularium {
namespace mathutil {

    Eigen::Matrix3d eulerToMatrix(
        Eigen::Vector3d rotation)
    {
        Eigen::AngleAxisd rollAngle(rotation[0], Eigen::Vector3d::UnitX());
        Eigen::AngleAxisd pitchAngle(rotation[1], Eigen::Vector3d::UnitY());
        Eigen::AngleAxisd yawAngle(rotation[2], Eigen::Vector3d::UnitZ());

        Eigen::Quaterniond q = yawAngle * pitchAngle * rollAngle;
        return q.matrix();
    }

    /**
    *	given three positions (p0, p1, p2), construct an orthonormal rotation basis
    *   for the particle at p1 so that:
    *      - X basis vector points to p2
    *      - Y basis vector points to p0
    *      - Z basis vector is cross product of X and Y
    **/
    Eigen::Matrix3d GetRotationMatrix(
        std::vector<Eigen::Vector3d> basisPositions)
    {
        auto v1 = (basisPositions[2] - basisPositions[1]).normalized();
        auto v2 = (basisPositions[0] - basisPositions[1]).normalized();
        v1 = (v1 - v1.dot(v2) * v2).normalized();
        auto v3 = v1.cross(v2);

        Eigen::Matrix3d m;
        m << v1[0], v1[1], v1[2],
             v2[0], v2[1], v2[2],
             v3[0], v3[1], v3[2];

        return m;
    }

    Eigen::Matrix3d getRandomOrientation()
    {
        Eigen::Vector3d rotation;
        rotation[0] = (rand() % 8) * 45;
        rotation[1] = (rand() % 8) * 45;
        rotation[2] = (rand() % 8) * 45;
        return eulerToMatrix(rotation);
    }

    Eigen::Matrix3d getErrorOrientation(
        float errValue)
    {
        // Assign an error orientation that can be checked to know if the rotation algorithm failed
        Eigen::Vector3d rotation;
        rotation[0] = errValue;
        rotation[1] = errValue;
        rotation[2] = errValue;
        return eulerToMatrix(rotation);
    }

    Eigen::Vector3d getRandomPerpendicularVector(
        Eigen::Vector3d vector
    )
    {
        if (vector[0] == 0 && vector[1] == 0)
        {
            if (vector[2] == 0)
            {
                LOG_F(ERROR, "Failed to get perpendicular vector to zero vector!");
                return Eigen::Vector3d(0, 0, 0);
            }
            return Eigen::Vector3d(0, 1, 0);
        }

        auto u = Eigen::Vector3d(-vector[1], vector[0], 0);
        u.normalize();
        
//        // TODO
//        vector.normalize();
//        auto angle = float(rand() % 360) * float(M_PI) / float(180);
//        std::cout << "random angle = " << angle << std::endl;
//        auto r = Eigen::AngleAxisd(angle, vector).toRotationMatrix();
        
        return u;// r * u;
    }

} // namespace mathutil
} // namespace simularium
} // namespace aics
