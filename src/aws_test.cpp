//snippet-sourcedescription:[get_object.cpp demonstrates how to retrieve an object from an Amazon S3 bucket.]
//snippet-service:[s3]
//snippet-keyword:[Amazon S3]
//snippet-keyword:[C++]
//snippet-keyword:[Code Sample]
//snippet-sourcetype:[full-example]
//snippet-sourceauthor:[AWS]


/*
   Copyright 2010-2019 Amazon.com, Inc. or its affiliates. All Rights Reserved.
   This file is licensed under the Apache License, Version 2.0 (the "License").
   You may not use this file except in compliance with the License. A copy of
   the License is located at
    http://aws.amazon.com/apache2.0/
   This file is distributed on an "AS IS" BASIS, WITHOUT WARRANTIES OR
   CONDITIONS OF ANY KIND, either express or implied. See the License for the
   specific language governing permissions and limitations under the License.
*/
//snippet-start:[s3.cpp.get_object.inc]
#include <aws/core/Aws.h>
#include <aws/s3/S3Client.h>
#include <aws/core/utils/threading/Executor.h>
#include <aws/transfer/TransferManager.h>
#include <aws/core/utils/memory/AWSMemory.h>
#include <aws/core/utils/threading/Executor.h>
#include <iostream>
//snippet-end:[s3.cpp.get_object.inc]

/**
 * Get an object from an Amazon S3 bucket.
 */
int main(int argc, char** argv)
{
  Aws::SDKOptions options;
  options.loggingOptions.logLevel = Aws::Utils::Logging::LogLevel::Info;

  Aws::InitAPI(options);
  {
    // snippet-start:[s3.cpp.get_object.code]
    // Assign these values before running the program
    const Aws::String bucket_name = "aics-agentviz-data";
    const Aws::String object_name = "trajectory/actin5-1.h5";  // For demo, set to a text file

    Aws::Client::ClientConfiguration config;
    config.region = "us-east-2";

    auto executor = Aws::MakeShared<Aws::Utils::Threading::PooledThreadExecutor>("test-pool", 10);
    Aws::Transfer::TransferManagerConfiguration tcc(executor.get());
    tcc.s3Client =std::make_shared<Aws::S3::S3Client>(config);

    auto transferManager = Aws::Transfer::TransferManager::Create(tcc);
    auto downloadHandle = transferManager->DownloadFile(bucket_name, object_name, "./data/" + object_name);

    downloadHandle->WaitUntilFinished();

    auto status = downloadHandle->GetStatus();
    if(status == Aws::Transfer::TransferStatus::FAILED)
    {
      std::cerr << downloadHandle->GetLastError() << std::endl;
    }
    else if (status == Aws::Transfer::TransferStatus::CANCELED)
    {
      std::cout << "Download cancelled" << std::endl;
    }
    else if (status == Aws::Transfer::TransferStatus::COMPLETED)
    {
      std::cout << "Completed download" << std::endl;
    }
  }

  Aws::ShutdownAPI(options);
  return 0;
}
