#include "simularium/fileio/trajectory_info_v1.h"

namespace aics {
namespace simularium {
namespace fileio {

void TrajectoryFileInfoV1::ParseJSON(Json::Value& jsonRoot) {

}

Json::Value TrajectoryFileInfoV1::GetJSON() {
  Json::Value out;
  out["fileName"] = this->m_filename;
  out["totalSteps"] = this->m_totalSteps;
  out["timeStepSize"] = this->m_timeStepSize;
  out["spatialUnitFactorMeters"] = this->m_spatialUnitFactorMeters;

  Json::Value size;
  size["x"] = this->m_size.x;
  size["y"] = this->m_size.y;
  size["z"] = this->m_size.z;
  out["size"] = size;

  Json::Value typeMapping;
  for (auto& entry :this->m_typeMapping) {
      std::string id = std::to_string(entry.first);
      std::string name = entry.second;

      Json::Value newEntry;
      newEntry["name"] = name;

      typeMapping[id] = newEntry;
  }
  out["typeMapping"] = typeMapping;

  Json::Value cameraDefault;
  Json::Value camPos, camLook, camUp;
  camPos["x"] = this->m_cameraDefault.position[0];
  camPos["y"] = this->m_cameraDefault.position[1];
  camPos["z"] = this->m_cameraDefault.position[2];

  camLook["x"] = this->m_cameraDefault.lookAtPoint[0];
  camLook["y"] = this->m_cameraDefault.lookAtPoint[1];
  camLook["z"] = this->m_cameraDefault.lookAtPoint[2];

  camUp["x"] = this->m_cameraDefault.upVector[0];
  camUp["y"] = this->m_cameraDefault.upVector[1];
  camUp["z"] = this->m_cameraDefault.upVector[2];

  cameraDefault["position"] = camPos;
  cameraDefault["lookAtPoint"] = camLook;
  cameraDefault["upVector"] = camUp;
  cameraDefault["fovDegrees"] = this->m_cameraDefault.fovDegrees;

  out["cameraDefault"] = cameraDefault;
}

} // namespace fileio
} // namespace simularium
} // namespace aics
