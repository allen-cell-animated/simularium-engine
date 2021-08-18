#include "simularium/simularium.h"
#include "simularium/fileio/trajectory_info_v1.h"
#include "test/dummy_trajectory_info.h"
#include "gtest/gtest.h"
#include <cstdio>

namespace aics {
namespace simularium {
    namespace test {
        class TrajectoryInfoV1Tests : public ::testing::Test {
        };

        TEST_F(TrajectoryInfoV1Tests, BasicTest)
        {
          std::cout << test::v1_trajectory_info << std::endl;
        }

    } // namespace test
} // namespace simularium
} // namespace aics
