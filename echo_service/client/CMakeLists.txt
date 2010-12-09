add_library(echo_client SHARED echo_client_activator echo_client)
include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories("../server")
target_link_libraries(echo_client framework)

bundle(echo_client)
