#ifndef AICS_AWS_UTIL_H
#define AICS_AWS_UTIL_H

#include <aws/core/Aws.h>

namespace aics {
namespace agentsim {
namespace aws_util {

bool Download(Aws::String object_name);

} // namespace aws_util
} // namespace agentsim
} // namespace aics

#endif // AICS_AWS_UTIL_H
