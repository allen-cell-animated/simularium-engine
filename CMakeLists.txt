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

# HDF5 directory below is included for Ubuntu
set(EXTERNAL_INCLUDES
"${CMAKE_SOURCE_DIR}/${EXTERNAL_DIRECTORY}/raknet"
"${CMAKE_SOURCE_DIR}/${EXTERNAL_DIRECTORY}/readdy"
"${CMAKE_SOURCE_DIR}/${EXTERNAL_DIRECTORY}/openmm"
"/usr/include/hdf5/serial"
)

include_directories(${CMAKE_SOURCE_DIR}/${INCLUDE_DIRECTORY} ${CMAKE_SOURCE_DIR}/${EXTERNAL_DIRECTORY} ${EXTERNAL_INCLUDES})

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
"pthread"
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
