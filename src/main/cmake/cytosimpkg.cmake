add_library("cytosimPKG" STATIC
    "${DEPENDENCY_DIRECTORY}/cytosim/src/play/frame_reader.cc"
    "cytosimpkg.cpp"
)

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

target_include_directories("cytosimPKG" PRIVATE
    "${CYTOSIM_PKG_INCLUDES}"
    "${INTERNAL_INCLUDES}"
)

target_link_libraries("cytosimPKG" PRIVATE
    "${CYTOSIM_DLLS}"
)
