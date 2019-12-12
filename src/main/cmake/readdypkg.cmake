add_library("readdyPKG" STATIC
    "math_util.cpp"
    "readdypkg.cpp"
)

set(READDY_PKG_INCLUDES
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
    CACHE STRING "The include directories needed by the ReaDDy simulation package"
)

target_include_directories("readdyPKG" PRIVATE
    "${READDY_PKG_INCLUDE_DIRECTORIES}"
    "${INTERNAL_INCLUDES}"
    "${EXTERNAL_DIRECTORY}"
)
target_link_libraries("readdyPKG" PRIVATE
    "readdy"
    "readdy_model"
    "readdy_kernel_cpu"
    "readdy_kernel_singlecpu"
    "readdy_common"
    "readdy_io"
    "readdy_plugin"
    "${HDF5_LIBRARIES}"
    "${HDF5_HL_LIBRARIES}"
)
