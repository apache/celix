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

celix_subproject(UTILS "Option to build the utilities library" "ON")
if (UTILS) 
    cmake_minimum_required(VERSION 2.6)
    
    add_definitions(-DUSE_FILE32API)
    #aux_source_directory("private/src" SRC)
    include_directories("private/include")
    include_directories("public/include")
    add_library(celix_utils SHARED 
		private/src/array_list.c
		public/include/array_list.h 
		private/include/array_list_private.h 

		private/src/hash_map.c
		public/include/hash_map.h 
		private/include/hash_map_private.h

		private/src/linkedlist.c
		private/src/linked_list_iterator.c
		public/include/linkedlist.h
		public/include/linked_list_iterator.h 
		private/include/linked_list_private.h

		private/src/netstring.c
		public/include/netstring.h
		public/include/exports.h
	)
    
    IF(UNIX)
    	target_link_libraries(celix_utils m)
    ENDIF(UNIX)
    target_link_libraries(celix_utils ${APR_LIBRARY})
    
    install(TARGETS celix_utils DESTINATION lib COMPONENT framework)
    FILE(GLOB files "public/include/*.h")
    INSTALL(FILES ${files} DESTINATION include/celix COMPONENT framework)
    
    celix_subproject(UTILS-TESTS "Option to build the utilities library tests" "OFF")
    if (UTILS-TESTS)
	    include_directories(${CUNIT_INCLUDE_DIRS})
	    include_directories(${CPPUTEST_INCLUDE_DIR})
	    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
	    include_directories("${PROJECT_SOURCE_DIR}/utils/private/include")
	    
	    add_executable(hash_map_test private/test/hash_map_test.c)
	    target_link_libraries(hash_map_test celix_utils ${CUNIT_LIBRARIES})
	    
	    add_executable(hash_map_test_hash private/test/hash_map_test_hash.c)
	    target_link_libraries(hash_map_test_hash celix_utils ${CUNIT_LIBRARIES})
	    
	    add_executable(array_list_test2 private/test/array_list_test.c)
	    target_link_libraries(array_list_test2 celix_utils ${CUNIT_LIBRARIES})
	    
	    add_executable(array_list_test private/test/array_list_test.cpp)
		target_link_libraries(array_list_test celix_utils ${CPPUTEST_LIBRARY})
	    
	    add_executable(linked_list_test private/test/linked_list_test.c)
	    target_link_libraries(linked_list_test celix_utils ${CUNIT_LIBRARIES})

	    add_executable(netstring_test private/test/netstring_test.c)
	    target_link_libraries(netstring_test celix_utils celix_framework ${CUNIT_LIBRARIES})
		    
	    
	    run_test(array_list_test2)
	    run_test(hash_map_test)
	    run_test(hash_map_test_hash)
	    run_test(linked_list_test)
	    run_test(netstring_test)
	    
	    ADD_TARGET_FOR_TEST(array_list_test)
    endif(UTILS-TESTS)
endif (UTILS)
