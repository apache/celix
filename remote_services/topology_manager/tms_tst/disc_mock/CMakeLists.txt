#
# Licensed under Apache License v2. See LICENSE for more information.
#

include_directories(
        ${CPPUTEST_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/framework/public/include
        ${PROJECT_SOURCE_DIR}/utils/public/include
        ${PROJECT_SOURCE_DIR}/remote_services/discovery/private/include
)


add_bundle(topology_manager_disc_mock_bundle
    VERSION 0.0.1
    SOURCES
        disc_mock_activator.c
        disc_mock_service.c

)
target_link_libraries(topology_manager_disc_mock_bundle ${CPPUTEST_LIBRARY} celix_framework celix_utils)
