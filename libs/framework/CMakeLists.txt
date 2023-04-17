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

find_package(ZLIB REQUIRED)
find_package(libuuid REQUIRED)
find_package(CURL REQUIRED)

set(SOURCES
        src/attribute.c src/bundle.c src/bundle_archive.c src/celix_bundle_cache.c
        src/bundle_context.c src/bundle_revision.c src/celix_errorcodes.c
        src/framework.c src/manifest.c
        src/manifest_parser.c src/module.c
        src/requirement.c src/capability.c src/resolver.c src/wire.c
        src/service_reference.c src/service_registration.c
        src/service_registry.c src/service_tracker.c src/service_tracker_customizer.c
        src/celix_log.c src/celix_launcher.c
        src/celix_framework_factory.c
        src/dm_dependency_manager_impl.c src/dm_component_impl.c
        src/dm_service_dependency.c src/celix_libloader.c
        src/framework_bundle_lifecycle_handler.c
        src/celix_bundle_state.c
        src/celix_framework_utils.c
        src/celix_module_private.h)
add_library(framework_obj OBJECT ${SOURCES})

set(CELIX_FRAMEWORK_CMAKE_CONFIGURE_ALT_OPTION ON)
if (CELIX_FRAMEWORK_CMAKE_CONFIGURE_ALT_OPTION)
    function (celix_configure_framework_target)
        list(GET ARGN 0 FRAMEWORK_TARGET_NAME)

        target_include_directories(${FRAMEWORK_TARGET_NAME}
            PRIVATE
                ${CMAKE_CURRENT_LIST_DIR}/include_deprecated
            PUBLIC
                $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
                $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/celix/gen/includes/${FRAMEWORK_TARGET_NAME}>
                $<INSTALL_INTERFACE:include/celix/framework>)
        target_compile_options(${FRAMEWORK_TARGET_NAME} PRIVATE -DUSE_FILE32API)
        target_compile_options(${FRAMEWORK_TARGET_NAME} PRIVATE -Wno-deprecated-declarations) #note part of the api is deprecated, ignore this warning on own api
        target_link_libraries(${FRAMEWORK_TARGET_NAME} PUBLIC Celix::utils ${CELIX_OPTIONAL_EXTRA_LIBS})
        target_link_libraries(${FRAMEWORK_TARGET_NAME} PUBLIC libuuid::libuuid CURL::libcurl ZLIB::ZLIB)
        target_link_libraries(${FRAMEWORK_TARGET_NAME} PRIVATE ${CMAKE_DL_LIBS})
        set_target_properties(${FRAMEWORK_TARGET_NAME} PROPERTIES C_VISIBILITY_PRESET hidden)
        celix_deprecated_utils_headers(${FRAMEWORK_TARGET_NAME})

        generate_export_header(${FRAMEWORK_TARGET_NAME}
                BASE_NAME "CELIX_FRAMEWORK"
                EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/celix/gen/includes/${FRAMEWORK_TARGET_NAME}/celix_framework_export.h")
    endfunction ()

    celix_configure_framework_target(framework_obj)

    add_library(framework SHARED $<TARGET_OBJECTS:framework_obj>)
    celix_configure_framework_target(framework)
    set_target_properties(framework
        PROPERTIES
            "VERSION" "${CELIX_MAJOR}.${CELIX_MINOR}.${CELIX_MICRO}"
            "SOVERSION" ${CELIX_MAJOR}
            OUTPUT_NAME "celix_framework")
else ()
    target_include_directories(framework_obj PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        $<INSTALL_INTERFACE:include/celix/framework>
    )
    target_include_directories(framework_obj PRIVATE ${CMAKE_CURRENT_LIST_DIR}/include_deprecated)
    target_compile_options(framework_obj PRIVATE -DUSE_FILE32API)
    target_compile_options(framework_obj PRIVATE -Wno-deprecated-declarations) #note part of the api is deprecated, ignore this warning on own api
    target_link_libraries(framework_obj PUBLIC Celix::utils ${CELIX_OPTIONAL_EXTRA_LIBS})
    target_link_libraries(framework_obj PUBLIC libuuid::libuuid CURL::libcurl ZLIB::ZLIB)
    target_link_libraries(framework_obj PRIVATE ${CMAKE_DL_LIBS})
    #Note visibility preset is also needed on OBJECT libraries, to work correctly
    set_target_properties(framework_obj PROPERTIES C_VISIBILITY_PRESET hidden)
    celix_deprecated_utils_headers(framework_obj)

    generate_export_header(framework_obj
            BASE_NAME "CELIX_FRAMEWORK"
            EXPORT_FILE_NAME "${CMAKE_BINARY_DIR}/celix/gen/includes/framework/celix_framework_export.h")
    target_include_directories(utils_obj PUBLIC
            $<BUILD_INTERFACE:${CMAKE_BINARY_DIR}/celix/gen/includes/framework>)

    add_library(framework SHARED)
    target_link_libraries(framework PUBLIC framework_obj)
    set_target_properties(framework
        PROPERTIES
            C_VISIBILITY_PRESET hidden
            "VERSION" "${CELIX_MAJOR}.${CELIX_MINOR}.${CELIX_MICRO}"
            "SOVERSION" ${CELIX_MAJOR}
            OUTPUT_NAME "celix_framework")

    celix_deprecated_utils_headers(framework)
endif ()

# By omitting OBJECTS DESTINATION, object files will NOT be installed.
install(TARGETS framework framework_obj EXPORT celix LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/framework)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/framework COMPONENT framework)
install(DIRECTORY include_deprecated/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/framework COMPONENT framework)
install(DIRECTORY ${CMAKE_BINARY_DIR}/celix/gen/includes/framework/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix/framework COMPONENT framework)

#Alias setup to match external usage
add_library(Celix::framework ALIAS framework)
add_library(Celix::framework_obj ALIAS framework_obj)

if (ENABLE_TESTING AND CELIX_CXX17) #framework tests are C++17
    add_subdirectory(gtest)
endif()

add_subdirectory(benchmark)
