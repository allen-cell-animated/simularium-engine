#ifndef AICS_LOGGER_H
#define AICS_LOGGER_H

#include <string>

namespace aics {
namespace agentsim {
namespace logger {
    void Fatal(std::string msg);
    void Error(std::string msg);
    void Warn(std::string msg);
    void Info(std::string msg);
} // namespace logger
} // namespace agentsim
} // namespace aics

#endif // AICS_LOGGER_H
