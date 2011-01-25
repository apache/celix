add_library(echo_server SHARED echo_server_activator echo_server)
include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
target_link_libraries(echo_server framework)

bundle(echo_server)
