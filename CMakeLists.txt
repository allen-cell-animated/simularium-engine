cmake_minimum_required(VERSION 3.2.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(AGENTVIZ_FULL_DEBUG_SYMBOLS TRUE)

set(TARGET "agentsim")
set(SERVER_TARGET "agentsim_server")
set(CLIENT_TARGET "agentsim_client")
set(TEST_TARGET "agentsim_tests")

link_directories(
  "/usr/local/opt/openssl/lib"
)

if(${AGENTVIZ_FULL_DEBUG_SYMBOLS})
  message("Building Agent Viz with full debugging symbols")
  add_definitions("-g3 -ggdb")
endif()

# The file extension to attach to executables built
set(EXE_FILE_TYPE "exe")

set(SERVER_PROGRAM "${SERVER_TARGET}.${EXE_FILE_TYPE}")
set(CLIENT_PROGRAM "${CLIENT_TARGET}.${EXE_FILE_TYPE}")
set(TEST_PROGRAM "${TEST_TARGET}.${EXE_FILE_TYPE}")

set(SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIRECTORY "${CMAKE_SOURCE_DIR}/inc")
set(EXTERNAL_DIRECTORY "${CMAKE_SOURCE_DIR}/ext")
set(DEPENDENCY_DIRECTORY "${CMAKE_SOURCE_DIR}/dep")

SET(CMAKE_EXE_LINKER_FLAGS
"${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath /usr/local/lib -Wl,-rpath /usr/lib")

project(${TARGET})
add_executable(${SERVER_PROGRAM} ${SOURCE_DIRECTORY}/${SERVER_TARGET}.cpp)
add_executable(${TEST_PROGRAM} ${SOURCE_DIRECTORY}/${TEST_TARGET}.cpp)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(HDF5 COMPONENTS HL REQUIRED)

if(UNIX AND APPLE)
  set(PLATFORM_LIBRARIES
    "crypto"
  )
endif()

set(INTERNAL_INCLUDES
"${INCLUDE_DIRECTORY}"
"${SOURCE_DIRECTORY}"
"${EXTERNAL_DIRECTORY}"
)

target_include_directories(${SERVER_PROGRAM} PUBLIC
${INTERNAL_INCLUDES}
"${EXTERNAL_DIRECTORY}/websocketpp"
"${EXTERNAL_DIRECTORY}/websocketpp/common"
"${EXTERNAL_DIRECTORY}/asio"
"${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-core/include"
"${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-transfer/include"
"${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-awstransfer/include"
"${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-s3/include"
)

target_include_directories(${TEST_PROGRAM} PUBLIC
"${INTERNAL_INCLUDES}"
"${EXTERNAL_DIRECTORY}/websocketpp"
"${EXTERNAL_DIRECTORY}/websocketpp/common"
"${EXTERNAL_DIRECTORY}/asio"
)

add_subdirectory(${DEPENDENCY_DIRECTORY})
set(CYTOSIM_DLLS "${CYTOSIM_LIBRARIES}")
add_subdirectory(${SOURCE_DIRECTORY})

target_link_libraries(${SERVER_PROGRAM}
    "${TARGET}"
    Threads::Threads
)

target_link_libraries(${TEST_PROGRAM}
  "${TARGET}"
  Threads::Threads
  "gtest_main"
  "gtest"
)

target_include_directories(${TEST_PROGRAM} PRIVATE
    "${READDY_PKG_INCLUDES}"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-core/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-transfer/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-awstransfer/include"
    "${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-s3/include"
)

set(CLIENT_PROGRAM_SOURCES
    "${SOURCE_DIRECTORY}/${CLIENT_TARGET}.cpp"
    "${SOURCE_DIRECTORY}/main/jsoncpp.cpp"
    "${SOURCE_DIRECTORY}/main/cli_client.cpp"
)
add_executable(${CLIENT_PROGRAM} ${CLIENT_PROGRAM_SOURCES})
target_include_directories(${CLIENT_PROGRAM} PRIVATE
    ${INTERNAL_INCLUDES}
    "${EXTERNAL_DIRECTORY}/websocketpp"
    "${EXTERNAL_DIRECTORY}/websocketpp/common"
    "${EXTERNAL_DIRECTORY}/asio"
    "${OPENSSL_INCLUDES}"
    "/usr/local/opt/openssl/include"
)
target_link_libraries(${CLIENT_PROGRAM}
    Threads::Threads
    ${OPENSSL_LIBRARIES}
    ${PLATFORM_LIBRARIES}
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/trajectory")
