add_library("loguru" STATIC "${EXTERNAL_DIRECTORY}/loguru/loguru.cpp")
target_include_directories("loguru" PRIVATE "${EXTERNAL_DIRECTORY}/loguru")

add_library("logger" STATIC "logger.cpp")
target_include_directories("logger" PRIVATE
    "${INCLUDE_DIRECTORY}"
    "${EXTERNAL_DIRECTORY}"
)
target_link_libraries("logger" PRIVATE
    "loguru"
)
