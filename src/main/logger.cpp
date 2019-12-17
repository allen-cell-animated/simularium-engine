#include "agentsim/util/logger.h"
#include "loguru/loguru.hpp"
#include <iostream>

/*
    Encountered difficulty with incorporating FMT based loggers
    FMT appears to either modify global state, or otherwise
    clash in deallocation with any version of FMT included in
    dependencies.

    Naturally, many of the simulation packages include
    thier own loggers, some of which (for C++ packages)
    are likely to rely on FMT.

    This is why an unobstrusive logging library was chosen
    and a restricted api (strings only, no formatting) was
    implemented; to minimize dependency clashing amongst
    logging libraries
*/

namespace aics {
namespace agentsim {
namespace logger {

    void Fatal(std::string msg) {
        LOG_F(FATAL, msg.c_str());
    }

    void Error(std::string msg) {
        LOG_F(ERROR, msg.c_str());
    }

    void Warn(std::string msg) {
        LOG_F(WARNING, msg.c_str());
    }

    void Info(std::string msg) {
        LOG_F(INFO, msg.c_str());
    }

    void SetThreadName(std::string name) {
        loguru::set_thread_name(name.c_str());
    }

} // namespace logger
} // namespace agentsim
} // namespace aics
