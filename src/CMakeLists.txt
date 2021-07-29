SET(CMAKE_EXE_LINKER_FLAGS
"${CMAKE_EXE_LINKER_FLAGS} -Wl,-rpath /usr/local/lib -Wl,-rpath /usr/lib")

link_directories(
  "/usr/local/opt/openssl/lib"
)

#set(THREADS_PREFER_PTHREAD_FLAG ON)
find_package(Threads REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(HDF5 COMPONENTS HL REQUIRED)

add_executable(${SERVER_PROGRAM} ${SOURCE_DIRECTORY}/${SERVER_TARGET}.cpp)

if(UNIX AND APPLE)
  set(PLATFORM_LIBRARIES
    "crypto"
  )
endif()

target_include_directories(${SERVER_PROGRAM} PUBLIC
"${INCLUDE_DIRECTORY}"
"${EXTERNAL_DIRECTORY}"
"${EXTERNAL_DIRECTORY}/websocketpp"
"${EXTERNAL_DIRECTORY}/websocketpp/common"
"${EXTERNAL_DIRECTORY}/asio"
"/usr/local/opt/openssl/include"
)

target_link_libraries(${SERVER_PROGRAM}
    "${TARGET}"
    Threads::Threads
)

set(CLIENT_PROGRAM_SOURCES
    "${SOURCE_DIRECTORY}/${CLIENT_TARGET}.cpp"
    "${EXTERNAL_DIRECTORY}/json/jsoncpp.cpp"
    "${SOURCE_DIRECTORY}/main/cli_client.cpp"
)
add_executable(${CLIENT_PROGRAM} ${CLIENT_PROGRAM_SOURCES})
target_include_directories(${CLIENT_PROGRAM} PRIVATE
    "${INCLUDE_DIRECTORY}"
    "${EXTERNAL_DIRECTORY}"
    "${EXTERNAL_DIRECTORY}/websocketpp"
    "${EXTERNAL_DIRECTORY}/websocketpp/common"
    "${EXTERNAL_DIRECTORY}/asio"
    "${OPENSSL_INCLUDES}"
    "/usr/local/opt/openssl/include"
)
target_link_libraries(${CLIENT_PROGRAM}
    Threads::Threads
    ${OPENSSL_LIBRARIES}
    ${PLATFORM_LIBRARIES}
)

add_subdirectory("main")
add_subdirectory("test")

