#include "simularium/fileio/trajectory_info_parser.h"
#include "simularium/fileio/trajectory_info_v1.h"
#include "loguru/loguru.hpp"
#include <iostream>

namespace aics {
namespace simularium {
namespace fileio {

std::shared_ptr<TrajectoryInfo> TrajectoryInfoParser::Parse(Json::Value& root) {
    int version = root["version"].asInt();

    std::shared_ptr<TrajectoryInfo> tfi;
    switch(version) {
      case 1: {
          tfi = std::make_shared<TrajectoryFileInfoV1>();
          tfi->ParseJSON(root);
        break;
      }
      case 2: {
          tfi = std::make_shared<TrajectoryFileInfoV1>();
          tfi->ParseJSON(root);
          break;
      }
      default:
      LOG_F(ERROR, "Unrecognized version of trajectory info: version %i", version);
    }

    return tfi;
}

} // namespace fileio
} // namespace simularium
} // namespace aics
