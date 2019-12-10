add_library("cytosimPKG" STATIC
    "${PROJECT_SOURCE_DIR}/dep/cytosim/src/play/frame_reader.cc"
    "cytosimpkg.cpp"
)

set(CYTOSIM_PKG_INCLUDES
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
  CACHE STRING "The include directories needed by the Cytosim simulation package"
)

target_include_directories("cytosimPKG" PRIVATE
    "${CYTOSIM_PKG_INCLUDES}"
    "${INTERNAL_INCLUDES}"
)

message("${CYTOSIM_DLLS}")
target_link_libraries("cytosimPKG" PRIVATE
    "${CYTOSIM_DLLS}"
)
