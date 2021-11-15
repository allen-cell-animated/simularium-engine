#include "simularium/network/tfp_to_json.h"
#include "simularium/network/net_message_ids.h"

namespace aics {
namespace simularium {

Json::Value tfp_to_json(TrajectoryFileProperties tfp) {
  Json::Value fprops;
  fprops["version"] = 3;
  fprops["msgType"] = WebRequestTypes::id_trajectory_file_info;
  fprops["totalSteps"] = tfp.numberOfFrames;
  fprops["timeStepSize"] = tfp.timeStepSize;

  Json::Value timeUnits;
  timeUnits["magnitude"] = tfp.timeUnits.magnitude;
  timeUnits["name"] = tfp.timeUnits.name;
  fprops["timeUnits"] = timeUnits;

  Json::Value spatialUnits;
  spatialUnits["magnitude"] = tfp.spatialUnits.magnitude;
  spatialUnits["name"] = tfp.spatialUnits.name;
  fprops["spatialUnits"] = spatialUnits;

  Json::Value typeMapping;
  for (auto entry : tfp.typeMapping) {
      std::string id = std::to_string(entry.first);
      std::string name = entry.second.name;

      Json::Value typeEntry;
      typeEntry["name"] = name;

      Json::Value geometry;
      geometry["displayType"] = entry.second.geometry.displayType;
      geometry["url"] = entry.second.geometry.url;
      geometry["color"] = entry.second.geometry.color;
      typeEntry["geometry"] = geometry;

      typeMapping[id] = typeEntry;
  }

  Json::Value size;
  size["x"] = tfp.boxX;
  size["y"] = tfp.boxY;
  size["z"] = tfp.boxZ;

  fprops["typeMapping"] = typeMapping;
  fprops["size"] = size;

  Json::Value cameraDefault;
  Json::Value camPos, camLook, upVec;

  camPos["x"] = tfp.cameraDefault.position[0];
  camPos["y"] = tfp.cameraDefault.position[1];
  camPos["z"] = tfp.cameraDefault.position[2];

  camLook["x"] = tfp.cameraDefault.lookAtPoint[0];
  camLook["y"] = tfp.cameraDefault.lookAtPoint[1];
  camLook["z"] = tfp.cameraDefault.lookAtPoint[2];

  upVec["x"] = tfp.cameraDefault.upVector[0];
  upVec["y"] = tfp.cameraDefault.upVector[1];
  upVec["z"] = tfp.cameraDefault.upVector[2];

  cameraDefault["position"] = camPos;
  cameraDefault["lookAtPosition"] = camLook;
  cameraDefault["upVector"] = upVec;
  cameraDefault["fovDegrees"] = tfp.cameraDefault.fovDegrees;
  fprops["cameraDefault"] = cameraDefault;

  return fprops;
}

} // namespace simularium
} // namespace aics
