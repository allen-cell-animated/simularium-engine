#include "simularium/fileio/parse_traj_info.h"
#include "loguru/loguru.hpp"

namespace aics {
namespace simularium {
    void parse_metadata_v1(
        TrajectoryFileProperties& out,
        const Json::Value& root)
    {
        out.fileName = root["fileName"].asString();
        out.numberOfFrames = root["totalSteps"].asInt();
        out.timeStepSize = root["timeStepSize"].asFloat();
    }

    void parse_v1_type_mapping(
        TrajectoryFileProperties& out,
        const Json::Value& root)
    {
        const Json::Value typeMapping = root["typeMapping"];
        std::vector<std::string> ids = typeMapping.getMemberNames();
        for (auto& id : ids) {
            std::size_t idKey = std::atoi(id.c_str());
            const Json::Value entry = typeMapping[id];
            TypeEntry newEntry;
            newEntry.name = entry["name"].asString();

            out.typeMapping[idKey] = newEntry;
        }
    }

    void parse_v1_box_size(
        TrajectoryFileProperties& out,
        const Json::Value& root)
    {
        const Json::Value& size = root["size"];
        out.boxX = size["x"].asFloat();
        out.boxY = size["y"].asFloat();
        out.boxZ = size["z"].asFloat();
    }

    void parse_optional_v1_camera_default(
        TrajectoryFileProperties& out,
        const Json::Value& root)
    {
        const Json::Value& cameraDefault = root["cameraDefault"];
        if (root != Json::nullValue) {
            const Json::Value& cpos = cameraDefault["position"];
            if (cpos != Json::nullValue) {
                out.cameraDefault.position[0] = cpos["x"].asFloat();
                out.cameraDefault.position[1] = cpos["y"].asFloat();
                out.cameraDefault.position[2] = cpos["z"].asFloat();
            }
            const Json::Value& lookAt = cameraDefault["lookAtPoint"];
            if (lookAt != Json::nullValue) {
                out.cameraDefault.lookAtPoint[0] = lookAt["x"].asFloat();
                out.cameraDefault.lookAtPoint[1] = lookAt["y"].asFloat();
                out.cameraDefault.lookAtPoint[2] = lookAt["z"].asFloat();
            }
            const Json::Value& upVec = cameraDefault["upVector"];
            if (upVec != Json::nullValue) {
                out.cameraDefault.upVector[0] = upVec["x"].asFloat();
                out.cameraDefault.upVector[1] = upVec["y"].asFloat();
                out.cameraDefault.upVector[2] = upVec["z"].asFloat();
            }

            if (cameraDefault.isMember("fovDegrees")) {
                out.cameraDefault.fovDegrees = cameraDefault["fovDegrees"].asFloat();
            }
        }
    }

    void parse_optional_time_units_v2(
        TrajectoryFileProperties& out,
        const Json::Value& root)
    {
        const Json::Value& timeUnits = root["timeUnits"];
        if (timeUnits != Json::nullValue) {
            out.timeUnits.magnitude = timeUnits["magnitude"].asFloat();
            out.timeUnits.name = timeUnits["name"].asString();
        }
    }

    void parse_optional_space_units_v2(
        TrajectoryFileProperties& out,
        const Json::Value& root)
    {
        const Json::Value& spatialUnits = root["spatialUnits"];
        if (spatialUnits != Json::nullValue) {
            out.spatialUnits.magnitude = spatialUnits["magnitude"].asFloat();
            out.spatialUnits.name = spatialUnits["name"].asString();
        }
    }

    void parse_v2_typemapping(
        TrajectoryFileProperties& out,
        const Json::Value& root)
    {
        const Json::Value typeMapping = root["typeMapping"];
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

            out.typeMapping[idKey] = newEntry;
        }
    }

    TrajectoryFileProperties parse_traj_info_v1(Json::Value fprops)
    {
        TrajectoryFileProperties tfp;

        parse_v1_type_mapping(tfp, fprops);
        parse_v1_box_size(tfp, fprops);
        parse_metadata_v1(tfp, fprops);

        tfp.spatialUnits.magnitude = 1;
        tfp.spatialUnits.name = "nm";

        tfp.timeUnits.magnitude = 1;
        tfp.timeUnits.name = "ns";

        parse_optional_v1_camera_default(tfp, fprops);

        return tfp;
    }

    TrajectoryFileProperties parse_traj_info_v2(Json::Value fprops)
    {
        TrajectoryFileProperties tfp;

        parse_v1_type_mapping(tfp, fprops);
        parse_v1_box_size(tfp, fprops);
        parse_metadata_v1(tfp, fprops);

        parse_optional_time_units_v2(tfp, fprops);
        parse_optional_space_units_v2(tfp, fprops);

        parse_optional_v1_camera_default(tfp, fprops);

        return tfp;
    }

    TrajectoryFileProperties parse_traj_info_v3(Json::Value fprops)
    {
        TrajectoryFileProperties tfp;

        parse_v2_typemapping(tfp, fprops);
        parse_v1_box_size(tfp, fprops);
        parse_metadata_v1(tfp, fprops);

        parse_optional_time_units_v2(tfp, fprops);
        parse_optional_space_units_v2(tfp, fprops);

        parse_optional_v1_camera_default(tfp, fprops);

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
            LOG_F(WARNING, "Unrecognized version %i, defaulting to version 3", version);
        }
        }

        return parse_traj_info_v3(trajInfo);
    }

} // namespace simularium
} // namespace aics
