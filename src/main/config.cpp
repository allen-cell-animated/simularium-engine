#include "simularium/config/config.h"

namespace aics {
namespace simularium {
    namespace config {
        std::string GetCacheFolder() { return "/tmp/aics/simularium/"; }
        std::string GetS3Location() { return "trajectory/"; }
        std::string GetS3CacheLocation() { return "cache/trajectory/" + GetEnvironment() + "/"; }
        std::string GetEnvironment() { char* env = std::getenv("APP_ENVIRONMENT"); if (env) return env; else return ""; }

    } // namespace config
} // namespace simularium
} // namespace aics
