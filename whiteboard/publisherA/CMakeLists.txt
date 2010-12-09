add_library(publisherA SHARED activator publisher)
include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories("../publisherService")
target_link_libraries(publisherA framework)

bundle(publisherA)
package(publisherA FILES ../publisherService/publisher.h)