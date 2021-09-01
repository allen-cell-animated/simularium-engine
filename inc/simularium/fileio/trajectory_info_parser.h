#ifndef AICS_TRAJECTORY_INFO_PARSER_H
#define AICS_TRAJECTORY_INFO_PARSER_H

#include "simularium/fileio/trajectory_info.h"
#include <json/json.h>
#include <memory>

namespace aics {
namespace simularium {
namespace fileio {
  class TrajectoryInfoParser {
  public:
    std::shared_ptr<TrajectoryInfo> Parse(Json::Value& root);
  private:

  };

} // namespace fileio
} // namespace simularium
} // namespace aics

#endif // AICS_TRAJECTORY_INFO_PARSER_H
