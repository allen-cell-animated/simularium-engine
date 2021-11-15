#ifndef AICS_TFP_TO_JSON
#define AICS_TFP_TO_JSON

#include "simularium/network/trajectory_properties.h"
#include <json/json.h>

namespace aics {
namespace simularium {

Json::Value tfp_to_json(TrajectoryFileProperties tfp);

} // namespace simularium
} // namespace aics

#endif // AICS_TFP_TO_JSON
