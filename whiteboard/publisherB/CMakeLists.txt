add_library(publisherB SHARED activator publisher)
include_directories("${PROJECT_SOURCE_DIR}/celix")
include_directories("../publisherService")
target_link_libraries(publisherB framework)

bundle(publisherB)
package(publisherB FILES ../publisherService/publisher.h)