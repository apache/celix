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

if (NOT WIN32)
	include(FindCurses)
endif (NOT WIN32)

FIND_PATH(CUNIT_INCLUDE_DIR CUnit/Basic.h
  /usr/local/include
  /usr/include
  /opt/local/include  
)

FIND_PATH(CUNIT_SHARE_DIR CUnit-List.dtd
	/usr/local/share/Cunit
  	/usr/share/CUnit
  	/opt/local/share/CUnit
)

# On unix system, debug and release have the same name
FIND_LIBRARY(CUNIT_LIBRARY cunit
             ${CUNIT_INCLUDE_DIR}/../../lib
             /usr/local/lib
             /usr/lib
             )
FIND_LIBRARY(CUNIT_DEBUG_LIBRARY cunit
             ${CUNIT_INCLUDE_DIR}/../../lib
             /usr/local/lib
             /usr/lib
             )

IF(CUNIT_INCLUDE_DIR)
  IF(CUNIT_LIBRARY)
    SET(CUNIT_FOUND "YES")
    if (WIN32)
    	SET(CUNIT_LIBRARIES ${CUNIT_LIBRARY})
	    SET(CUNIT_DEBUG_LIBRARIES ${CUNIT_DEBUG_LIBRARY})
    else (WIN32)
    	SET(CUNIT_LIBRARIES ${CUNIT_LIBRARY} ${CURSES_LIBRARY})
	    SET(CUNIT_DEBUG_LIBRARIES ${CUNIT_DEBUG_LIBRARY} ${CURSES_DEBUG_LIBRARY})
    endif (WIN32)
  ENDIF(CUNIT_LIBRARY)
  IF(CUNIT_INCLUDE_DIR)
  	if (WIN32)
    	SET(CUNIT_INCLUDE_DIRS ${CUNIT_INCLUDE_DIR})
	else (WIN32)
		MESSAGE(STATUS "Found CUNIT: ${CUNIT_INCLUDE_DIR}")
		SET(CUNIT_INCLUDE_DIRS ${CUNIT_INCLUDE_DIR} ${CURSES_INCLUDE_DIR})
	endif (WIN32)
  ENDIF(CUNIT_INCLUDE_DIR)
ENDIF(CUNIT_INCLUDE_DIR)
