# All of the tests should be placed alongside the certificates needed for the
#  network tests, the top level of the binary directory
set(EXECUTABLE_OUTPUT_PATH
    "${CMAKE_BINARY_DIR}"
)

set(TEST_LIST
"test_client_server"
"test_net_commands"
"test_sim_time"
)

set(TEST_INCLUDES
    "${INCLUDE_DIRECTORY}"
    "${EXTERNAL_DIRECTORY}"
    "${EXTERNAL_DIRECTORY}/websocketpp"
    "${EXTERNAL_DIRECTORY}/websocketpp/common"
    "${EXTERNAL_DIRECTORY}/asio"
    "${DEPENDENCY_DIRECTORY}/gtest/googletest/include"
    "${OPENSSL_INCLUDES}"
    "/usr/local/opt/openssl/include"
)

set(TEST_LIBS
    "${TARGET}"
    Threads::Threads
    "gtest_main"
    "gtest"
    "${CYTOSIM_LIBRARIES}"
)

foreach(TEST_NAME ${TEST_LIST})
	add_executable(${TEST_NAME} "${TEST_NAME}.cpp")
	target_include_directories(${TEST_NAME} PUBLIC "${TEST_INCLUDES}")
	target_link_libraries(${TEST_NAME} PUBLIC "${TEST_LIBS}")
endforeach()

add_executable(test_aws_util "test_aws_util.cpp")
target_include_directories(test_aws_util PUBLIC
    "${INCLUDE_DIRECTORY}"
    "${EXTERNAL_DIRECTORY}"
    "${EXTERNAL_DIRECTORY}/websocketpp"
    "${EXTERNAL_DIRECTORY}/websocketpp/common"
    "${EXTERNAL_DIRECTORY}/asio"
    "${DEPENDENCY_DIRECTORY}/gtest/googletest/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/crt/aws-crt-cpp/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/crt/aws-crt-cpp/crt/aws-c-common/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/crt/aws-crt-cpp/crt/aws-c-common/verification/cbmc/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/crt/aws-crt-cpp/crt/aws-c-io/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/crt/aws-crt-cpp/crt/aws-c-mqtt/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-core/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-s3/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-awstransfer/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-transfer/include"
    "${OPENSSL_INCLUDES}"
    "/usr/local/opt/openssl/include"
)
target_link_libraries(test_aws_util PUBLIC
	"${TARGET}"
	"gtest_main"
	"gtest"
)
