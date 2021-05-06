#include <aws/core/Aws.h>
#include <aws/core/utils/logging/ConsoleLogSystem.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/threading/Executor.h>
#include <aws/s3/S3Client.h>
#include <aws/transfer/TransferManager.h>
#include <iostream>
#include <string>

static const Aws::String kBucketName = "aics-agentviz-data";
static const Aws::String kAwsRegion = "us-east-2";

namespace aics {
namespace simularium {
    namespace aws_util {

        bool Download(std::string objectNameStr, std::string destinationStr)
        {
            auto objectName = Aws::String(objectNameStr.c_str(), objectNameStr.size());
            auto destination = Aws::String(destinationStr.c_str(), destinationStr.size());

            bool success = true;
            Aws::SDKOptions options;
            options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Error;
            options.loggingOptions.logger_create_fn =
                [] {
                    return std::make_shared<Aws::Utils::Logging::ConsoleLogSystem>(
                        Aws::Utils::Logging::LogLevel::Error);
                };

            Aws::InitAPI(options);
            {
                Aws::Client::ClientConfiguration config;
                config.region = kAwsRegion;

                auto executor = Aws::MakeShared<Aws::Utils::Threading::PooledThreadExecutor>("test-pool", 10);
                Aws::Transfer::TransferManagerConfiguration tcc(executor.get());
                tcc.s3Client = std::make_shared<Aws::S3::S3Client>(config);

                auto transferManager = Aws::Transfer::TransferManager::Create(tcc);
                auto downloadHandle = transferManager->DownloadFile(kBucketName, objectName, destination);

                downloadHandle->WaitUntilFinished();

                auto status = downloadHandle->GetStatus();
                if (status == Aws::Transfer::TransferStatus::FAILED || status == Aws::Transfer::TransferStatus::CANCELED) {
                    std::cerr << downloadHandle->GetLastError() << std::endl;
                    success = false;
                }
            }

            Aws::ShutdownAPI(options);
            return success;
        }

        bool Upload(std::string fileNameStr, std::string objectNameStr)
        {
            auto fileName = Aws::String(fileNameStr.c_str(), fileNameStr.size());
            auto objectName = Aws::String(objectNameStr.c_str(), objectNameStr.size());

            bool success = true;
            Aws::SDKOptions options;
            options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Error;
            options.loggingOptions.logger_create_fn =
                [] {
                    return std::make_shared<Aws::Utils::Logging::ConsoleLogSystem>(
                        Aws::Utils::Logging::LogLevel::Error);
                };

            Aws::InitAPI(options);
            {
                Aws::Client::ClientConfiguration config;
                config.region = kAwsRegion;

                auto executor = Aws::MakeShared<Aws::Utils::Threading::PooledThreadExecutor>("test-pool", 10);
                Aws::Transfer::TransferManagerConfiguration tcc(executor.get());
                tcc.s3Client = std::make_shared<Aws::S3::S3Client>(config);

                auto transferManager = Aws::Transfer::TransferManager::Create(tcc);
                auto uploadHandle = transferManager->UploadFile(
                    fileName,
                    kBucketName,
                    objectName,
                    "text/binary",
                    Aws::Map<Aws::String, Aws::String>());

                uploadHandle->WaitUntilFinished();

                auto status = uploadHandle->GetStatus();
                if (status == Aws::Transfer::TransferStatus::FAILED || status == Aws::Transfer::TransferStatus::CANCELED) {
                    std::cerr << uploadHandle->GetLastError() << std::endl;
                    success = false;
                }
            }

            Aws::ShutdownAPI(options);
            return success;
        }

    } // namespace aws_util
} // namespace simularium
} // namespace aics
