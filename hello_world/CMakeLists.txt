add_library(hello_world SHARED activator)
include_directories("${PROJECT_SOURCE_DIR}/celix")
target_link_libraries(hello_world framework)

bundle(hello_world)
