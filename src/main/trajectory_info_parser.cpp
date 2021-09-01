#include "simularium/fileio/trajectory_info_parser.h"
#include "simularium/fileio/trajectory_info_v1.h"
#include "loguru/loguru.hpp"

namespace aics {
namespace simularium {
namespace fileio {

std::shared_ptr<TrajectoryInfo> TrajectoryInfoParser::Parse(Json::Value& root) {
    std::string version = root["version"].asString();
    int major_version = std::atoi(version.substr(version.find('.')).c_str());

    std::shared_ptr<TrajectoryInfo> tfi;
    switch(major_version) {
      case 1: {
          tfi = std::make_shared<TrajectoryFileInfoV1>();
          tfi->ParseJSON(root);
        break;
      }
      default:
      LOG_F(ERROR, "Unrecognized version of trajectory info: version %i", major_version);
    }

    return tfi;
}

} // namespace fileio
} // namespace simularium
} // namespace aics
