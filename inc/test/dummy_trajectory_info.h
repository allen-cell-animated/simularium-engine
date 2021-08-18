#ifndef AICS_DUMMY_TRAJECTORY_INFO
#define AICS_DUMMY_TRAJECTORY_INFO

#include <string>
namespace aics {
namespace simularium {
namespace test {

static std::string v1_trajectory_info = R""""(
  {
    "filename": "dummy_test_data_v1",
    "version": "1",
    "totalSteps": 100,
    "timeStepSize": 1,
    "spatialUnitFactorMeters": 1e-9,
    "size": {
      "x": 100,
      "y": 100,
      "z": 100
    },
    "CameraPosition": {
      "position": {
        "x": 1,
        "y": 1,
        "z": 1
      },
      "lookAtPoint": {
        "x": 0,
        "y": 0,
        "z": 0
      },
      "upVector": {
        "x": 0,
        "y": 1,
        "z": 0
      },
      "fovDegrees": 90
    }
  }
)"""";

static std::string v2_trajectory_info = R""""(
  {
    "filename": "dummy_test_data_v2",
    "version": "2",
    "totalSteps": 100,
    "timeStepSize": 1,
    "timeUnits": {
      "magnitude": 1e-9,
      "name": "nano"
    },
    "spatialUnits": {
      "magnitude": 1e-9,
      "name": "nano"
    },
    "size": {
      "x": 100,
      "y": 100,
      "z": 100
    },
    "CameraPosition": {
      "position": {
        "x": 1,
        "y": 1,
        "z": 1
      },
      "lookAtPoint": {
        "x": 0,
        "y": 0,
        "z": 0
      },
      "upVector": {
        "x": 0,
        "y": 1,
        "z": 0
      },
      "fovDegrees": 90
    }
  }
)"""";

} // namespace test
} // namespace simularium
} // namespace aics

#endif // AICS_DUMMY_TRAJECTORY_INFO
