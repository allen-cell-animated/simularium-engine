cmake_minimum_required(VERSION 3.2.0)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(DIMENSION 3) # Used by Cytosim to compile 2D or 3D
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR})

set(TARGET "agentsim")
set(SERVER_TARGET "agentsim_server")
set(CLIENT_TARGET "agentsim_client")

add_compile_options(-Wno-deprecated-declarations)

message("Building Agent Viz with full debugging symbols")
add_definitions("-g3 -ggdb")

# The file extension to attach to executables built
set(EXE_FILE_TYPE "exe")

set(SERVER_PROGRAM "${SERVER_TARGET}.${EXE_FILE_TYPE}")
set(CLIENT_PROGRAM "${CLIENT_TARGET}.${EXE_FILE_TYPE}")

set(SOURCE_DIRECTORY "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIRECTORY "${CMAKE_SOURCE_DIR}/inc")
set(EXTERNAL_DIRECTORY "${CMAKE_SOURCE_DIR}/ext")
set(DEPENDENCY_DIRECTORY "${CMAKE_SOURCE_DIR}/dep")

project(${TARGET})

add_subdirectory(${DEPENDENCY_DIRECTORY})
add_subdirectory(${SOURCE_DIRECTORY})

file(MAKE_DIRECTORY "${CMAKE_BINARY_DIR}/trajectory")
