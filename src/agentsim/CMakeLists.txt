set(SOURCES
"agent.cpp"
"combination_reaction.cpp"
"readdypkg.cpp"
"simpleinteraction.cpp"
"simplemove.cpp"
"simulation.cpp"
"statechange_reaction.cpp"
)

add_library(${TARGET} SHARED ${SOURCES})
