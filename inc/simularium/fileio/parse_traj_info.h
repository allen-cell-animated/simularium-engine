#ifndef AICS_PARSE_TRAJ_INFO_H
#define AICS_PARSE_TRAJ_INFO_H

#include "simularium/network/trajectory_properties.h"
#include <json/json.h>

namespace aics {
namespace simularium {

    TrajectoryFileProperties parse_trajectory_info_json(Json::Value trajInfo);

} // namespace simularium
} // namespace aics

#endif // AICS_PARSE_TRAJ_INFO_H
