#
# Licensed under Apache License v2. See LICENSE for more information.
#

include_directories(
        ${CPPUTEST_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/framework/public/include
        ${PROJECT_SOURCE_DIR}/utils/public/include
)

SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_topology_manager_test_bundle")
SET(BUNDLE_VERSION "0.0.1")
SET_HEADERS("Bundle-Name: Apache Celix Remote Service Topology Manager test bundle")


bundle(topology_manager_test_bundle
    SOURCES
        tst_activator.c   
    FILES
         org.apache.celix.test.MyBundle.descriptor
)

target_link_libraries(topology_manager_test_bundle ${CPPUTEST_LIBRARY} celix_framework celix_utils)
