#ifndef AICS_SIMULARIUM_BINARY_FILE_H
#define AICS_SIMULARIUM_BINARY_FILE_H

#include <string>
#include <fstream>
#include "simularium/agent_data.h"

namespace aics {
namespace simularium {
typedef std::vector<float> BroadcastDataBuffer;

struct BroadcastUpdate {
    int new_pos;
    BroadcastDataBuffer buffer;
};

namespace fileio {
namespace binary {
  const unsigned char eof[4] = {'\\', 'e', 'o', 'f'};
}

class SimulariumBinaryFile {
  public:
    void Create(std::string filePath, std::size_t numFrames=10000);
    void WriteFrame(TrajectoryFrame tf);
    BroadcastUpdate GetBroadcastFrame(std::size_t frameNumber);
    BroadcastUpdate GetBroadcastUpdate(std::size_t currentPos, std::size_t bufferSize);

    std::size_t NumSavedFrames();
    std::size_t GetEndOfFilePos();
    std::size_t GetFramePos(std::size_t frameNumber);

  private:
    void WriteHeader();
    void AllocateTOC(std::size_t size);

    std::fstream m_fstream;
    std::size_t m_endTOC;
};

} // namespace fileio
} // namespace simularium
} // namespace aics

#endif // AICS_SIMULARIUM_BINARY_FILE_H
