cmake_minimum_required(VERSION 3.2.0)

set(TARGET "agentsim")
set(SERVER_TARGET "agentsim_server")
set(TEST_TARGET "agentsim_tests")

# The file extension to attach to executables built
set(EXE_FILE_TYPE "exe")

set(PROGRAM "${TARGET}.${EXE_FILE_TYPE}")
set(SERVER_PROGRAM "${SERVER_TARGET}.${EXE_FILE_TYPE}")
set(TEST_PROGRAM "${TEST_TARGET}.${EXE_FILE_TYPE}")

set(SOURCE_DIRECTORY "src")
set(INCLUDE_DIRECTORY "inc")
set(EXTERNAL_DIRECTORY "ext")
set(DEPENDENCY_DIRECTORY "dep")

project(${TARGET})
add_executable(${PROGRAM} ${SOURCE_DIRECTORY}/${TARGET}.cpp)
add_executable(${SERVER_PROGRAM} ${SOURCE_DIRECTORY}/${SERVER_TARGET}.cpp)
add_executable(${TEST_PROGRAM} ${SOURCE_DIRECTORY}/${TEST_TARGET}.cpp)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

# HDF5 directory below is included for Ubuntu
set(EXTERNAL_INCLUDES
"${DEPENDENCY_DIRECTORY}/RakNet/include/RakNet"
"/usr/include/hdf5/serial"
"${DEPENDENCY_DIRECTORY}/readdy/include"
"${DEPENDENCY_DIRECTORY}/readdy/libraries/h5rd/include"
"${DEPENDENCY_DIRECTORY}/readdy/libraries/json/include"
"${DEPENDENCY_DIRECTORY}/readdy/libraries/spdlog/include"
)

include_directories(${CMAKE_SOURCE_DIR}/${INCLUDE_DIRECTORY} ${CMAKE_SOURCE_DIR}/${EXTERNAL_DIRECTORY} ${EXTERNAL_INCLUDES})

# Disable options in subdirectory
set(READDY_BUILD_PYTHON_WRAPPER OFF CACHE BOOL "Build Python wrappers")

add_subdirectory(${SOURCE_DIRECTORY})
add_subdirectory(${DEPENDENCY_DIRECTORY})

set(READDY_DLLS
"readdy"
"readdy_model"
"readdy_kernel_cpu"
"readdy_kernel_singlecpu"
"readdy_common"
"readdy_io"
"readdy_plugin"
)

set(RAKNET_DLLS "RakNetDLL")

set(AGENTSIM_DLL
"agentsim"
"gtest_main"
)

target_link_libraries(${PROGRAM}
"${AGENTSIM_DLL}"
"${READDY_DLLS}"
"${RAKNET_DLLS}"
)

target_link_libraries(${SERVER_PROGRAM}
"${AGENTSIM_DLL}"
"${READDY_DLLS}"
"${RAKNET_DLLS}"
)

target_link_libraries(${TEST_PROGRAM}
"${AGENTSIM_DLL}"
"${READDY_DLLS}"
"${RAKNET_DLLS}"
)
