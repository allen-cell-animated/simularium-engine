#ifndef AICS_SIMULARIUM_CONFIG_H
#define AICS_SIMULARIUM_CONFIG_H

#include <string>

namespace aics {
namespace simularium {
namespace config {
  std::string GetCacheFolder();
  std::string GetS3Location();
  std::string GetS3CacheLocation();
  std::string GetEnvironment();

} // namespace config
} // namespace simularium
} // namespace aics

#endif // AICS_SIMULARIUM_CONFIG_H
