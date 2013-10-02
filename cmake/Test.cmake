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

include(FindCUnit)

ADD_CUSTOM_TARGET(test_cunit)
MACRO(run_test)
    PARSE_ARGUMENTS(TEST "" "" ${ARGN})
    LIST(GET TEST_DEFAULT_ARGS 0 EXEC)
	
	SET(__testTarget test_${EXEC})
	
	make_directory(${PROJECT_BINARY_DIR}/test_results)
		
	add_custom_target(${__testTarget}
		${EXEC} ${EXEC} 
		COMMAND if [ -e ${PROJECT_BINARY_DIR}/test_results/${EXEC}-Results.xml ]\; then xsltproc --path ${CUNIT_SHARE_DIR} ${CUNIT_SHARE_DIR}/CUnit-Run.xsl ${PROJECT_BINARY_DIR}/test_results/${EXEC}-Results.xml > ${EXEC}-Results.html \; fi
		COMMAND if [ -e ${PROJECT_BINARY_DIR}/test_results/${EXEC}-Listing.xml ]\; then xsltproc --path ${CUNIT_SHARE_DIR} ${CUNIT_SHARE_DIR}/CUnit-List.xsl ${PROJECT_BINARY_DIR}/test_results/${EXEC}-Listing.xml > ${EXEC}-Listing.html \; fi
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test_results
	)
	ADD_DEPENDENCIES(test_cunit ${__testTarget})
ENDMACRO(run_test)

ADD_CUSTOM_TARGET(test_cppu)
MACRO(run_cppu_test)
    PARSE_ARGUMENTS(TEST "" "" ${ARGN})
    LIST(GET TEST_DEFAULT_ARGS 0 EXEC)
	
	SET(__testTarget test_${EXEC})
	
	make_directory(${PROJECT_BINARY_DIR}/test_results)
		
	add_custom_target(${__testTarget}
		${EXEC} "-ojunit" 
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test_results
	)
	ADD_DEPENDENCIES(test_cppu ${__testTarget})
ENDMACRO(run_cppu_test)

Include(FindRuby)
ADD_CUSTOM_TARGET(testUnity)
MACRO(run_unity_test)
	PARSE_ARGUMENTS(TEST "SOURCE;MOCKS" "" ${ARGN})
	LIST(GET TEST_DEFAULT_ARGS 0 EXEC)
	
	SET(__testTarget test_${EXEC})
	SET(MOCKS "")
	FOREACH(MOCK ${TEST_MOCKS})
		get_filename_component(mockName ${MOCK} NAME_WE)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Mock${mockName}.c ${CMAKE_CURRENT_BINARY_DIR}/Mock${mockName}.h
      		COMMAND ${RUBY_EXECUTABLE} ${PROJECT_SOURCE_DIR}/cmake/cmock/lib/cmock.rb -o${PROJECT_SOURCE_DIR}/cmake/cmock/test.yml ${CMAKE_CURRENT_SOURCE_DIR}/${MOCK}
  			DEPENDS ${MOCK}
  			COMMENT "Generating mock for ${MOCK}"
      	)
      	SET(MOCKS "${MOCKS}${CMAKE_CURRENT_BINARY_DIR}/Mock${mockName}.c")
	ENDFOREACH(MOCK)

	add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/${EXEC}_runner.c
		COMMAND ${RUBY_EXECUTABLE} ${PROJECT_SOURCE_DIR}/cmake/unity/generate_test_runner.rb 
				${CMAKE_CURRENT_SOURCE_DIR}/${TEST_SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/${EXEC}_runner.c
		DEPENDS ${TEST_SOURCE}
		COMMENT "Generating runner: ${CMAKE_CURRENT_BINARY_DIR}/${EXEC}_runner.c"
	)
	include_directories(${PROJECT_SOURCE_DIR}/cmake/cmock/src)
	include_directories(${PROJECT_SOURCE_DIR}/cmake/unity/src)
	include_directories(${CMAKE_CURRENT_BINARY_DIR})
	add_executable(${EXEC} ${TEST_SOURCE} ${CMAKE_CURRENT_BINARY_DIR}/${EXEC}_runner.c 
	    ${PROJECT_SOURCE_DIR}/cmake/unity/src/unity.c
	    ${PROJECT_SOURCE_DIR}/cmake/cmock/src/cmock.c 
		${MOCKS})
	
	message(${CMAKE_CURRENT_BINARY_DIR}/${EXEC})
	#add_test(${EXEC} ${CMAKE_CURRENT_BINARY_DIR}/${EXEC})
	add_custom_command(OUTPUT ${PROJECT_BINARY_DIR}/test_results/${EXEC}
		COMMAND ${EXEC}
		COMMAND touch ${PROJECT_BINARY_DIR}/test_results/${EXEC}
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test_results
		COMMENT "Run tests"
	)
	
	ADD_CUSTOM_TARGET(${__testTarget}
    	DEPENDS ${PROJECT_BINARY_DIR}/test_results/${EXEC}
    	COMMENT "Some comment")
	ADD_DEPENDENCIES(testUnity ${__testTarget})
ENDMACRO(run_unity_test)

MACRO(gm)
	PARSE_ARGUMENTS(MOCK "" "" ${ARGN})
	LIST(GET MOCK_DEFAULT_ARGS 0 MOCKS)
	
	FOREACH(MOCK ${MOCKS})
		get_filename_component(mockName ${MOCK} NAME_WE)
		add_custom_command(OUTPUT ${CMAKE_CURRENT_BINARY_DIR}/Mock${mockName}.c ${CMAKE_CURRENT_BINARY_DIR}/Mock${mockName}.h
      		#COMMAND ${RUBY_EXECUTABLE} ${PROJECT_SOURCE_DIR}/cmake/cmock/lib/cmock.rb -o${PROJECT_SOURCE_DIR}/cmake/cmock/test.yml ${CMAKE_CURRENT_SOURCE_DIR}/${MOCK}
  			#DEPENDS ${MOCK}
  			COMMENT "Generating mock for ${MOCK}"
      	)
	ENDFOREACH(MOCK)
	
ENDMACRO(gm)