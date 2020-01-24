add_subdirectory(gtest)
add_subdirectory(readdy)
add_subdirectory(aws-sdk-cpp)

set(BUILD_SIM_EXECUTABLE OFF CACHE BOOL "don't build Cytosim Sim EXE" FORCE)
set(BUILD_PLAY_EXECUTABLE OFF CACHE BOOL "don't build Cytosim Play EXE" FORCE)
set(BUILD_CYTOSIM_TESTS OFF CACHE BOOL "don't build Cytosim Tests" FORCE)
set(BUILD_CYTOSIM_TOOLS OFF CACHE BOOL "don't build Cytosim Tools" FORCE)

if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_definitions("-mno-avx -mno-avx2")
endif()
add_subdirectory(cytosim)

set(CYTOSIM_LIBRARIES
    "${CYTOSIM_LIBRARIES}"
    PARENT_SCOPE
)
