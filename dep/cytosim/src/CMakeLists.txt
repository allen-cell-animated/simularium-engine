add_subdirectory("SFMT")
add_subdirectory("base")
add_subdirectory("math")
add_subdirectory("sim")
add_subdirectory("sim/spaces")
add_subdirectory("play")

if(BUILD_PLAY_EXECUTABLE OR BUILD_CYTOSIM_TESTS)
	add_subdirectory("gl")
endif()

if(BUILD_CYTOSIM_TOOLS)
	add_subdirectory("tools")
endif()

if(BUILD_CYTOSIM_TESTS)
	add_subdirectory("test")
endif()
