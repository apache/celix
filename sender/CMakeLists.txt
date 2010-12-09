add_library(sender SHARED test_activator listenerTest)
include_directories("${PROJECT_SOURCE_DIR}/celix" "${PROJECT_SOURCE_DIR}/dependency_manager")
include_directories("${PROJECT_SOURCE_DIR}/receiver")
target_link_libraries(sender framework dependency_manager)

bundle(sender)
package(sender)
