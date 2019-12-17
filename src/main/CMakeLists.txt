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
"${INCLUDE_DIRECTORY}"
"${EXTERNAL_DIRECTORY}"
"${EXTERNAL_DIRECTORY}/websocketpp"
"${EXTERNAL_DIRECTORY}/websocketpp/common"
"${EXTERNAL_DIRECTORY}/asio"
"${OPENSSL_INCLUDES}"
"/usr/local/opt/openssl/include"
)

target_link_libraries(${TARGET} PRIVATE
"readdyPKG"
"cytosimPKG"
"aws-cpp-sdk-s3"
"aws-cpp-sdk-awstransfer"
"aws-cpp-sdk-transfer"
"aws-cpp-sdk-core"
"${OPENSSL_LIBRARIES}"
"${PLATFORM_LIBRARIES}"
)
