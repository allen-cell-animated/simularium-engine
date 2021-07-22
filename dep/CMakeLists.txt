# GOOGLE TEST DEPENDENCY
#add_subdirectory(gtest)

# READDY DEPENDENCY
#set(READDY_BUILD_PYTHON_WRAPPER OFF CACHE BOOL "don't build ReaDDy's python wrapper")
#set(READDY_CREATE_TEST_TARGET OFF CACHE BOOL "don't build Readdy test target")
#add_subdirectory(readdy)

# AWS SDK CPP DEPENDENCY
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    add_compile_options("-Wno-stringop-truncation")
    add_compile_options("-Wno-nonnull")
endif()
add_subdirectory(aws-sdk-cpp)
if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_compile_options(aws-cpp-sdk-core PUBLIC "-Wno-nonnull")
    target_compile_options(aws-cpp-sdk-core PUBLIC "-Wno-uninitialized")
    target_compile_options(aws-cpp-sdk-core PUBLIC "-Wno-stringop-overflow")
endif()


# CYTOSIM DEPENDENCY
#set(BUILD_SIM_EXECUTABLE OFF CACHE BOOL "don't build Cytosim Sim executable" FORCE)
#set(BUILD_PLAY_EXECUTABLE OFF CACHE BOOL "don't build Cytosim Play executable" FORCE)
#set(BUILD_CYTOSIM_TESTS OFF CACHE BOOL "don't build Cytosim Tests" FORCE)
#set(BUILD_CYTOSIM_TOOLS OFF CACHE BOOL "don't build Cytosim Tools" FORCE)

#if("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
#    add_definitions("-mno-avx -mno-avx2")
#endif()
#add_subdirectory(cytosim)

#    "${CYTOSIM_LIBRARIES}"
set(CYTOSIM_LIBRARIES
    PARENT_SCOPE
)
