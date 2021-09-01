#ifndef AICS_TRAJECTORY_INFO_H
#define AICS_TRAJECTORY_INFO_H

#include <json/json.h>

namespace aics {
namespace simularium {
namespace fileio {
class TrajectoryInfo {
public:
  virtual void ParseJSON(Json::Value& root) = 0;
  virtual Json::Value GetJSON() = 0;
private:

};

} // namespace fileio
} // namespace simularium
} // namespace aics

#endif // AICS_TRAJECTORY_INFO_H
