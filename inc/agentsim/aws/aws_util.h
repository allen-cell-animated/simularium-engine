#ifndef AICS_AWS_UTIL_H
#define AICS_AWS_UTIL_H

#include <string>
#include <aws/core/Aws.h>

namespace aics {
namespace agentsim {
    namespace aws_util {

        /**
        *   Download
        *
        *   @param  objectName  the path in S3 to the desired object (e.g. "folder/file1.txt")
        *   @param  destination the file-path to save the object to (e.g. /tmp/cache.txt)
        *
        *   downloads a specified object from AWS S3
        */
        bool Download(std::string objectName, std::string destination);

        /**
        *   Upload
        *
        *   @param  objectName  the desired fullpath
        */
        bool Upload(std::string fileName, std::string objectName);

    } // namespace aws_util
} // namespace agentsim
} // namespace aics

#endif // AICS_AWS_UTIL_H
