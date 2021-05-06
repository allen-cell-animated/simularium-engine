#ifndef AICS_SIMULARIUM_BINARY_FILE_H
#define AICS_SIMULARIUM_BINARY_FILE_H

#include "simularium/agent_data.h"
#include <fstream>
#include <string>

namespace aics {
namespace simularium {
    typedef std::vector<float> BroadcastDataBuffer;

    struct BroadcastUpdate {
        int new_pos;
        BroadcastDataBuffer buffer;
    };

    namespace fileio {
        namespace binary {
            const char eof[20] = {
                '\\', 'E', 'O', 'F',
                'T', 'H', 'E',
                'F', 'R', 'A', 'M', 'E',
                'E', 'N', 'D', 'S',
                'H', 'E', 'R', 'E'
            };

            static const int HEADER_SIZE = 16;
            static const int TOC_ENTRY_COUNT_OFFSET = HEADER_SIZE;
            static const int TOC_ENTRY_START_OFFSET = HEADER_SIZE + 4;

        }

        class SimulariumBinaryFile {
        public:
            void Create(std::string filePath, std::size_t numFrames = 10000);
            void Open(std::string filePath);
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
