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
    find_package(CURL REQUIRED)

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
    include_directories(${CURL_INCLUDE_DIR})
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
	 private/src/celix_log.c private/src/celix_launcher.c

	 private/include/attribute.h public/include/framework_exports.h

	 public/include/framework.h public/include/properties.h public/include/bundle_context.h public/include/bundle.h
	 public/include/bundle_activator.h public/include/service_registration.h public/include/service_reference.h
	 public/include/bundle_archive.h public/include/utils.h public/include/module.h public/include/service_tracker.h
	 public/include/service_tracker_customizer.h public/include/requirement.h
	 
		${IO}
	 
	)
	if(NOT APPLE)
      set(UUID ${UUID_LIBRARY})
    endif()
    target_link_libraries(celix_framework celix_utils ${UUID} ${ZLIB_LIBRARY} ${CURL_LIBRARIES})

    install(TARGETS celix_framework DESTINATION lib COMPONENT framework)
    FILE(GLOB files "public/include/*.h")
    INSTALL(FILES ${files} DESTINATION include/celix COMPONENT framework)
    INSTALL(DIRECTORY "${PROJECT_SOURCE_DIR}/cmake/" DESTINATION share/celix/cmake/modules COMPONENT framework) 


    if (ENABLE_TESTING)
        find_package(CppUTest REQUIRED)
        include_directories(${CPPUTEST_INCLUDE_DIR})
        add_subdirectory(tst)
    endif()


	celix_subproject(FRAMEWORK_TESTS "Option to build the framework tests" "OFF" DEPS)
    if (ENABLE_TESTING AND FRAMEWORK_TESTS)
    	find_package(CppUTest REQUIRED)
    	
	    include_directories(${CPPUTEST_INCLUDE_DIR})
	    include_directories(${CPPUTEST_EXT_INCLUDE_DIR})
	    
        add_executable(attribute_test 
            private/test/attribute_test.cpp
            private/src/attribute.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(attribute_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
        
        add_executable(bundle_archive_test 
            private/test/bundle_archive_test.cpp
            private/mock/bundle_revision_mock.c
            private/src/bundle_archive.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(bundle_archive_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
        
        add_executable(bundle_cache_test 
            private/test/bundle_cache_test.cpp
            private/mock/bundle_archive_mock.c
            private/mock/properties_mock.c
            private/src/bundle_cache.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(bundle_cache_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(bundle_context_test 
            private/test/bundle_context_test.cpp
            private/mock/bundle_mock.c
            private/mock/framework_mock.c
            private/src/bundle_context.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(bundle_context_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(bundle_revision_test 
            private/test/bundle_revision_test.cpp
            private/mock/miniunz_mock.c
            private/mock/manifest_mock.c
            private/src/bundle_revision.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(bundle_revision_test ${ZLIB_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
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
        target_link_libraries(bundle_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(capability_test 
            private/test/capability_test.cpp
            private/mock/attribute_mock.c
            private/mock/version_mock.c
            private/src/capability.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(capability_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(filter_test 
            private/test/filter_test.cpp
            private/mock/properties_mock.c
            private/src/filter.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(filter_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
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
        target_link_libraries(framework_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} ${UUID} celix_utils pthread dl)
    
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
        target_link_libraries(manifest_parser_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(manifest_test 
            private/test/manifest_test.cpp
            private/mock/properties_mock.c
            private/src/utils.c
            private/src/manifest.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(manifest_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(module_test 
            private/test/module_test.cpp
            private/mock/bundle_mock.c
            private/mock/version_mock.c
            private/mock/manifest_mock.c
            private/mock/manifest_parser_mock.c
            private/mock/capability_mock.c
            private/mock/requirement_mock.c
            private/mock/wire_mock.c
            private/src/module.c)
        target_link_libraries(module_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(properties_test 
            private/test/properties_test.cpp
            private/src/utils.c
            private/src/properties.c)
        target_link_libraries(properties_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
            
        add_executable(requirement_test 
            private/test/requirement_test.cpp
            private/mock/attribute_mock.c
            private/mock/capability_mock.c
            private/mock/version_range_mock.c
            private/src/requirement.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(requirement_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(resolver_test 
            private/test/resolver_test.cpp
            private/mock/bundle_mock.c
            private/mock/requirement_mock.c
            private/mock/capability_mock.c
            private/mock/module_mock.c
            private/mock/wire_mock.c
            private/src/resolver.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(resolver_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(service_reference_test 
            private/test/service_reference_test.cpp
            private/mock/properties_mock.c
            private/mock/service_registration_mock.c
            private/mock/service_registry_mock.c
            private/src/service_reference.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(service_reference_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
            add_executable(service_registration_test 
            private/test/service_registration_test.cpp
            private/mock/properties_mock.c
            private/mock/service_registry_mock.c
            private/src/service_registration.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(service_registration_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
            
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
        target_link_libraries(service_registry_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(service_tracker_customizer_test 
            private/test/service_tracker_customizer_test.cpp
            private/mock/service_reference_mock.c
            private/src/service_tracker_customizer.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c) 
        target_link_libraries(service_tracker_customizer_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(service_tracker_test 
            private/test/service_tracker_test.cpp 
            private/mock/bundle_context_mock.c
            private/mock/service_reference_mock.c 
            private/mock/service_tracker_customizer_mock.c
            private/src/service_tracker.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(service_tracker_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
        
        add_executable(utils_test 
            private/test/utils_test.cpp private/src/utils.c)
        target_link_libraries(utils_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
            
        add_executable(version_range_test 
            private/test/version_range_test.cpp 
            private/src/version_range.c
            private/mock/version_mock.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(version_range_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY})
	    
        add_executable(version_test 
            private/test/version_test.cpp 
            private/src/version.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c)
        target_link_libraries(version_test ${CPPUTEST_LIBRARY})
	    
	    add_executable(wire_test
	       private/mock/requirement_mock.c
	       private/mock/capability_mock.c
	       private/mock/module_mock.c
            private/src/celix_errorcodes.c
            private/src/celix_log.c
            private/src/wire.c
	        private/test/wire_test.cpp) 
		target_link_libraries(wire_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY})
		
		set_target_properties(wire_test PROPERTIES COMPILE_FLAGS "-include ${CPPUTEST_INCLUDE_DIR}/CppUTest/MemoryLeakDetectorMallocMacros.h -include ${CPPUTEST_INCLUDE_DIR}/CppUTest/MemoryLeakDetectorNewMacros.h")
		
        add_test(NAME framework_test_1 COMMAND attribute_test)
        add_test(NAME framework_test_2 COMMAND bundle_archive_test)
        #ADD_TARGET_FOR_TEST(bundle_cache_test)
        add_test(NAME framework_test_3 COMMAND bundle_context_test)
        add_test(NAME framework_test_4 COMMAND bundle_revision_test)
        add_test(NAME framework_test_5 COMMAND bundle_test)
        add_test(NAME framework_test_6 COMMAND capability_test)
        add_test(NAME framework_test_7 COMMAND filter_test)
        add_test(NAME framework_test_8 COMMAND framework_test)
        add_test(NAME framework_test_9 COMMAND manifest_parser_test)
        add_test(NAME framework_test_10 COMMAND manifest_test)
        add_test(NAME framework_test_11 COMMAND module_test)
        add_test(NAME framework_test_12 COMMAND properties_test)
        add_test(NAME framework_test_13 COMMAND requirement_test)
        add_test(NAME framework_test_14 COMMAND resolver_test)
        add_test(NAME framework_test_15 COMMAND service_reference_test)
        add_test(NAME framework_test_16 COMMAND service_registration_test)
        add_test(NAME framework_test_17 COMMAND service_registry_test)
        add_test(NAME framework_test_18 COMMAND service_tracker_customizer_test)
        add_test(NAME framework_test_19 COMMAND service_tracker_test)
        add_test(NAME framework_test_20 COMMAND version_range_test)
        add_test(NAME framework_test_21 COMMAND version_test)
	    add_test(NAME framework_test_22 COMMAND wire_test)
	    
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
		SETUP_TARGET_FOR_COVERAGE(wire_test_c wire_test ${CMAKE_BINARY_DIR}/coverage/wire_test)
		
	endif (ENABLE_TESTING AND FRAMEWORK_TESTS)
endif (FRAMEWORK)
