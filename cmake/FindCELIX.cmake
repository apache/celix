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


# - Try to find Celix define the variables for the binaries/headers and include 
#   the Celix cmake modules needed to build bundles
#
# Once done this will define
#  CELIX_FOUND - System has Apache Celix
#  CELIX_INCLUDE_DIRS - The Apache Celix include directories
#  CELIX_LIBRARIES - The libraries needed to use Apache Celix
#  CELIX_LAUNCHER - The path to the celix launcher
#  CELIX_BUNDLES_DIR - The path where the Celix provided bundles are installed

set(CELIX_DIR_FROM_FINDCELIX "${CMAKE_CURRENT_LIST_DIR}/../../../..")


find_path(CELIX_INCLUDE_DIR celix_errno.h
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
          	PATH_SUFFIXES include include/celix)

find_library(CELIX_FRAMEWORK_LIBRARY NAMES celix_framework
             	PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
             	PATH_SUFFIXES lib lib64)
             
find_library(CELIX_UTILS_LIBRARY NAMES celix_utils
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES lib lib64)

find_program(CELIX_LAUNCHER NAMES celix
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES bin)

find_file(CELIX_CMAKECELIX_FILE CMakeCelix.cmake
             	PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
             	PATH_SUFFIXES share/celix/cmake/modules)

#NOTE assuming shell.zip is always installed.
find_path(CELIX_BUNDLES_DIR shell.zip
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
          	PATH_SUFFIXES share/celix/bundles)
	


include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CELIX_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CELIX  DEFAULT_MSG
                                  CELIX_FRAMEWORK_LIBRARY CELIX_UTILS_LIBRARY CELIX_INCLUDE_DIR CELIX_LAUNCHER CELIX_CMAKECELIX_FILE) 
mark_as_advanced(CELIX_INCLUDE_DIR CELIX_FRAMEWORK_LIBRARY CELIX_UTILS_LIBRARY CELIX_LAUNCHER CELIX_CMAKECELIX_FILE)

if(CELIX_FOUND)
	set(CELIX_LIBRARIES ${CELIX_FRAMEWORK_LIBRARY} ${CELIX_UTILS_LIBRARY})
	set(CELIX_INCLUDE_DIRS ${CELIX_INCLUDE_DIR})
	include(${CELIX_CMAKECELIX_FILE})
endif()
