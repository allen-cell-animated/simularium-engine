add_library("jsoncpp" STATIC "${EXTERNAL_DIRECTORY}/json/jsoncpp.cpp")
target_include_directories("jsoncpp" PUBLIC "${EXTERNAL_DIRECTORY}")
