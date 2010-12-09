add_library(receiver-2.0 SHARED activator serviceTest)
include_directories("${PROJECT_SOURCE_DIR}/celix" "${PROJECT_SOURCE_DIR}/dependency_manager") 
target_link_libraries(receiver-2.0 dependency_manager framework)

bundle(receiver-2.0)
package(receiver-2.0 FILES serviceTest.h)