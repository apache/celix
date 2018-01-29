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

FIND_PATH(CPPUTEST_INCLUDE_DIR NAMES CppUTest/TestHarness.h
    PATHS $ENV{CPPUTEST_DIR} ${CPPUTEST_DIR} /usr /usr/local /opt/local
    PATH_SUFFIXES include    
)

FIND_PATH(CPPUTEST_EXT_INCLUDE_DIR NAMES CppUTestExt/MockSupport.h
    PATHS $ENV{CPPUTEST_DIR} ${CPPUTEST_DIR} /usr /usr/local /opt/local
    PATH_SUFFIXES include    
)

FIND_LIBRARY(CPPUTEST_LIBRARY NAMES CppUTest
    PATHS $ENV{CPPUTEST_DIR} ${CPPUTEST_DIR} /usr /usr/local /opt/local
    PATH_SUFFIXES lib lib64 
)

FIND_LIBRARY(CPPUTEST_EXT_LIBRARY NAMES CppUTestExt
    PATHS $ENV{CPPUTEST_DIR} ${CPPUTEST_DIR} /usr /usr/local /opt/local
    PATH_SUFFIXES lib lib64 
)

# handle the QUIETLY and REQUIRED arguments and set CURL_FOUND to TRUE if 
# all listed variables are TRUE
INCLUDE(FindPackageHandleStandardArgs)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CPPUTEST DEFAULT_MSG CPPUTEST_LIBRARY CPPUTEST_INCLUDE_DIR)
FIND_PACKAGE_HANDLE_STANDARD_ARGS(CPPUTEST_EXT DEFAULT_MSG CPPUTEST_EXT_LIBRARY CPPUTEST_EXT_INCLUDE_DIR)

mark_as_advanced(CPPUTEST_INCLUDE_DIR CPPUTEST_LIBRARY CPPUTEST_EXT_LIBRARY CPPUTEST_INCLUDE_DIR) 

IF(CPPUTEST_FOUND)
    SET(CPPUTEST_LIBRARIES ${CPPUTEST_LIBRARY})
    SET(CPPUTEST_INCLUDE_DIRS ${CPPUTEST_INCLUDE_DIR})
ENDIF(CPPUTEST_FOUND)

IF(CPPUTEST_EXT_FOUND)
    SET(CPPUTEST_EXT_LIBRARIES ${CPPUTEST_EXT_LIBRARY})
    SET(CPPUTEST_EXT_INCLUDE_DIRS ${CPPUTEST_EXT_INCLUDE_DIR})
ENDIF(CPPUTEST_EXT_FOUND)
