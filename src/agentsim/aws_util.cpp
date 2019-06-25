#include <aws/core/Aws.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/threading/Executor.h>
#include <aws/s3/S3Client.h>
#include <aws/transfer/TransferManager.h>
#include <iostream>

static const Aws::String bucket_name = "aics-agentviz-data";

namespace aics {
namespace agentsim {
    namespace aws_util {

        bool Download(Aws::String object_name)
        {
            bool success = true;
            Aws::SDKOptions options;
            options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;

            Aws::InitAPI(options);
            {
                // snippet-start:[s3.cpp.get_object.code]
                // Assign these values before running the program

                Aws::Client::ClientConfiguration config;
                config.region = "us-east-2";

                auto executor = Aws::MakeShared<Aws::Utils::Threading::PooledThreadExecutor>("test-pool", 10);
                Aws::Transfer::TransferManagerConfiguration tcc(executor.get());
                tcc.s3Client = std::make_shared<Aws::S3::S3Client>(config);

                auto transferManager = Aws::Transfer::TransferManager::Create(tcc);
                auto downloadHandle = transferManager->DownloadFile(bucket_name, object_name, object_name);

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

    } // namespace aws_util
} // namespace agentsim
} // namespace aics
