#
# Licensed under Apache License v2. See LICENSE for more information.
#

include_directories(
        ${PROJECT_SOURCE_DIR}/launcher/public/include
        ${CPPUTEST_INCLUDE_DIR}
        ${PROJECT_SOURCE_DIR}/framework/public/include
        ${PROJECT_SOURCE_DIR}/utils/public/include
)


bundle(rsa_dfi_tst_bundle
    SOURCES
        tst_activator.c

    FILES
        ${PROJECT_SOURCE_DIR}/remote_services/examples/calculator_service/public/include/org.apache.celix.calc.api.Calculator2.descriptor
)
target_link_libraries(rsa_dfi_tst_bundle ${CPPUTEST_LIBRARY} celix_framework celix_utils)