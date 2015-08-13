#
# Licensed under Apache License v2. See LICENSE for more information.
#

include_directories(
        ${CPPUTEST_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/framework/public/include
        ${PROJECT_SOURCE_DIR}/utils/public/include
        ${PROJECT_SOURCE_DIR}/remote_services/examples/calculator_service/public/include
)

SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_remote_service_admin_dfi_tst_bundle")
SET(BUNDLE_VERSION "0.0.1")
SET_HEADERS("Bundle-Name: Apache Celix Remote Service Admin DFI test bundle")


bundle(rsa_dfi_tst_bundle
    SOURCES
        tst_activator.c

    FILES
        ${PROJECT_SOURCE_DIR}/remote_services/examples/calculator_service/public/include/org.apache.celix.calc.api.Calculator2.descriptor
)
target_link_libraries(rsa_dfi_tst_bundle ${CPPUTEST_LIBRARY} celix_framework celix_utils)