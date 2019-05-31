cmake_minimum_required(VERSION 3.2.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(AGENTVIZ_FULL_DEBUG_SYMBOLS TRUE)

set(TARGET "agentsim")
set(SERVER_TARGET "agentsim_server")
set(CLIENT_TARGET "agentsim_client")
set(TEST_TARGET "agentsim_tests")

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

project(${TARGET})
add_executable(${SERVER_PROGRAM} ${SOURCE_DIRECTORY}/${SERVER_TARGET}.cpp)
add_executable(${CLIENT_PROGRAM} ${SOURCE_DIRECTORY}/${CLIENT_TARGET}.cpp)
add_executable(${TEST_PROGRAM} ${SOURCE_DIRECTORY}/${TEST_TARGET}.cpp)

find_package(HDF5 COMPONENTS HL REQUIRED)
set(HDF5_INCLUDES
"${HDF5_INCLUDE_DIRS}"
"${HDF5_HL_INCLUDE_DIR}"
)
set(HDF5_DLLS
"${HDF5_LIBRARIES}"
"${HDF5_HL_LIBRARIES}"
)

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

target_include_directories(${CLIENT_PROGRAM} PUBLIC
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

find_library(LAPACK_LIB lapack REQUIRED)
find_library(BLAS_LIB blas REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

foreach(CYTO_LIB ${CYTOSIM_LIBRARIES})
  add_dependencies(${TARGET} ${CYTO_LIB})
  break()
endforeach()


set(READDY_DLLS
  "readdy"
  "readdy_model"
  "readdy_kernel_cpu"
  "readdy_kernel_singlecpu"
  "readdy_common"
  "readdy_io"
  "readdy_plugin"
)

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
  "${HDF5_DLLS}"
  "${READDY_DLLS}"
)

target_link_libraries(${CLIENT_PROGRAM}
  "${AGENTSIM_DLL}"
  "${CYTOSIM_DLLS}"
  Threads::Threads
  "${HDF5_DLLS}"
  "${READDY_DLLS}"
)

target_link_libraries(${TEST_PROGRAM}
  "${AGENTSIM_DLL}"
  "${CYTOSIM_DLLS}"
  Threads::Threads
  "${GTEST_DLLS}"
  "${HDF5_DLLS}"
  "${READDY_DLLS}"
  ${AWS_DLLS}
)
