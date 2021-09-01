#ifndef AICS_TRAJECTORY_INFO_H
#define AICS_TRAJECTORY_INFO_H

#include <json/json.h>

namespace aics {
namespace simularium {

class TrajectoryInfo {
public:
  virtual void ParseJSON(Json::Value& root) = 0;
private:

};

} // namespace simularium
} // namespace aics

#endif // AICS_TRAJECTORY_INFO_H
