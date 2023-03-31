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
target_include_directories(framework_obj PUBLIC
        $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/include>
        )
target_compile_options(framework_obj PRIVATE -DUSE_FILE32API)
target_compile_options(framework_obj PRIVATE -Wno-deprecated-declarations) #note part of the api is deprecated, ignore this warning on own api
target_link_libraries(framework_obj PUBLIC Celix::utils ${CELIX_OPTIONAL_EXTRA_LIBS})
target_link_libraries(framework_obj PUBLIC libuuid::libuuid CURL::libcurl ZLIB::ZLIB)
target_link_libraries(framework_obj PRIVATE ${CMAKE_DL_LIBS})
celix_deprecated_utils_headers(framework_obj)

add_library(framework SHARED)
target_link_libraries(framework PUBLIC framework_obj)
set_target_properties(framework PROPERTIES OUTPUT_NAME "celix_framework")
set_target_properties(framework PROPERTIES "SOVERSION" ${CELIX_MAJOR})

celix_deprecated_utils_headers(framework)

# By omitting OBJECTS DESTINATION, object files will NOT be installed.
install(TARGETS framework framework_obj EXPORT celix LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework
        INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/celix COMPONENT framework)

#Alias setup to match external usage
add_library(Celix::framework ALIAS framework)
add_library(Celix::framework_obj ALIAS framework_obj)

if (ENABLE_TESTING AND CELIX_CXX17) #framework tests are C++17
    add_subdirectory(gtest)
endif()

add_subdirectory(benchmark)
