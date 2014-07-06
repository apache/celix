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

celix_subproject(FRAMEWORK "Option to build the framework shared library" "ON" DEPS UTILS)
if (FRAMEWORK) 
    cmake_minimum_required(VERSION 2.6)
    
    find_package(ZLIB REQUIRED)
    find_package(UUID REQUIRED)

    include(CPackComponent)
    
    #cpack_add_component(framework
    #	DISPLAY_NAME Framework
    #    DESCRIPTION "The Apache Celix framework library"
    #    REQUIRED
    #)
    
    CELIX_ADD_COMPONENT(framework
    	DISPLAY_NAME Framework
        DESCRIPTION "The Apache Celix framework library"
        GROUP all
    )
    
    add_definitions(-DUSE_FILE32API)
    include_directories(${ZLIB_INCLUDE_DIR})
    include_directories(${UUID_INCLUDE_DIR})
    include_directories("private/include")
    include_directories("public/include")
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")

	if(WIN32)
	  set(IO private/src/iowin32.c)
	endif(WIN32)

    add_library(celix_framework SHARED
	 private/src/attribute.c private/src/bundle.c private/src/bundle_archive.c private/src/bundle_cache.c
	 private/src/bundle_context.c private/src/bundle_revision.c private/src/capability.c private/src/celix_errorcodes.c
	 private/src/filter.c private/src/framework.c private/src/manifest.c private/src/ioapi.c
	 private/src/manifest_parser.c private/src/miniunz.c private/src/module.c private/src/properties.c 
	 private/src/requirement.c private/src/resolver.c private/src/service_reference.c private/src/service_registration.c 
	 private/src/service_registry.c private/src/service_tracker.c private/src/service_tracker_customizer.c
	 private/src/unzip.c private/src/utils.c private/src/version.c private/src/version_range.c private/src/wire.c
	 private/src/celix_log.c

	 private/include/attribute.h public/include/framework_exports.h

	 public/include/framework.h public/include/properties.h public/include/bundle_context.h public/include/bundle.h
	 public/include/bundle_activator.h public/include/service_registration.h public/include/service_reference.h
	 public/include/bundle_archive.h public/include/utils.h public/include/module.h public/include/service_tracker.h
	 public/include/service_tracker_customizer.h
	 
		${IO}
	 
	)
	if(NOT APPLE)
      set(UUID ${UUID_LIBRARY})
    endif()
    target_link_libraries(celix_framework celix_utils ${UUID} ${ZLIB_LIBRARY} ${APR_LIBRARY})
    
    install(TARGETS celix_framework DESTINATION lib COMPONENT framework)
    FILE(GLOB files "public/include/*.h")
    INSTALL(FILES ${files} DESTINATION include/celix COMPONENT framework)
    INSTALL(DIRECTORY "${PROJECT_SOURCE_DIR}/cmake/" DESTINATION share/celix/cmake/modules COMPONENT framework) 

	celix_subproject(FRAMEWORK_TESTS "Option to build the framework tests" "OFF" DEPS)
    if (FRAMEWORK_TESTS)
    	find_package(CppUTest REQUIRED)
    	
	    include_directories(${CPPUTEST_INCLUDE_DIR})
	    include_directories(${CPPUTEST_EXT_INCLUDE_DIR})
	    
	    add_executable(version_test 
	        private/test/version_test.cpp 
	        private/src/version.c
	        private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(version_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY})
		
		add_executable(version_range_test 
		    private/test/version_range_test.cpp 
		    private/src/version_range.c
		    private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(version_range_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY})
		
		add_executable(utils_test 
		  private/test/utils_test.cpp private/src/utils.c
		  )
		target_link_libraries(utils_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(service_tracker_test 
			private/test/service_tracker_test.cpp 
			private/mock/bundle_context_mock.c
			private/mock/service_reference_mock.c 
			private/mock/service_tracker_customizer_mock.c
			private/src/service_tracker.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(service_tracker_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(service_tracker_customizer_test 
			private/test/service_tracker_customizer_test.cpp
			private/mock/service_reference_mock.c
			private/src/service_tracker_customizer.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c) 
		target_link_libraries(service_tracker_customizer_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(service_registry_test 
			private/test/service_registry_test.cpp
			private/mock/framework_mock.c
			private/mock/bundle_mock.c
			private/mock/filter_mock.c
			private/mock/service_reference_mock.c
			private/mock/service_registration_mock.c
			private/mock/properties_mock.c
			private/src/service_registry.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c) 
		target_link_libraries(service_registry_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(service_registration_test 
			private/test/service_registration_test.cpp
			private/mock/properties_mock.c
			private/mock/service_registry_mock.c
			private/src/service_registration.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(service_registration_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(service_reference_test 
			private/test/service_reference_test.cpp
			private/mock/service_registration_mock.c
			private/mock/service_registry_mock.c
			private/src/service_reference.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(service_reference_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(resolver_test 
			private/test/resolver_test.cpp
			private/mock/bundle_mock.c
			private/mock/requirement_mock.c
			private/mock/capability_mock.c
			private/mock/module_mock.c
			private/mock/wire_mock.c
			private/src/resolver.c)
		target_link_libraries(resolver_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
	    
		add_executable(requirement_test 
			private/test/requirement_test.cpp
			private/mock/attribute_mock.c
			private/mock/capability_mock.c
			private/mock/version_range_mock.c
			private/src/requirement.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(requirement_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(properties_test 
			private/test/properties_test.cpp
			private/src/utils.c
			private/src/properties.c)
		target_link_libraries(properties_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(module_test 
			private/test/module_test.cpp
			private/mock/bundle_mock.c
			private/mock/version_mock.c
			private/mock/manifest_mock.c
			private/mock/manifest_parser_mock.c
			private/mock/capability_mock.c
			private/mock/wire_mock.c
			private/src/module.c)
		target_link_libraries(module_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(manifest_test 
			private/test/manifest_test.cpp
			private/mock/properties_mock.c
			private/src/utils.c
			private/src/manifest.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(manifest_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(manifest_parser_test 
			private/test/manifest_parser_test.cpp
			private/mock/attribute_mock.c
			private/mock/manifest_mock.c
			private/mock/capability_mock.c
			private/mock/requirement_mock.c
			private/mock/version_mock.c
			private/src/utils.c
			private/src/manifest_parser.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(manifest_parser_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(framework_test 
			private/test/framework_test.cpp
			private/mock/properties_mock.c
			private/mock/resolver_mock.c
			private/mock/service_reference_mock.c
			private/mock/service_registry_mock.c
			private/mock/service_registration_mock.c
			private/mock/filter_mock.c
			private/mock/bundle_mock.c
			private/mock/bundle_context_mock.c
			private/mock/module_mock.c
			private/mock/bundle_archive_mock.c
			private/mock/bundle_revision_mock.c
			private/mock/bundle_cache_mock.c
			private/mock/manifest_mock.c
			private/src/utils.c
			private/src/celix_errorcodes.c
			private/src/celix_log.c
			private/src/framework.c)
		target_link_libraries(framework_test ${APR_LIBRARY} ${APRUTIL_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} ${UUID} celix_utils)
		
		add_executable(filter_test 
			private/test/filter_test.cpp
			private/mock/properties_mock.c
			private/src/filter.c)
		target_link_libraries(filter_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(capability_test 
			private/test/capability_test.cpp
			private/mock/attribute_mock.c
			private/mock/version_mock.c
			private/src/capability.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(capability_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(bundle_test 
			private/test/bundle_test.cpp
			private/mock/framework_mock.c
			private/mock/module_mock.c
			private/mock/bundle_archive_mock.c
			private/mock/bundle_revision_mock.c
			private/mock/resolver_mock.c
			private/mock/version_mock.c
			private/src/utils.c
			private/src/bundle.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(bundle_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(bundle_revision_test 
			private/test/bundle_revision_test.cpp
			private/mock/miniunz_mock.c
			private/mock/manifest_mock.c
			private/src/bundle_revision.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(bundle_revision_test ${ZLIB_LIBRARY} ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(bundle_context_test 
			private/test/bundle_context_test.cpp
			private/mock/bundle_mock.c
			private/mock/framework_mock.c
			private/src/bundle_context.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(bundle_context_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(bundle_cache_test 
			private/test/bundle_cache_test.cpp
			private/mock/bundle_archive_mock.c
			private/mock/properties_mock.c
			private/src/bundle_cache.c
			private/src/celix_errorcodes.c
			private/src/celix_log.c)
		target_link_libraries(bundle_cache_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(bundle_archive_test 
			private/test/bundle_archive_test.cpp
			private/mock/bundle_revision_mock.c
			private/src/bundle_archive.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(bundle_archive_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
		
		add_executable(attribute_test 
			private/test/attribute_test.cpp
			private/src/attribute.c
			private/src/celix_errorcodes.c
            private/src/celix_log.c)
		target_link_libraries(attribute_test ${APR_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils)
	    
	    ADD_TARGET_FOR_TEST(version_test)
		ADD_TARGET_FOR_TEST(version_range_test)
		ADD_TARGET_FOR_TEST(service_tracker_test)
		ADD_TARGET_FOR_TEST(service_tracker_customizer_test)
		ADD_TARGET_FOR_TEST(service_registry_test)
		ADD_TARGET_FOR_TEST(service_registration_test)
		ADD_TARGET_FOR_TEST(service_reference_test)
		ADD_TARGET_FOR_TEST(resolver_test)
		ADD_TARGET_FOR_TEST(requirement_test)
		ADD_TARGET_FOR_TEST(properties_test)
		ADD_TARGET_FOR_TEST(module_test)
		ADD_TARGET_FOR_TEST(manifest_test)
		ADD_TARGET_FOR_TEST(manifest_parser_test)
		ADD_TARGET_FOR_TEST(framework_test)
		ADD_TARGET_FOR_TEST(filter_test)
		ADD_TARGET_FOR_TEST(capability_test)
		ADD_TARGET_FOR_TEST(bundle_test)
		ADD_TARGET_FOR_TEST(bundle_revision_test)
		ADD_TARGET_FOR_TEST(bundle_context_test)
		ADD_TARGET_FOR_TEST(bundle_cache_test)
		ADD_TARGET_FOR_TEST(bundle_archive_test)
		ADD_TARGET_FOR_TEST(attribute_test)
	    
		SETUP_TARGET_FOR_COVERAGE(version_test_c version_test ${CMAKE_BINARY_DIR}/coverage/version_test)
		SETUP_TARGET_FOR_COVERAGE(version_range_test_c version_range_test ${CMAKE_BINARY_DIR}/coverage/version_range_test)
		SETUP_TARGET_FOR_COVERAGE(service_tracker_test_c service_tracker_test ${CMAKE_BINARY_DIR}/coverage/service_tracker_test)
		SETUP_TARGET_FOR_COVERAGE(service_tracker_customizer_test_c service_tracker_customizer_test ${CMAKE_BINARY_DIR}/coverage/service_tracker_customizer_test)
		SETUP_TARGET_FOR_COVERAGE(service_registry_test_c service_registry_test ${CMAKE_BINARY_DIR}/coverage/service_registry_test)
		SETUP_TARGET_FOR_COVERAGE(service_registration_test_c service_registration_test ${CMAKE_BINARY_DIR}/coverage/service_registration_test)
		SETUP_TARGET_FOR_COVERAGE(service_reference_test_c service_reference_test ${CMAKE_BINARY_DIR}/coverage/service_reference_test)
		SETUP_TARGET_FOR_COVERAGE(resolver_test_c resolver_test ${CMAKE_BINARY_DIR}/coverage/resolver_test)
		SETUP_TARGET_FOR_COVERAGE(requirement_test_c requirement_test ${CMAKE_BINARY_DIR}/coverage/requirement_test)
		SETUP_TARGET_FOR_COVERAGE(properties_test_c properties_test ${CMAKE_BINARY_DIR}/coverage/properties_test)
		SETUP_TARGET_FOR_COVERAGE(module_test_c module_test ${CMAKE_BINARY_DIR}/coverage/module_test)
		SETUP_TARGET_FOR_COVERAGE(manifest_test_c manifest_test ${CMAKE_BINARY_DIR}/coverage/manifest_test)
		SETUP_TARGET_FOR_COVERAGE(manifest_parser_test_c manifest_parser_test ${CMAKE_BINARY_DIR}/coverage/manifest_parser_test)
		SETUP_TARGET_FOR_COVERAGE(framework_test_c framework_test ${CMAKE_BINARY_DIR}/coverage/framework_test)
		SETUP_TARGET_FOR_COVERAGE(filter_test_c filter_test ${CMAKE_BINARY_DIR}/coverage/filter_test)
		SETUP_TARGET_FOR_COVERAGE(capability_test_c capability_test ${CMAKE_BINARY_DIR}/coverage/capability_test)
		SETUP_TARGET_FOR_COVERAGE(bundle_test_c bundle_test ${CMAKE_BINARY_DIR}/coverage/bundle_test)
		SETUP_TARGET_FOR_COVERAGE(bundle_revision_test_c bundle_revision_test ${CMAKE_BINARY_DIR}/coverage/bundle_revision_test)
		SETUP_TARGET_FOR_COVERAGE(bundle_context_test_c bundle_context_test ${CMAKE_BINARY_DIR}/coverage/bundle_context_test)
		SETUP_TARGET_FOR_COVERAGE(bundle_cache_test_c bundle_cache_test ${CMAKE_BINARY_DIR}/coverage/bundle_cache_test)
		SETUP_TARGET_FOR_COVERAGE(bundle_archive_test_c bundle_archive_test ${CMAKE_BINARY_DIR}/coverage/bundle_archive_test)
		SETUP_TARGET_FOR_COVERAGE(attribute_test_c attribute_test ${CMAKE_BINARY_DIR}/coverage/attribute_test)
		
		add_subdirectory(private/integration-test)
	endif (FRAMEWORK_TESTS)
endif (FRAMEWORK)
