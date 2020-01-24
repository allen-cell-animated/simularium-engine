#ifndef AICS_AWS_UTIL_H
#define AICS_AWS_UTIL_H

#include <string>

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
        *   @param  fileName    the local file path to the object to upload
        *   @param  objectName  the desired fullpath of the object on AWS
        *
        *   uploads a specified object to AWS S3 storage, intended to
        *   store objects that need persistent, centralized, storage
        *   (e.g. reusable caches)
        */
        bool Upload(std::string fileName, std::string objectName);

    } // namespace aws_util
} // namespace agentsim
} // namespace aics

#endif // AICS_AWS_UTIL_H
