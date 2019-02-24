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

    #TODO setup intall framework
    #CELIX_ADD_COMPONENT(framework
    #	DISPLAY_NAME Framework
    #    DESCRIPTION "The Apache Celix framework library"
    #    GROUP all
    #)
    
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
	 private/src/manifest_parser.c private/src/miniunz.c private/src/module.c  
	 private/src/requirement.c private/src/resolver.c private/src/service_reference.c private/src/service_registration.c 
	 private/src/service_registry.c private/src/service_tracker.c private/src/service_tracker_customizer.c
	 private/src/unzip.c private/src/wire.c
	 private/src/celix_log.c private/src/celix_launcher.c

	 private/include/attribute.h public/include/framework_exports.h

	 public/include/bundle_context.h public/include/bundle.h
	 public/include/bundle_activator.h public/include/service_registration.h public/include/service_reference.h
	 public/include/bundle_archive.h public/include/module.h public/include/service_tracker.h
	 public/include/service_tracker_customizer.h public/include/requirement.h
	 
		${IO}
	 
	)
    set_target_properties(celix_framework PROPERTIES "SOVERSION" 2)
	if(NOT APPLE)
      set(UUID ${UUID_LIBRARY})
    endif()
    target_link_libraries(celix_framework celix_utils ${UUID} ${ZLIB_LIBRARY} ${CURL_LIBRARIES})

    install(TARGETS celix_framework DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework)
    FILE(GLOB files "public/include/*.h")
    INSTALL(FILES ${files} DESTINATION include/celix COMPONENT framework)
    INSTALL(DIRECTORY "${PROJECT_SOURCE_DIR}/cmake/" DESTINATION share/celix/cmake/modules COMPONENT framework) 


    if (ENABLE_TESTING)
        find_package(CppUTest REQUIRED)
        include_directories(${CPPUTEST_INCLUDE_DIR})

        find_package(Gtest REQUIRED)
        include_directories(${GTEST_INCLUDE_DIR})

        add_subdirectory(tst)
    endif()

set(ENABLE_TESTING ON)
set(FRAMEWORK_TESTS ON)

    if (ENABLE_TESTING AND FRAMEWORK_TESTS)
    	find_package(CppUTest REQUIRED)
    	
	    include_directories(${CPPUTEST_INCLUDE_DIR})
	    include_directories(${CPPUTEST_EXT_INCLUDE_DIR})
	    include_directories(${PROJECT_SOURCE_DIR}/utils/private/include)
	    
        add_executable(attribute_test 
            private/test/attribute_test.cpp
            private/src/attribute.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(attribute_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
        
#        add_executable(bundle_archive_test 
#            private/mock/celix_log_mock.c
#            private/test/bundle_archive_test.cpp
#            private/src/bundle_revision.c
#            private/src/manifest.c
#            private/src/miniunz.c
#            private/src/unzip.c
#            private/src/ioapi.c
#            private/src/properties.c
#            private/src/bundle_archive.c
#            private/src/celix_errorcodes.c
#            private/src/utils.c)
#        target_link_libraries(bundle_archive_test celix_utils ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} ${ZLIB_LIBRARY} pthread)
        
        
        add_executable(bundle_cache_test 
            private/test/bundle_cache_test.cpp
            private/mock/bundle_archive_mock.c
            private/mock/properties_mock.c
            private/src/bundle_cache.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(bundle_cache_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
        add_executable(bundle_context_test 
            private/test/bundle_context_test.cpp
            private/mock/bundle_mock.c
            private/mock/framework_mock.c
            private/mock/service_registry_mock.c
            private/src/bundle_context.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(bundle_context_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
        add_executable(bundle_revision_test 
            private/test/bundle_revision_test.cpp
            private/mock/miniunz_mock.c
            private/mock/manifest_mock.c
            private/src/bundle_revision.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(bundle_revision_test ${ZLIB_LIBRARY} ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
        add_executable(bundle_test 
            private/test/bundle_test.cpp
            private/mock/framework_mock.c
            private/mock/module_mock.c
            private/mock/bundle_archive_mock.c
            private/mock/bundle_revision_mock.c
            private/mock/resolver_mock.c
            private/mock/version_mock.c
            private/src/bundle.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(bundle_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)

	    add_executable(thing_test
          private/test/thing_test.cpp)
        target_link_libraries(thing_test ${GTEST_LIBRARY} celix_utils pthread)

        add_executable(capability_test 
            private/test/capability_test.cpp
            private/mock/attribute_mock.c
            private/mock/version_mock.c
            private/src/capability.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(capability_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
        
        add_executable(celix_errorcodes_test 
            private/test/celix_errorcodes_test.cpp
            private/src/celix_errorcodes.c)
	   	target_link_libraries(celix_errorcodes_test ${CPPUTEST_LIBRARY})
	    
        add_executable(filter_test 
            private/test/filter_test.cpp
            private/src/filter.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(filter_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
        add_executable(framework_test 
            private/test/framework_test.cpp
            #private/mock/properties_mock.c
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
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c
            private/src/framework.c)
        target_link_libraries(framework_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} ${UUID} celix_utils pthread dl)
    
        add_executable(manifest_parser_test 
            private/test/manifest_parser_test.cpp
            private/mock/manifest_mock.c
            private/mock/version_mock.c
            private/mock/version_range_mock.c
            private/src/attribute.c
            private/src/capability.c
            private/src/requirement.c
            private/src/manifest_parser.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(manifest_parser_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
        add_executable(manifest_test 
            private/test/manifest_test.cpp
            private/mock/properties_mock.c
            private/src/manifest.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(manifest_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
#        add_executable(module_test 
#            private/test/module_test.cpp
#            private/mock/bundle_mock.c
#            private/mock/version_mock.c
#            private/mock/manifest_mock.c
#            private/mock/manifest_parser_mock.c
#            private/mock/capability_mock.c
#            private/mock/requirement_mock.c
#            private/mock/wire_mock.c
#            private/src/module.c)
#        target_link_libraries(module_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
        add_executable(requirement_test 
            private/test/requirement_test.cpp
            private/mock/attribute_mock.c
            private/mock/capability_mock.c
            private/mock/version_range_mock.c
            private/src/requirement.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(requirement_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
#        add_executable(resolver_test 
#            private/test/resolver_test.cpp
#            private/mock/bundle_mock.c
#            private/mock/requirement_mock.c
#            private/mock/capability_mock.c
#            private/mock/manifest_parser_mock.c
#            private/mock/version_mock.c
#            private/src/wire.c
#            private/src/module.c
#            private/src/resolver.c
#            private/src/celix_errorcodes.c
#            private/mock/celix_log_mock.c)
#        target_link_libraries(resolver_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
        add_executable(service_reference_test 
            private/test/service_reference_test.cpp
            private/mock/properties_mock.c
            private/mock/service_registration_mock.c
            private/mock/service_registry_mock.c
            private/src/service_reference.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(service_reference_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
	     add_executable(service_registration_test 
            private/test/service_registration_test.cpp
            private/mock/service_registry_mock.c
            private/src/service_registration.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
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
            private/mock/module_mock.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c)
        target_link_libraries(service_registry_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
        add_executable(service_tracker_customizer_test 
            private/test/service_tracker_customizer_test.cpp
            private/mock/service_reference_mock.c
            private/src/service_tracker_customizer.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c) 
        target_link_libraries(service_tracker_customizer_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
	    
#        add_executable(service_tracker_test 
#            private/test/service_tracker_test.cpp 
#            private/mock/bundle_context_mock.c
#            private/mock/service_reference_mock.c 
#            private/mock/service_tracker_customizer_mock.c
#            private/src/service_tracker.c
#            private/src/celix_errorcodes.c
#            private/mock/celix_log_mock.c)
#        target_link_libraries(service_tracker_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)
        
	    
	    add_executable(wire_test
	       private/mock/requirement_mock.c
	       private/mock/capability_mock.c
	       private/mock/module_mock.c
            private/src/celix_errorcodes.c
            private/mock/celix_log_mock.c
            private/src/wire.c
	        private/test/wire_test.cpp) 
		target_link_libraries(wire_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY})
		
		configure_file(private/resources-test/manifest_sections.txt ${CMAKE_BINARY_DIR}/framework/resources-test/manifest_sections.txt COPYONLY)
		configure_file(private/resources-test/manifest.txt ${CMAKE_BINARY_DIR}/framework/resources-test/manifest.txt COPYONLY)
				
		#set_target_properties(wire_test PROPERTIES COMPILE_FLAGS "-include ${CPPUTEST_INCLUDE_DIR}/CppUTest/MemoryLeakDetectorMallocMacros.h -include ${CPPUTEST_INCLUDE_DIR}/CppUTest/MemoryLeakDetectorNewMacros.h")
			
        add_test(NAME attribute_test COMMAND attribute_test)
#        add_test(NAME bundle_archive_test COMMAND bundle_archive_test)
        add_test(NAME bundle_cache_test COMMAND bundle_cache_test)
        add_test(NAME bundle_context_test COMMAND bundle_context_test)
        add_test(NAME bundle_revision_test  COMMAND bundle_revision_test)
        add_test(NAME bundle_test COMMAND bundle_test)
        add_test(NAME capability_test COMMAND capability_test)
        add_test(NAME celix_errorcodes_test COMMAND celix_errorcodes_test)
        add_test(NAME filter_test COMMAND filter_test)
        add_test(NAME framework_test COMMAND framework_test)
        add_test(NAME manifest_parser_test COMMAND manifest_parser_test)
        add_test(NAME manifest_test COMMAND manifest_test)
#        add_test(NAME module_test COMMAND module_test)
        add_test(NAME requirement_test COMMAND requirement_test)
#        add_test(NAME resolver_test COMMAND resolver_test)
        add_test(NAME service_reference_test COMMAND service_reference_test)
        add_test(NAME service_registration_test COMMAND service_registration_test)
        add_test(NAME service_registry_test COMMAND service_registry_test)
        add_test(NAME service_tracker_customizer_test COMMAND service_tracker_customizer_test)
#        add_test(NAME service_tracker_test COMMAND service_tracker_test)
	add_test(NAME wire_test COMMAND wire_test)
	    
	SETUP_TARGET_FOR_COVERAGE(attribute_test attribute_test ${CMAKE_BINARY_DIR}/coverage/attribute_test/attribute_test)
#        SETUP_TARGET_FOR_COVERAGE(bundle_archive_test bundle_archive_test ${CMAKE_BINARY_DIR}/coverage/bundle_archive_test/bundle_archive_test)
        SETUP_TARGET_FOR_COVERAGE(bundle_cache_test bundle_cache_test ${CMAKE_BINARY_DIR}/coverage/bundle_cache_test/bundle_cache_test)
        SETUP_TARGET_FOR_COVERAGE(bundle_context_test bundle_context_test ${CMAKE_BINARY_DIR}/coverage/bundle_context_test/bundle_context_test)
        SETUP_TARGET_FOR_COVERAGE(bundle_revision_test bundle_revision_test ${CMAKE_BINARY_DIR}/coverage/bundle_revision_test/bundle_revision_test)
        SETUP_TARGET_FOR_COVERAGE(bundle_test bundle_test ${CMAKE_BINARY_DIR}/coverage/bundle_test/bundle_test)
        SETUP_TARGET_FOR_COVERAGE(capability_test capability_test ${CMAKE_BINARY_DIR}/coverage/capability_test/capability_test)
        SETUP_TARGET_FOR_COVERAGE(celix_errorcodes_test celix_errorcodes_test ${CMAKE_BINARY_DIR}/coverage/celix_errorcodes_test/celix_errorcodes_test)
        SETUP_TARGET_FOR_COVERAGE(filter_test filter_test ${CMAKE_BINARY_DIR}/coverage/filter_test/filter_test)
        SETUP_TARGET_FOR_COVERAGE(framework_test framework_test ${CMAKE_BINARY_DIR}/coverage/framework_test/framework_test)
        SETUP_TARGET_FOR_COVERAGE(manifest_parser_test manifest_parser_test ${CMAKE_BINARY_DIR}/coverage/manifest_parser_test/manifest_parser_test)
        SETUP_TARGET_FOR_COVERAGE(manifest_test manifest_test ${CMAKE_BINARY_DIR}/coverage/manifest_test/manifest_test)
#        SETUP_TARGET_FOR_COVERAGE(module_test module_test ${CMAKE_BINARY_DIR}/coverage/module_test/module_test)
        SETUP_TARGET_FOR_COVERAGE(requirement_test requirement_test ${CMAKE_BINARY_DIR}/coverage/requirement_test/requirement_test)
#        SETUP_TARGET_FOR_COVERAGE(resolver_test resolver_test ${CMAKE_BINARY_DIR}/coverage/resolver_test/resolver_test)
        SETUP_TARGET_FOR_COVERAGE(service_reference_test service_reference_test ${CMAKE_BINARY_DIR}/coverage/service_reference_test/service_reference_test)
        SETUP_TARGET_FOR_COVERAGE(service_registration_test service_registration_test ${CMAKE_BINARY_DIR}/coverage/service_registration_test/service_registration_test)
        SETUP_TARGET_FOR_COVERAGE(service_registry_test service_registry_test ${CMAKE_BINARY_DIR}/coverage/service_registry_test/service_registry_test)
        SETUP_TARGET_FOR_COVERAGE(service_tracker_customizer_test service_tracker_customizer_test ${CMAKE_BINARY_DIR}/coverage/service_tracker_customizer_test/service_tracker_customizer_test)
#        SETUP_TARGET_FOR_COVERAGE(service_tracker_test service_tracker_test ${CMAKE_BINARY_DIR}/coverage/service_tracker_test/service_tracker_test)
		SETUP_TARGET_FOR_COVERAGE(wire_test wire_test ${CMAKE_BINARY_DIR}/coverage/wire_test/wire_test)
		
	endif (ENABLE_TESTING AND FRAMEWORK_TESTS)
endif (FRAMEWORK)
