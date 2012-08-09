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

cmake_minimum_required (VERSION 2.6)
project (HessianC C)

set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/../cmake/modules/")

SET(CMAKE_BUILD_TYPE "Debug")
SET(CMAKE_C_FLAGS "-D_GNU_SOURCE -std=gnu99 ${CMAKE_C_FLAGS}")

MACRO(PARSE_ARGUMENTS prefix arg_names option_names)
  SET(DEFAULT_ARGS)
  FOREACH(arg_name ${arg_names})    
    SET(${prefix}_${arg_name})
  ENDFOREACH(arg_name)
  FOREACH(option ${option_names})
    SET(${prefix}_${option} FALSE)
  ENDFOREACH(option)

  SET(current_arg_name DEFAULT_ARGS)
  SET(current_arg_list)
  FOREACH(arg ${ARGN})            
    SET(larg_names ${arg_names})    
    LIST(FIND larg_names "${arg}" is_arg_name)                   
    IF (is_arg_name GREATER -1)
      SET(${prefix}_${current_arg_name} ${current_arg_list})
      SET(current_arg_name ${arg})
      SET(current_arg_list)
    ELSE (is_arg_name GREATER -1)
      SET(loption_names ${option_names})    
      LIST(FIND loption_names "${arg}" is_option)            
      IF (is_option GREATER -1)
	     SET(${prefix}_${arg} TRUE)
      ELSE (is_option GREATER -1)
	     SET(current_arg_list ${current_arg_list} ${arg})
      ENDIF (is_option GREATER -1)
    ENDIF (is_arg_name GREATER -1)
  ENDFOREACH(arg)
  SET(${prefix}_${current_arg_name} ${current_arg_list})
ENDMACRO(PARSE_ARGUMENTS)

ADD_CUSTOM_TARGET(test)
MACRO(run_test)
    PARSE_ARGUMENTS(TEST "" "" ${ARGN})
    LIST(GET TEST_DEFAULT_ARGS 0 EXEC)
	
	SET(__testTarget test_${EXEC})
	
	file(MAKE_DIRECTORY ${PROJECT_BINARY_DIR}/test_results)
		
	add_custom_target(${__testTarget}
		${EXEC} ${EXEC} 
		COMMAND xsltproc --path ${CUNIT_SHARE_DIR} ${CUNIT_SHARE_DIR}/CUnit-Run.xsl ${PROJECT_BINARY_DIR}/test_results/${EXEC}-Results.xml > ${EXEC}-Results.html
		COMMAND xsltproc --path ${CUNIT_SHARE_DIR} ${CUNIT_SHARE_DIR}/CUnit-List.xsl ${PROJECT_BINARY_DIR}/test_results/${EXEC}-Listing.xml > ${EXEC}-Listing.html
		
		WORKING_DIRECTORY ${PROJECT_BINARY_DIR}/test_results
	)
	ADD_DEPENDENCIES(test ${__testTarget})
ENDMACRO(run_test)

include_directories("private/include")
include_directories("public/include")

add_library(hessian STATIC private/src/hessian_out private/src/hessian_in)

include(FindCUnit)

include_directories(${CUNIT_INCLUDE_DIRS})
add_executable(test_hessian_out private/test/test_hessian_out.c)
target_link_libraries(test_hessian_out hessian ${CUNIT_LIBRARIES})

run_test(test_hessian_out)
