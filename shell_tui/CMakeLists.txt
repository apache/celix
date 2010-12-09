add_library(shell_tui SHARED shell_tui)
include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories("${PROJECT_SOURCE_DIR}/shell")
target_link_libraries(shell_tui framework)

bundle(shell_tui)
package(shell_tui)