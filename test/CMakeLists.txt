add_executable(main main)
include_directories("${PROJECT_SOURCE_DIR}/celix")
target_link_libraries(main framework)

add_definitions(-DUSE_FILE32API)