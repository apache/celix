add_library(shell SHARED shell command ps_command start_command stop_command)
include_directories("${PROJECT_SOURCE_DIR}/celix")
target_link_libraries(shell framework)

bundle(shell)
package(shell FILES shell.h command.h)