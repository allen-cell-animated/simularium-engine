set(SOURCES
"jsoncpp.cpp"
"agent.cpp"
"combination_reaction.cpp"
"model.cpp"
"readdypkg.cpp"
"simpleinteraction.cpp"
"simplemove.cpp"
"simulation.cpp"
"statechange_reaction.cpp"
)

add_library(${TARGET} SHARED ${SOURCES})

target_include_directories(${TARGET} PRIVATE
${INTERNAL_INCLUDES}
${EXTERNAL_INCLUDES}
)

target_compile_options(${TARGET} PUBLIC
"-std=c++17"
)
