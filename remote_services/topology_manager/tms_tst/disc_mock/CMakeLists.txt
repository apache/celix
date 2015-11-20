#
# Licensed under Apache License v2. See LICENSE for more information.
#

include_directories(
        ${CPPUTEST_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/framework/public/include
        ${PROJECT_SOURCE_DIR}/utils/public/include
        ${PROJECT_SOURCE_DIR}/remote_services/discovery/private/include
)

SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_topology_manager_scoped_disc_mock_bundle")
SET(BUNDLE_VERSION "0.0.1")
SET_HEADERS("Bundle-Name: Apache Celix Topology manager discovery mock bundle")


bundle(topology_manager_disc_mock_bundle
    SOURCES
        disc_mock_activator.c
        disc_mock_service.c

)
target_link_libraries(topology_manager_disc_mock_bundle ${CPPUTEST_LIBRARY} celix_framework celix_utils)
