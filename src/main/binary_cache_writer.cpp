#include "simularium/fileio/binary_cache_writer.h"

namespace aics {
namespace simularium {
namespace fileio {

void BinaryCacheWriter::SerializeFrame(
  std::ofstream& os,
  std::size_t frame_number,
  AgentDataFrame& adf
) {
  os << "F" << frame_number << this->m_delimiter
     << adf.size() << this->m_delimiter;
  for (aics::simularium::AgentData ad : adf) {
      this->SerializeAgent(os, ad);
  }
  os << std::endl;
}

void BinaryCacheWriter::SerializeAgent(
  std::ofstream& os,
  AgentData& ad
) {
    auto vals = aics::simularium::Serialize(ad);
    for (auto val : vals) {
        os << val << this->m_delimiter;
    }
}

} // namespace fileio
} // namespace simularium
} // namespace aics
