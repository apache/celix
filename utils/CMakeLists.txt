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

celix_subproject(UTILS "Option to build the utilities library" ON)
if (UTILS) 
    cmake_minimum_required(VERSION 2.6)
    
if (ANDROID)
    add_definitions(-DUSE_FILE32API)
endif ()

    include_directories("private/include")
    include_directories("public/include")
    add_library(celix_utils SHARED 
                private/src/array_list.c
                private/src/hash_map.c
                private/src/linked_list.c
                private/src/linked_list_iterator.c
                private/src/celix_threads.c
                private/src/version.c
                private/src/version_range.c
                private/src/thpool.c
                private/src/properties.c
                private/src/utils.c
    )

    set_target_properties(celix_utils PROPERTIES "SOVERSION" 2)
    
    IF(UNIX AND NOT ANDROID)
        target_link_libraries(celix_utils m pthread)
    ELSEIF(ANDROID)
        target_link_libraries(celix_utils m)
    ENDIF()
    
    install(TARGETS celix_utils DESTINATION ${CMAKE_INSTALL_LIBDIR} COMPONENT framework)
    FILE(GLOB files "public/include/*.h")
    INSTALL(FILES ${files} DESTINATION include/celix COMPONENT framework)
    
    celix_subproject(UTILS-TESTS "Option to build the utilities library tests" "OFF")

        if (ENABLE_TESTING AND UTILS-TESTS)
            find_package(CppUTest REQUIRED)

            SET(CMAKE_SKIP_BUILD_RPATH  FALSE) 
            SET(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE) 
            SET(CMAKE_INSTALL_RPATH "${PROJECT_BINARY_DIR}/utils")


            include_directories(${CUNIT_INCLUDE_DIRS})
            include_directories(${CPPUTEST_INCLUDE_DIR})
            include_directories(public/include)
            include_directories(utils/private/include)
            
            add_executable(hash_map_test private/test/hash_map_test.cpp)
            target_link_libraries(hash_map_test celix_utils ${CPPUTEST_LIBRARY} pthread)
            
            add_executable(array_list_test private/test/array_list_test.cpp)
            target_link_libraries(array_list_test celix_utils ${CPPUTEST_LIBRARY} pthread)
            
            add_executable(celix_threads_test private/test/celix_threads_test.cpp)
            target_link_libraries(celix_threads_test celix_utils ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} pthread)
            add_executable(linked_list_test private/test/linked_list_test.cpp)
            target_link_libraries(linked_list_test celix_utils ${CPPUTEST_LIBRARY} pthread)
            
            add_executable(thread_pool_test private/test/thread_pool_test.cpp)
            target_link_libraries(thread_pool_test celix_utils ${CPPUTEST_LIBRARY} pthread) 

            add_executable(properties_test private/test/properties_test.cpp)
            target_link_libraries(properties_test ${CPPUTEST_LIBRARY} ${CPPUTEST_EXT_LIBRARY} celix_utils pthread)

            add_executable(utils_test private/test/utils_test.cpp)
            target_link_libraries(utils_test ${CPPUTEST_LIBRARY} celix_utils pthread)
		
            configure_file(private/resources-test/properties.txt ${CMAKE_BINARY_DIR}/utils/resources-test/properties.txt COPYONLY)

            add_test(NAME run_array_list_test COMMAND array_list_test)
            add_test(NAME run_hash_map_test COMMAND hash_map_test)
            add_test(NAME run_celix_threads_test COMMAND celix_threads_test)
            add_test(NAME run_thread_pool_test COMMAND thread_pool_test)
            add_test(NAME run_linked_list_test COMMAND linked_list_test)
            add_test(NAME run_properties_test COMMAND properties_test)
            add_test(NAME run_utils_test COMMAND utils_test)
        
            SETUP_TARGET_FOR_COVERAGE(array_list_test array_list_test ${CMAKE_BINARY_DIR}/coverage/array_list_test/array_list_test)
            SETUP_TARGET_FOR_COVERAGE(hash_map hash_map_test ${CMAKE_BINARY_DIR}/coverage/hash_map_test/hash_map_test)
            SETUP_TARGET_FOR_COVERAGE(celix_threads_test celix_threads_test ${CMAKE_BINARY_DIR}/coverage/celix_threads_test/celix_threads_test)
            SETUP_TARGET_FOR_COVERAGE(thread_pool_test thread_pool_test ${CMAKE_BINARY_DIR}/coverage/thread_pool_test/thread_pool_test)
            SETUP_TARGET_FOR_COVERAGE(linked_list_test linked_list_test ${CMAKE_BINARY_DIR}/coverage/linked_list_test/linked_list_test)
            SETUP_TARGET_FOR_COVERAGE(properties_test properties_test ${CMAKE_BINARY_DIR}/coverage/properties_test/properties_test)
            SETUP_TARGET_FOR_COVERAGE(utils_test utils_test ${CMAKE_BINARY_DIR}/coverage/utils_test/utils_test)

   endif(ENABLE_TESTING AND UTILS-TESTS)
endif (UTILS)
