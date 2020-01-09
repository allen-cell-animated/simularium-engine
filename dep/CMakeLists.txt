add_subdirectory(gtest)
add_subdirectory(readdy)
add_subdirectory(cytosim)
add_subdirectory(aws-sdk-cpp)

set(CYTOSIM_LIBRARIES
    "${CYTOSIM_LIBRARIES}"
    PARENT_SCOPE
)
