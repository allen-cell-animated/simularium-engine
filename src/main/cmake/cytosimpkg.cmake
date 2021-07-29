add_library("cytosimPKG" STATIC
    "${DEPENDENCY_DIRECTORY}/cytosim/src/play/frame_reader.cc"
    "agent.cpp"
    "cytosimpkg.cpp"
)

target_compile_definitions("cytosimPKG" PRIVATE
-DDIM=${DIMENSION}
)

find_library(LAPACK_LIB lapack REQUIRED)
find_library(BLAS_LIB blas REQUIRED)

if(NOT BLAS_LIB)
    message(FATAL_ERROR "BLAS library not found")
endif()
set(CYTOSIM_PKG_INCLUDES
  "${DEPENDENCY_DIRECTORY}/cytosim/src"
  "${DEPENDENCY_DIRECTORY}/cytosim/src/sim"
  "${DEPENDENCY_DIRECTORY}/cytosim/src/sim/spaces"
  "${DEPENDENCY_DIRECTORY}/cytosim/src/sim/couples"
  "${DEPENDENCY_DIRECTORY}/cytosim/src/sim/fibers"
  "${DEPENDENCY_DIRECTORY}/cytosim/src/sim/hands"
  "${DEPENDENCY_DIRECTORY}/cytosim/src/sim/organizers"
  "${DEPENDENCY_DIRECTORY}/cytosim/src/sim/singles"
  "${DEPENDENCY_DIRECTORY}/cytosim/src/base"
  "${DEPENDENCY_DIRECTORY}/cytosim/src/math"
  "${DEPENDENCY_DIRECTORY}/cytosim/src/SFMT"
  "${DEPENDENCY_DIRECTORY}/cytosim/src/play"
  CACHE STRING "The include directories needed by the Cytosim simulation package"
)

message("Cytosim packages ${CYTOSIM_LIBRARIES}")

target_include_directories("cytosimPKG" PRIVATE
    "${CYTOSIM_PKG_INCLUDES}"
    "${INCLUDE_DIRECTORY}"
    "${EXTERNAL_DIRECTORY}"
)

target_link_libraries("cytosimPKG" PRIVATE
    "cytosimD3"
    "cytospaceD3";
    "cytomath";
    "cytobase"
    Threads::Threads
    "${LAPACK_LIB}"
    "${BLAS_LIB}"
    "loguru"
)
