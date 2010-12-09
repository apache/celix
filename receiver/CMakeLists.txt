add_library(receiver SHARED activator serviceTest)
include_directories("${PROJECT_SOURCE_DIR}/celix")
target_link_libraries(receiver framework)

bundle(receiver)
package(receiver FILES serviceTest.h)