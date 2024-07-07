# Licensed to the Apache Software Foundation (ASF) under one
# or more contributor license agreements.  See the NOTICE file
# distributed with this work for additional information
# regarding copyright ownership.  The ASF licenses this file
# to you under the Apache License, Version 2.0 (the
# "License"); you may not use this file except in compliance
# with the License.  You may obtain a copy of the License at
# 
#   http://www.apache.org/licenses/LICENSE-2.0
# 
# Unless required by applicable law or agreed to in writing,
# software distributed under the License is distributed on an
# "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
# KIND, either express or implied.  See the License for the
# specific language governing permissions and limitations
# under the License.

celix_subproject(FRAMEWORK "Option to build the Celix Framework" ON)
if (FRAMEWORK)
    find_package(libuuid REQUIRED)
    option(FRAMEWORK_CURLINIT "Do CURL initialization in framework" ON)
    if (FRAMEWORK_CURLINIT)
        find_package(CURL REQUIRED)
    endif ()
    set(FRAMEWORK_SRC
            src/bundle.c src/bundle_archive.c src/celix_bundle_cache.c
            src/bundle_context.c src/bundle_revision.c
            src/framework.c src/manifest.c
            src/manifest_parser.c src/module.c
            src/service_reference.c src/service_registration.c
            src/service_registry.c src/service_tracker.c src/service_tracker_customizer.c
            src/celix_log.c src/celix_launcher.c
            src/celix_framework_factory.c
            src/dm_dependency_manager_impl.c src/dm_component_impl.c
            src/dm_service_dependency.c src/celix_libloader.c
            src/framework_bundle_lifecycle_handler.c
            src/celix_bundle_state.c
            src/celix_framework_utils.c
            src/celix_scheduled_event.c
            src/celix_framework_bundle.c
            src/celix_bundle_manifest.c
            )
    add_library(framework SHARED ${FRAMEWORK_SRC})

    set_target_properties(framework
            PROPERTIES
            "VERSION" "${CELIX_MAJOR}.${CELIX_MINOR}.${CELIX_MICRO}"
            "SOVERSION" ${CELIX_MAJOR}
            OUTPUT_NAME "celix_framework")
    celix_target_hide_symbols(framework)
    target_include_directories(framework PUBLIC
            $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
            )
    target_include_directories(framework PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include_deprecated)
    target_compile_options(framework PRIVATE -DUSE_FILE32API)
    target_compile_options(framework PRIVATE -Wno-deprecated-declarations) #note part of the api is deprecated, ignore this warning on own api
    if (NOT FRAMEWORK_CURLINIT)
        target_compile_definitions(framework PRIVATE CELIX_NO_CURLINIT)
    endif ()
    target_link_libraries(framework PUBLIC Celix::utils ${CELIX_OPTIONAL_EXTRA_LIBS})

    set(FRAMEWORK_DEPS libuuid::libuuid ${CMAKE_DL_LIBS})
    if (FRAMEWORK_CURLINIT)
        set(FRAMEWORK_DEPS ${FRAMEWORK_DEPS} CURL::libcurl)
    endif ()

    target_link_libraries(framework PRIVATE ${FRAMEWORK_DEPS})

    generate_export_header(framework
            BASE_NAME "CELIX_FRAMEWORK"
            EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/celix/gen/includes/framework/celix_framework_export.h")
    target_include_directories(framework PUBLIC $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/celix/gen/includes/framework>)
    celix_deprecated_utils_headers(framework)

    install(TARGETS framework EXPORT celix LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework
            INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/framework)
    install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/framework COMPONENT framework)
    install(DIRECTORY ${CMAKE_BINARY_DIR}/celix/gen/includes/framework/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/framework COMPONENT framework)

    #TODO #509, remove installation of deprecated headers
    install(DIRECTORY include_deprecated/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/framework COMPONENT framework)

    #Alias setup to match external usage
    add_library(Celix::framework ALIAS framework)

    if (ENABLE_TESTING AND EI_TESTS)
        add_subdirectory(error_injector)
    endif ()
    if (ENABLE_TESTING)
        add_library(framework_cut STATIC ${FRAMEWORK_SRC})

        target_include_directories(framework_cut PUBLIC
                ${CMAKE_CURRENT_LIST_DIR}/src
                ${CMAKE_CURRENT_LIST_DIR}/include
                ${CMAKE_BINARY_DIR}/celix/gen/includes/framework
                ${CMAKE_CURRENT_LIST_DIR}/include_deprecated
                )
        target_compile_options(framework_cut PRIVATE -DUSE_FILE32API)
        target_compile_options(framework_cut PRIVATE -Wno-deprecated-declarations) #note part of the api is deprecated, ignore this warning on own api
        if (NOT FRAMEWORK_CURLINIT)
            target_compile_definitions(framework_cut PRIVATE CELIX_NO_CURLINIT)
        endif ()
        target_link_libraries(framework_cut PUBLIC Celix::utils ${CELIX_OPTIONAL_EXTRA_LIBS} ${FRAMEWORK_DEPS})
        celix_deprecated_utils_headers(framework_cut)
        add_subdirectory(gtest)
    endif()

    add_subdirectory(benchmark)
endif ()
