TrajectoryFileProperties parse_traj_info_v1(Json::Value fprops)
{
    TrajectoryFileProperties tfp;

    const Json::Value typeMapping = fprops["typeMapping"];
    std::vector<std::string> ids = typeMapping.getMemberNames();
    for (auto& id : ids) {
        std::size_t idKey = std::atoi(id.c_str());
        const Json::Value entry = typeMapping[id];
        TypeEntry newEntry;
        newEntry.name = entry["name"].asString();

        tfp.typeMapping[idKey] = newEntry;
    }

    const Json::Value& size = fprops["size"];
    tfp.boxX = size["x"].asFloat();
    tfp.boxY = size["y"].asFloat();
    tfp.boxZ = size["z"].asFloat();

    tfp.fileName = fprops["fileName"].asString();
    tfp.numberOfFrames = fprops["totalSteps"].asInt();
    tfp.timeStepSize = fprops["timeStepSize"].asFloat();

    // @TODO (if needed): Translate spatial unit factor to space units
    //tfp.spatialUnitFactorMeters = fprops["spatialUnitFactorMeters"].asFloat();

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
    }

    return tfp;
}

TrajectoryFileProperties parse_traj_info_v2(Json::Value fprops)
{
    TrajectoryFileProperties tfp;

    const Json::Value typeMapping = fprops["typeMapping"];
    std::vector<std::string> ids = typeMapping.getMemberNames();
    for (auto& id : ids) {
        std::size_t idKey = std::atoi(id.c_str());
        const Json::Value entry = typeMapping[id];
        TypeEntry newEntry;
        newEntry.name = entry["name"].asString();

        tfp.typeMapping[idKey] = newEntry;
    }

    const Json::Value& size = fprops["size"];
    tfp.boxX = size["x"].asFloat();
    tfp.boxY = size["y"].asFloat();
    tfp.boxZ = size["z"].asFloat();

    tfp.fileName = fprops["fileName"].asString();
    tfp.numberOfFrames = fprops["totalSteps"].asInt();
    tfp.timeStepSize = fprops["timeStepSize"].asFloat();

    const Json::Value& timeUnits = fprops["timeUnits"];
    tfp.timeUnits.magnitude = timeUnits["magnitude"].asFloat();
    tfp.timeUnits.name = timeUnits["name"].asString();

    const Json::Value& spatialUnits = fprops["spatialUnits"];
    tfp.spatialUnits.magnitude = spatialUnits["magnitude"].asFloat();
    tfp.spatialUnits.name = spatialUnits["name"].asString();

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
    }

    return tfp;
}

TrajectoryFileProperties parse_traj_info_v3(Json::Value fprops)
{
    TrajectoryFileProperties tfp;

    const Json::Value typeMapping = fprops["typeMapping"];
    std::vector<std::string> ids = typeMapping.getMemberNames();
    for (auto& id : ids) {
        std::size_t idKey = std::atoi(id.c_str());
        const Json::Value entry = typeMapping[id];
        TypeEntry newEntry;
        newEntry.name = entry["name"].asString();

        const Json::Value geom = entry["geometry"];
        if (geom != Json::nullValue) {
            newEntry.geometry.displayType = geom["displayType"].asString();
            newEntry.geometry.url = geom["url"].asString();
            newEntry.geometry.color = geom["color"].asString();
        }

        tfp.typeMapping[idKey] = newEntry;
    }

    const Json::Value& size = fprops["size"];
    tfp.boxX = size["x"].asFloat();
    tfp.boxY = size["y"].asFloat();
    tfp.boxZ = size["z"].asFloat();

    tfp.fileName = fprops["fileName"].asString();
    tfp.numberOfFrames = fprops["totalSteps"].asInt();
    tfp.timeStepSize = fprops["timeStepSize"].asFloat();

    const Json::Value& timeUnits = fprops["timeUnits"];
    tfp.timeUnits.magnitude = timeUnits["magnitude"].asFloat();
    tfp.timeUnits.name = timeUnits["name"].asString();

    const Json::Value& spatialUnits = fprops["spatialUnits"];
    tfp.spatialUnits.magnitude = spatialUnits["magnitude"].asFloat();
    tfp.spatialUnits.name = spatialUnits["name"].asString();

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
    }

    return tfp;
}

TrajectoryFileProperties parse_trajectory_info_json(Json::Value trajInfo)
{
    int version = trajInfo["version"].asInt();

    switch (version) {
    case 3: {
        return parse_traj_info_v3(trajInfo);
        break;
    }
    case 2: {
        return parse_traj_info_v2(trajInfo);
        break;
    }
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
