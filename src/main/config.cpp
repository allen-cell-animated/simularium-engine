#include "simularium/config/config.h"

namespace aics {
namespace simularium {
    namespace config {
        std::string GetCacheFolder() { return "/tmp/aics/simularium/"; }
        std::string GetS3Location() { return "trajectory/"; }
        std::string GetS3CacheLocation() { return "cache/trajectory/" + GetEnvironment() + "/"; }
        std::string GetEnvironment() { return std::getenv("APP_ENVIRONMENT"); }

    } // namespace config
} // namespace simularium
} // namespace aics
