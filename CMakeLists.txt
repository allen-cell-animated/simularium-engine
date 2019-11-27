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

add_compile_options(-Wno-deprecated-declarations)

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

set(CYTOSIM_INCLUDES
  "${EXTERNAL_DIRECTORY}/cytosim"
  "${EXTERNAL_DIRECTORY}/cytosim/sim"
  "${EXTERNAL_DIRECTORY}/cytosim/sim/spaces"
  "${EXTERNAL_DIRECTORY}/cytosim/sim/couples"
  "${EXTERNAL_DIRECTORY}/cytosim/sim/fibers"
  "${EXTERNAL_DIRECTORY}/cytosim/sim/hands"
  "${EXTERNAL_DIRECTORY}/cytosim/sim/organizers"
  "${EXTERNAL_DIRECTORY}/cytosim/sim/singles"
  "${EXTERNAL_DIRECTORY}/cytosim/base"
  "${EXTERNAL_DIRECTORY}/cytosim/math"
  "${EXTERNAL_DIRECTORY}/cytosim/SFMT"
  "${EXTERNAL_DIRECTORY}/cytosim/play"
)

set(EXTERNAL_INCLUDES
"${EXTERNAL_DIRECTORY}/readdy"
"${CYTOSIM_INCLUDES}"
"${EXTERNAL_DIRECTORY}/websocketpp"
"${EXTERNAL_DIRECTORY}/websocketpp/common"
"${EXTERNAL_DIRECTORY}/asio"
"${EXTERNAL_DIRECTORY}/aws-sdk-cpp"
"${HDF5_INCLUDES}"
"${OPENSSL_INCLUDES}"
"/usr/local/opt/openssl/include"
)

set(INTERNAL_INCLUDES
"${INCLUDE_DIRECTORY}"
"${SOURCE_DIRECTORY}"
"${EXTERNAL_DIRECTORY}"
)

target_include_directories(${SERVER_PROGRAM} PUBLIC
${INTERNAL_INCLUDES}
${EXTERNAL_INCLUDES}
)

target_include_directories(${TEST_PROGRAM} PUBLIC
${INTERNAL_INCLUDES}
${EXTERNAL_INCLUDES}
)

add_subdirectory(${SOURCE_DIRECTORY})
add_subdirectory(${DEPENDENCY_DIRECTORY})

add_dependencies(${TARGET} readdy)
add_dependencies(${TARGET} gtest)
add_dependencies(${TARGET} gtest_main)

foreach(CYTO_LIB ${CYTOSIM_LIBRARIES})
  add_dependencies(${TARGET} ${CYTO_LIB})
  break()
endforeach()

set(CYTOSIM_DLLS
  "${CYTOSIM_LIBRARIES}"
)

set(AGENTSIM_DLL
  "agentsim"
)

set(GTEST_DLLS
  "gtest_main"
  "gtest"
)

set(AWS_DLLS
  "aws-cpp-sdk-s3"
  "aws-cpp-sdk-awstransfer"
  "aws-cpp-sdk-transfer"
  "aws-cpp-sdk-core"
)

target_link_libraries(${SERVER_PROGRAM}
  "${AGENTSIM_DLL}"
  "${CYTOSIM_DLLS}"
  Threads::Threads
  "${AWS_DLLS}"
)

target_link_libraries(${TEST_PROGRAM}
  "${AGENTSIM_DLL}"
  Threads::Threads
  "${GTEST_DLLS}"
  ${AWS_DLLS}
)

target_include_directories(${TEST_PROGRAM} PRIVATE
    "${DEPENDENCY_DIRECTORY}/readdy/include/"
    "${DEPENDENCY_DIRECTORY}/readdy/readdy_testing/include"
    "${DEPENDENCY_DIRECTORY}/readdy/libraries/c-blosc/include/"
    "${DEPENDENCY_DIRECTORY}/readdy/libraries/Catch2/include/"
    "${DEPENDENCY_DIRECTORY}/readdy/libraries/h5rd/include/"
    "${DEPENDENCY_DIRECTORY}/readdy/libraries/json/include/"
    "${DEPENDENCY_DIRECTORY}/readdy/libraries/pybind11/include/"
    "${DEPENDENCY_DIRECTORY}/readdy/libraries/spdlog/include/"
    "${HDF5_INCLUDE_DIRS}"
    "${HDF5_HL_INCLUDE_DIR}"
)

set(CLIENT_PROGRAM_SOURCES
    "${SOURCE_DIRECTORY}/${CLIENT_TARGET}.cpp"
    "${SOURCE_DIRECTORY}/${TARGET}/jsoncpp.cpp"
    "${SOURCE_DIRECTORY}/${TARGET}/cli_client.cpp"
)
add_executable(${CLIENT_PROGRAM} ${CLIENT_PROGRAM_SOURCES})
target_include_directories(${CLIENT_PROGRAM} PRIVATE
    ${INTERNAL_INCLUDES}
    ${EXTERNAL_INCLUDES}
    ${OPENSSL_INCLUDES}
)
target_link_libraries(${CLIENT_PROGRAM}
    Threads::Threads
    ${OPENSSL_LIBRARIES}
    ${PLATFORM_LIBRARIES}
)

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/trajectory")
