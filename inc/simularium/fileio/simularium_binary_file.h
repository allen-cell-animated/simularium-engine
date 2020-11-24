#ifndef AICS_SIMULARIUM_BINARY_FILE_H
#define AICS_SIMULARIUM_BINARY_FILE_H

#include <string>
#include <fstream>
#include "simularium/agent_data.h"

namespace aics {
namespace simularium {

class SimulariumBinaryFile {
  public:
    void Create(std::string filePath, std::size_t numFrames=10000);
    void WriteFrame(TrajectoryFrame tf);

  private:
    void WriteHeader();
    void AllocateTOC(std::size_t size);

    std::fstream m_fstream;
};

} // namespace simularium
} // namespace aics

#endif // AICS_SIMULARIUM_BINARY_FILE_H
