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

FIND_PATH(CppUTest_INCLUDE_DIR NAMES CppUTest/TestHarness.h
    PATHS $ENV{CPPUTEST_DIR} ${CPPUTEST_DIR} /usr /usr/local /opt/local
    PATH_SUFFIXES include    
)

FIND_PATH(CppUTest_EXT_INCLUDE_DIR NAMES CppUTestExt/MockSupport.h
    PATHS $ENV{CPPUTEST_DIR} ${CPPUTEST_DIR} /usr /usr/local /opt/local
    PATH_SUFFIXES include    
)

FIND_LIBRARY(CppUTest_LIBRARY NAMES CppUTest
    PATHS $ENV{CPPUTEST_DIR} ${CPPUTEST_DIR} /usr /usr/local /opt/local
    PATH_SUFFIXES lib lib64 
)

FIND_LIBRARY(CppUTest_EXT_LIBRARY NAMES CppUTestExt
    PATHS $ENV{CPPUTEST_DIR} ${CPPUTEST_DIR} /usr /usr/local /opt/local
    PATH_SUFFIXES lib lib64 
)

# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CppUTest DEFAULT_MSG CppUTest_LIBRARY CppUTest_INCLUDE_DIR)


IF(CppUTest_FOUND)
    SET(CppUTest_LIBRARIES ${CppUTest_LIBRARY})
    SET(CppUTest_INCLUDE_DIRS ${CppUTest_INCLUDE_DIR})
    if(NOT TARGET CppUTest::CppUTest)
        add_library(CppUTest::CppUTest STATIC IMPORTED)
        set_target_properties(CppUTest::CppUTest PROPERTIES
                IMPORTED_LOCATION "${CppUTest_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${CppUTest_INCLUDE_DIR}"
                )
    endif()
    SET(CppUTest_EXT_LIBRARIES ${CppUTest_EXT_LIBRARY})
    SET(CppUTest_EXT_INCLUDE_DIRS ${CppUTest_EXT_INCLUDE_DIR})
    if(NOT TARGET CppUTest::CppUTestExt)
        add_library(CppUTest::CppUTestExt STATIC IMPORTED)
        set_target_properties(CppUTest::CppUTestExt PROPERTIES
                IMPORTED_LOCATION "${CppUTest_EXT_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${CppUTest_EXT_INCLUDE_DIR}"
                )
    endif()
ENDIF(CppUTest_FOUND)
