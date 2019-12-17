add_library("loguru" STATIC "${EXTERNAL_DIRECTORY}/loguru/loguru.cpp")
target_include_directories("loguru" PRIVATE "${EXTERNAL_DIRECTORY}/loguru")
