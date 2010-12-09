add_library(tracker_depman SHARED dependency_activator)
include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories("${PROJECT_SOURCE_DIR}/dependency_manager")
include_directories("../publisherService")
target_link_libraries(tracker_depman framework dependency_manager)

bundle(tracker_depman)
