#include "simularium/fileio/trajectory_info_v1.h"

namespace aics {
namespace simularium {
namespace fileio {

void TrajectoryFileInfoV1::ParseJSON(Json::Value& fprops) {
    // Required Fields
    const Json::Value typeMapping = fprops["typeMapping"];
    std::vector<std::string> ids = typeMapping.getMemberNames();
    for (auto& id : ids) {
        std::size_t idKey = std::atoi(id.c_str());
        const Json::Value entry = typeMapping[id];
        this->m_typeMapping[idKey] = entry["name"].asString();
    }

    const Json::Value& size = fprops["size"];
    this->m_size.x = size["x"].asFloat();
    this->m_size.y = size["y"].asFloat();
    this->m_size.z = size["z"].asFloat();

    this->m_filename = fprops["fileName"].asString();
    this->m_totalSteps = fprops["totalSteps"].asInt();
    this->m_timeStepSize = fprops["timeStepSize"].asFloat();
    this->m_spatialUnitFactorMeters = fprops["spatialUnitFactorMeters"].asFloat();

    // Optional Fields
    const Json::Value& cameraDefault = fprops["cameraDefault"];
    if (cameraDefault != Json::nullValue) {
        const Json::Value& cpos = cameraDefault["position"];
        if (cpos != Json::nullValue) {
            this->m_cameraDefault.position[0] = cpos["x"].asFloat();
            this->m_cameraDefault.position[1] = cpos["y"].asFloat();
            this->m_cameraDefault.position[2] = cpos["z"].asFloat();
        }
        const Json::Value& lookAt = cameraDefault["lookAtPoint"];
        if (lookAt != Json::nullValue) {
            this->m_cameraDefault.lookAtPoint[0] = lookAt["x"].asFloat();
            this->m_cameraDefault.lookAtPoint[1] = lookAt["y"].asFloat();
            this->m_cameraDefault.lookAtPoint[2] = lookAt["z"].asFloat();
        }
        const Json::Value& upVec = cameraDefault["upVector"];
        if (upVec != Json::nullValue) {
            this->m_cameraDefault.upVector[0] = upVec["x"].asFloat();
            this->m_cameraDefault.upVector[1] = upVec["y"].asFloat();
            this->m_cameraDefault.upVector[2] = upVec["z"].asFloat();
        }

        if (cameraDefault.isMember("fovDegrees")) {
            this->m_cameraDefault.fovDegrees = cameraDefault["fovDegrees"].asFloat();
        }
    }
}

void TrajectoryFileInfoV1::UpdateFromJSON(Json::Value& update) {
  if(update.isMember("fileName")) {
    this->m_filename = update["fileName"].asString();
  }

  if(update.isMember("totalSteps")) {
    this->m_totalSteps = update["totalSteps"].asInt();
  }

  if(update.isMember("timeStepSize")) {
    this->m_timeStepSize = update["timeStepSize"].asFloat();
  }
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
  return out;
}

} // namespace fileio
} // namespace simularium
} // namespace aics
