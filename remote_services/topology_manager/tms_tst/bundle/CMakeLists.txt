#
# Licensed under Apache License v2. See LICENSE for more information.
#

include_directories(
        ${CPPUTEST_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/framework/public/include
        ${PROJECT_SOURCE_DIR}/utils/public/include
)

add_bundle(topology_manager_test_bundle
    VERSION 0.0.1
    SOURCES
        tst_activator.c   
)
bundle_files(topology_manager_test_bundle
    org.apache.celix.test.MyBundle.descriptor
    DESTINATION .
)

target_link_libraries(topology_manager_test_bundle ${CPPUTEST_LIBRARY} celix_framework celix_utils)
