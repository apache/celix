aux_source_directory(. SRC)
add_library(framework SHARED ${SRC})
target_link_libraries(framework m dl z pthread)

set(dir .)
file (GLOB headers "${dir}/*.h")

install(FILES ${headers} DESTINATION include COMPONENT framework)
install(TARGETS framework LIBRARY DESTINATION lib COMPONENT framework)

add_definitions(-DUSE_FILE32API)