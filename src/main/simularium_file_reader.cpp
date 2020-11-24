#include "simularium/fileio/simularium_file_reader.h"
#include "loguru/loguru.hpp"
#include <algorithm>

namespace aics {
namespace simularium {
namespace fileio {

    bool SimulariumFileReader::DeserializeFrame(
      Json::Value& jsonRoot,
      std::size_t frameNumber,
      TrajectoryFrame& outFrame
    ) {
      AgentDataFrame adf;
      Json::Value& spatialData = jsonRoot["spatialData"];
      std::size_t start = spatialData["bundleStart"].asInt();
      std::size_t size = spatialData["bundleSize"].asInt();
      std::size_t frame = std::max(std::min(start + size, frameNumber), start);
      Json::Value::ArrayIndex index = (Json::Value::ArrayIndex)(frame - start);

      Json::Value& bundleData = spatialData["bundleData"];
      Json::Value& frameJson = bundleData[index];
      Json::Value& frameData = frameJson["data"];

      for(Json::Value::ArrayIndex i = 0; i != frameData.size();) {
          AgentData ad;
          ad.vis_type = frameData[i++].asInt();
          ad.id = frameData[i++].asInt();
          ad.type = frameData[i++].asInt();
          ad.x = frameData[i++].asFloat();
          ad.y = frameData[i++].asFloat();
          ad.z = frameData[i++].asFloat();
          ad.xrot = frameData[i++].asFloat();
          ad.yrot = frameData[i++].asFloat();
          ad.zrot = frameData[i++].asFloat();
          ad.collision_radius = frameData[i++].asFloat();

          std::size_t num_sp = frameData[i++].asInt();
          if(num_sp + i > frameData.size()) {
            LOG_F(WARNING, "At index %i, %zu subpoints, %i entries in data buffer", i, num_sp, frameData.size());
            LOG_F(ERROR, "Parsing error, incorrect data packing or misalignment in simularium JSON");
            return false;
          }

          for (std::size_t j = 0; j < num_sp; j++) {
            ad.subpoints.push_back(frameData[i++].asFloat());
          }

          adf.push_back(ad);
      }

      outFrame.data = adf;
      outFrame.time = frameJson["time"].asFloat();
      outFrame.frameNumber = frameNumber;

      return true;
    }

} // namespace fileio
} // namespace simularium
} // namespace aics
