#include "agentsim/util/logger.h"
#include "loguru/loguru.hpp"
#include <iostream>

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
