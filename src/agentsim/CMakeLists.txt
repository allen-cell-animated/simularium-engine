set(SOURCES
"${PROJECT_SOURCE_DIR}/dep/cytosim/src/play/frame_reader.cc"
"jsoncpp.cpp"
"aws_util.cpp"
"agent.cpp"
"model.cpp"
"readdypkg.cpp"
"cytosimpkg.cpp"
"simulation_cache.cpp"
"simulation.cpp"
)

find_library(LAPACK_LIB lapack REQUIRED)
find_library(BLAS_LIB blas REQUIRED)

set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)

add_library(${TARGET} STATIC ${SOURCES})

target_include_directories(${TARGET} PRIVATE
${INTERNAL_INCLUDES}
${EXTERNAL_INCLUDES}
)

target_link_libraries(${TARGET} PRIVATE
"${HDF5_DLLS}"
"${READDY_DLLS}"
"${CYTOSIM_LIBRARIES}"
"${AWS_DLLS}"
)
