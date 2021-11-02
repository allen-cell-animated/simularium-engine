TrajectoryFileProperties parse_traj_info_v1(Json::Value fprops) {
  TrajectoryFileProperties tfp;

  const Json::Value typeMapping = fprops["typeMapping"];
  std::vector<std::string> ids = typeMapping.getMemberNames();
  for (auto& id : ids) {
      std::size_t idKey = std::atoi(id.c_str());
      const Json::Value entry = typeMapping[id];
      tfp.typeMapping[idKey] = entry["name"].asString();
  }

  const Json::Value& size = fprops["size"];
  tfp.boxX = size["x"].asFloat();
  tfp.boxY = size["y"].asFloat();
  tfp.boxZ = size["z"].asFloat();

  tfp.fileName = fprops["fileName"].asString();
  tfp.numberOfFrames = fprops["totalSteps"].asInt();
  tfp.timeStepSize = fprops["timeStepSize"].asFloat();
  tfp.spatialUnitFactorMeters = fprops["spatialUnitFactorMeters"].asFloat();
  // Optional Fields
  const Json::Value& cameraDefault = fprops["cameraDefault"];
  if (cameraDefault != Json::nullValue) {
      const Json::Value& cpos = cameraDefault["position"];
      if (cpos != Json::nullValue) {
          tfp.cameraDefault.position[0] = cpos["x"].asFloat();
          tfp.cameraDefault.position[1] = cpos["y"].asFloat();
          tfp.cameraDefault.position[2] = cpos["z"].asFloat();
      }
      const Json::Value& lookAt = cameraDefault["lookAtPoint"];
      if (lookAt != Json::nullValue) {
          tfp.cameraDefault.lookAtPoint[0] = lookAt["x"].asFloat();
          tfp.cameraDefault.lookAtPoint[1] = lookAt["y"].asFloat();
          tfp.cameraDefault.lookAtPoint[2] = lookAt["z"].asFloat();
      }
      const Json::Value& upVec = cameraDefault["upVector"];
      if (upVec != Json::nullValue) {
          tfp.cameraDefault.upVector[0] = upVec["x"].asFloat();
          tfp.cameraDefault.upVector[1] = upVec["y"].asFloat();
          tfp.cameraDefault.upVector[2] = upVec["z"].asFloat();
      }

      if (cameraDefault.isMember("fovDegrees")) {
          tfp.cameraDefault.fovDegrees = cameraDefault["fovDegrees"].asFloat();
      }

      return tfp;
    }
}

TrajectoryFileProperties parse_trajectory_info_json(Json::Value trajInfo) {
    int version = trajInfo["version"].asInt();
    switch(version) {
      case 1: {
        return parse_traj_info_v1(trajInfo);
        break;
      }
      default: {
        LOG_F(WARNING, "Unrecognized version %i, defaulting to version 1", version);
      }
    }

    return parse_traj_info_v1(trajInfo);
}
