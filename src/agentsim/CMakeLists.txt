include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/readdypkg.cmake)
include(${CMAKE_CURRENT_SOURCE_DIR}/cmake/cytosimpkg.cmake)

set(SOURCES
"jsoncpp.cpp"
"aws_util.cpp"
"math_util.cpp"
"agent.cpp"
"model.cpp"
"simulation_cache.cpp"
"simulation.cpp"
"connection_manager.cpp"
"cli_client.cpp"
)

add_library(${TARGET} STATIC ${SOURCES})

target_include_directories(${TARGET} PRIVATE
"${INTERNAL_INCLUDES}"
"${EXTERNAL_INCLUDES}"
"${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-core/include"
"${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-transfer/include"
"${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-awstransfer/include"
"${DEPENDENCY_DIRECTORY}/aws-sdk-cpp/aws-cpp-sdk-s3/include"
"${OPENSSL_INCLUDES}"
)

target_link_libraries(${TARGET} PRIVATE
"readdyPKG"
"cytosimPKG"
"${AWS_DLLS}"
"${OPENSSL_LIBRARIES}"
"${PLATFORM_LIBRARIES}"
)
