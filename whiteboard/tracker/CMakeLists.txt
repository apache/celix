add_library(tracker SHARED activator)
include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories("../publisherService")
target_link_libraries(tracker framework)

bundle(tracker)
