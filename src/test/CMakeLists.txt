# All of the tests should be placed alongside the certificates needed for the
#  network tests, the top level of the binary directory
set(EXECUTABLE_OUTPUT_PATH
    "${CMAKE_BINARY_DIR}"
)

set(TEST_LIST
"test_agent"
"test_aws_util"
"test_client_server"
"test_net_commands"
)

set(TEST_INCLUDES
    "${INTERNAL_INCLUDES}"
    "${EXTERNAL_DIRECTORY}/websocketpp"
    "${EXTERNAL_DIRECTORY}/websocketpp/common"
    "${EXTERNAL_DIRECTORY}/asio"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-core/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-transfer/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-awstransfer/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-s3/include"
    "${DEPENDENCY_DIRECTORY}/gtest/googletest/include"
    "${OPENSSL_INCLUDES}"
    "/usr/local/opt/openssl/include"
)

set(TEST_LIBS
    "${TARGET}"
    Threads::Threads
    "gtest_main"
    "gtest"
)

foreach(TEST_NAME ${TEST_LIST})
	add_executable(${TEST_NAME} "${TEST_NAME}.cpp")
	target_include_directories(${TEST_NAME} PUBLIC "${TEST_INCLUDES}")
	target_link_libraries(${TEST_NAME} PUBLIC "${TEST_LIBS}")
endforeach()
