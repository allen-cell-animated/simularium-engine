#include "agentsim/fileio/binary_cache_writer.h"

namespace aics {
namespace agentsim {
namespace fileio {

void BinaryCacheWriter::SerializeFrame(
  std::ofstream& os,
  std::size_t frame_number,
  AgentDataFrame& adf
) {
  os << "F" << frame_number << this->m_delimiter
     << adf.size() << this->m_delimiter;
  for (aics::agentsim::AgentData ad : adf) {
      this->SerializeAgent(os, ad);
  }
  os << std::endl;
}

void BinaryCacheWriter::SerializeAgent(
  std::ofstream& os,
  AgentData& ad
) {
    auto vals = aics::agentsim::Serialize(ad);
    for (auto val : vals) {
        os << val << this->m_delimiter;
    }
}

} // namespace fileio
} // namespace agentsim
} // namespace aics
