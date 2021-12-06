#include "simularium/fileio/parse_traj_info.h"
#include "simularium/network/tfp_to_json.h"
#include "test/network/dummy_trajinfo_data.h"
#include "gtest/gtest.h"
#include <cstdio>
#include <sstream>

/**
* This is used in unit tests, and must correspond
* to the latest version of the traj-info specification
* also, all the below must be functionally equivalent
**/
#define LATEST_DUMMY_TRAJ_INFO v3_trajectory_info

namespace aics {
namespace simularium {
    namespace test {
        class TrajInfoTests : public ::testing::Test {
        protected:
          void SetUp() override {
            Json::Value dummyData;
            std::istringstream sstr(LATEST_DUMMY_TRAJ_INFO);
            sstr >> dummyData;
            auto tfp = parse_trajectory_info_json(dummyData);
            comparisonJSON = tfp_to_json(tfp);
          }

        public:
          Json::Value comparisonJSON;
        };

        TEST_F(TrajInfoTests, ParseV1)
        {
          Json::Value dummyData;
          std::istringstream sstr(v1_trajectory_info);
          sstr >> dummyData;
          TrajectoryFileProperties tfp = parse_trajectory_info_json(dummyData);
          auto outJson = tfp_to_json(tfp);
          EXPECT_EQ(outJson, comparisonJSON);
        }

    } // namespace test
} // namespace simularium
} // namespace aics
