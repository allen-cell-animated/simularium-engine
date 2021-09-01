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

        // A basic test to ensure parsing doesn't crash
        TEST_F(TrajectoryInfoV1Tests, BasicTest)
        {
          Json::Value fprops;
          Json::Reader reader;
          bool parseSuccess = reader.parse(v1_trajectory_info, fprops);
          EXPECT_EQ(parseSuccess, true);

          aics::simularium::fileio::TrajectoryFileInfoV1 v1;
          v1.ParseJSON(fprops);

          Json::Value out = v1.GetJSON();
          std::cout << out << std::endl;
        }
    } // namespace test
} // namespace simularium
} // namespace aics
