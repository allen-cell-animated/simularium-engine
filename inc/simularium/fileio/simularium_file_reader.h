#ifndef AICS_SIMULARIUM_FILE_READER_H
#define AICS_SIMULARIUM_FILE_READER_H

#include "simularium/agent_data.h"
#include <json/json.h>

namespace aics {
namespace simularium {
    namespace fileio {

        class SimulariumFileReader {
        public:
            bool DeserializeFrame(
                Json::Value& jsonRoot,
                std::size_t frameNumber,
                TrajectoryFrame& outFrame);
        };

    } // namespace fileio
} // namespace simularium
} // namespace aics

#endif // AICS_SIMULARIUM_FILE_READER_H
