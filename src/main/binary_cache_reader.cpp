#include "simularium/fileio/binary_cache_reader.h"

namespace aics {
namespace simularium {
namespace fileio {

bool BinaryCacheReader::GotoFrameNumber(
  std::ifstream& is,
  std::size_t frame_number
) {
  std::string line;
  is.seekg(0, is.beg); // go to beginning
  for (std::size_t i = 0; i < frame_number; ++i) {
      if (std::getline(is, line)) {
          line = "";
      } else {
          return false;
      }
  }

  std::getline(is, line, this->m_delimiter[0]);
  return true;
}

bool BinaryCacheReader::DeserializeFrame(
  std::ifstream& is,
  std::size_t frame_number,
  AgentDataFrame& adf
) {
    std::string line;
    if (!this->GotoFrameNumber(is, frame_number)) {
        return false;
    }

    // Get the number of agents in this data frame
    std::getline(is, line, this->m_delimiter[0]);
    std::size_t num_agents = std::stoi(line);

    for (std::size_t i = 0; i < num_agents; ++i) {
        aics::simularium::AgentData ad;
        if (this->DeserializeAgent(is, ad)) {
            adf.push_back(ad);
        }
    }

    return true;
}

bool BinaryCacheReader::DeserializeAgent(
  std::ifstream& is,
  AgentData& ad
) {
    std::string line;
    std::vector<float> vals;
    for (std::size_t i = 0; i < 11; ++i) {
        if (std::getline(is, line, this->m_delimiter[0])) {
            vals.push_back(std::atof(line.c_str()));
        } else {
            return false;
        }
    }

    ad.vis_type = vals[0];
    ad.id = vals[1];
    ad.type = vals[2];
    ad.x = vals[3];
    ad.y = vals[4];
    ad.z = vals[5];
    ad.xrot = vals[6];
    ad.yrot = vals[7];
    ad.zrot = vals[8];
    ad.collision_radius = vals[9];

    std::size_t num_sp = vals[10];
    for (std::size_t i = 0; i < num_sp; ++i) {
        if (std::getline(is, line, this->m_delimiter[0])) {
            ad.subpoints.push_back(std::atof(line.c_str()));
        } else {
            return false;
        }
    }

    return true;
}

} // namespace fileio
} // namespace simularium
} // namespace aics
