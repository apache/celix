bundle(echo_server SOURCES echo_server_activator echo_server)
include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
target_link_libraries(echo_server celix_framework)
