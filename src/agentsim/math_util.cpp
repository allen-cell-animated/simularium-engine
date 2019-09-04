#include "agentsim/util/math_util.h"

namespace aics {
namespace agentsim {
namespace mathutil {

    void OrthonormalizeMatrix(Eigen::Matrix3d& m)
    {
        auto v1 = m.col(0).normalized();
        auto v2 = m.col(1).normalized();
        auto v3 = m.col(2).normalized();

        v2 = (v2 - v2.dot(v1) * v1).normalized();
        v3 = (v3 - v3.dot(v1) * v1).normalized();
        v3 = (v3 - v3.dot(v2) * v2).normalized();

        m << v1[0], v1[1], v1[2],
            v2[0], v2[1], v2[2],
            v3[0], v3[1], v3[2];
    }

    Eigen::Matrix3d GetRotationMatrix(
        std::vector<Eigen::Vector3d> basisPositions)
    {
        std::vector<Eigen::Vector3d> basisVectors = std::vector<Eigen::Vector3d>(3);
        basisVectors[0] = (basisPositions[1] - basisPositions[0]).normalized();
        basisVectors[1] = (basisPositions[2] - basisPositions[0]).normalized();
        basisVectors[2] = (basisPositions[3] - basisPositions[0]).normalized();

        Eigen::Matrix3d rotation_matrix;
        rotation_matrix << basisVectors[0][0], basisVectors[0][1], basisVectors[0][2],
            basisVectors[1][0], basisVectors[1][1], basisVectors[1][2],
            basisVectors[2][0], basisVectors[2][1], basisVectors[2][2];

        OrthonormalizeMatrix(rotation_matrix);
        return rotation_matrix;
    }

} // namespace mathutil
} // namespace agentsim
} // namespace aics
