cmake_minimum_required(VERSION 3.2.0)

set(TARGET "agentsim")
set(SERVER_TARGET "agentsim_server")
set(TEST_TARGET "agentsim_tests")

# The file extension to attach to executables built
set(EXE_FILE_TYPE "exe")

set(PROGRAM "${TARGET}.${EXE_FILE_TYPE}")
set(SERVER_PROGRAM "${SERVER_TARGET}.${EXE_FILE_TYPE}")
set(TEST_PROGRAM "${TEST_TARGET}.${EXE_FILE_TYPE}")

set(SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIRECTORY "${CMAKE_SOURCE_DIR}/inc")
set(EXTERNAL_DIRECTORY "${CMAKE_SOURCE_DIR}/ext")
set(DEPENDENCY_DIRECTORY "${CMAKE_SOURCE_DIR}/dep")

project(${TARGET})
add_executable(${PROGRAM} ${SOURCE_DIRECTORY}/${TARGET}.cpp)
add_executable(${SERVER_PROGRAM} ${SOURCE_DIRECTORY}/${SERVER_TARGET}.cpp)
add_executable(${TEST_PROGRAM} ${SOURCE_DIRECTORY}/${TEST_TARGET}.cpp)

# HDF5 directory below is included for Ubuntu
set(EXTERNAL_INCLUDES
"/usr/include/hdf5/serial"
"${EXTERNAL_DIRECTORY}/raknet"
"${EXTERNAL_DIRECTORY}/readdy"
"${EXTERNAL_DIRECTORY}/websocketpp"
"${EXTERNAL_DIRECTORY}/websocketpp/common"
"${EXTERNAL_DIRECTORY}/asio"
)

set(INTERNAL_INCLUDES
"${INCLUDE_DIRECTORY}"
"${EXTERNAL_DIRECTORY}"
)

target_include_directories(${PROGRAM} PUBLIC
${INTERNAL_INCLUDES}
${EXTERNAL_INCLUDES}
)

target_include_directories(${SERVER_PROGRAM} PUBLIC
${INTERNAL_INCLUDES}
${EXTERNAL_INCLUDES}
)

target_include_directories(${TEST_PROGRAM} PUBLIC
${INTERNAL_INCLUDES}
${EXTERNAL_INCLUDES}
)


# Disable options in subdirectory
set(READDY_BUILD_PYTHON_WRAPPER OFF CACHE BOOL "Build Python wrappers")

add_subdirectory(${SOURCE_DIRECTORY})
add_subdirectory(${DEPENDENCY_DIRECTORY})

add_dependencies(${TARGET} readdy)

set(READDY_DLLS
"readdy"
"readdy_model"
"readdy_kernel_cpu"
"readdy_kernel_singlecpu"
"readdy_common"
"readdy_io"
"readdy_plugin"
)

set(AGENTSIM_DLL
"agentsim"
"gtest_main"
)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

target_link_libraries(${PROGRAM}
"${AGENTSIM_DLL}"
"${READDY_DLLS}"
)

target_link_libraries(${SERVER_PROGRAM}
Threads::Threads
"${AGENTSIM_DLL}"
"${READDY_DLLS}"
)

target_link_libraries(${TEST_PROGRAM}
"${AGENTSIM_DLL}"
"${READDY_DLLS}"
)
