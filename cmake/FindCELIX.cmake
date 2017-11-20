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
#  CELIX_LIBRARIES - The libraries needed to use Apache Celix (framework,utils and dfi)
#  CELIX_LAUNCHER - The path to the celix launcher
#  CELIX_FRAMEWORK_LIBRARY - The path to the celix framework library
#  CELIX_UTILS_LIBRARY - The path to the celix utils library
#  CELIX_DFI_LIBRARY - The path to the celix dfi libary
#
#
#  CELIX_BUNDLES_DIR - The path where the Celix provided bundles are installed
#  CELIX_DM_LIB - The Celix Dependency Manager library
#  CELIX_DM_STATIC_LIB - The Celix Dependency Manager static library
#  CELIX_DM_STATIC_CXX_LIB - The Celix C++ Dependency Manager static library

set(CELIX_DIR_FROM_FINDCELIX "${CMAKE_CURRENT_LIST_DIR}/../../../..")


#Find libraries celix_framework, celix_utils, etcdlib and celix_dfi
#Find celix launcher
find_path(CELIX_INCLUDE_DIR NAMES celix_errno.h
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES include include/celix
)

find_path(CELIX_ETCD_INCLUDE_DIR NAMES etcd.h
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES include include/etcdlib
		)

find_library(CELIX_FRAMEWORK_LIBRARY NAMES celix_framework
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES lib lib64
)

find_library(CELIX_UTILS_LIBRARY NAMES celix_utils
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES lib lib64
)

find_library(CELIX_DFI_LIBRARY NAMES celix_dfi
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES lib lib64
)

find_program(CELIX_LAUNCHER NAMES celix
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES bin
)

find_file(CELIX_USECELIX_FILE NAMES UseCelix.cmake
             	PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
             	PATH_SUFFIXES share/celix/cmake/modules/cmake_celix
)




#Finding dependency manager libraries for C and C++
find_library(CELIX_DM_LIB NAMES dependency_manager_so
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES lib lib64
		)

find_library(CELIX_DM_STATIC_LIB NAMES dependency_manager_static
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES lib lib64
		)

find_library(CELIX_DM_STATIC_CXX_LIB NAMES dependency_manager_cxx_static
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES lib lib64
		)

find_library(CELIX_ETCD_LIB NAMES etcdlib
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES lib lib64
		)


#Finding bundles dir.
find_path(CELIX_BUNDLES_DIR shell.zip #NOTE assuming shell.zip is always installed.
		PATHS ${CELIX_DIR_FROM_FINDCELIX} $ENV{CELIX_DIR} ${CELIX_DIR} /usr /usr/local
		PATH_SUFFIXES share/celix/bundles
)

#Finding bundles. If not found the <BUNDLEVAR>_BUNDLE var will be set to <BUNDLEVAR>-NOTFOUND
find_file(CELIX_SHELL_BUNDLE shell.zip
		PATHS ${CELIX_BUNDLES_DIR}
		NO_DEFAULT_PATH
		)
find_file(CELIX_SHELL_TUI_BUNDLE shell_tui.zip
		PATHS ${CELIX_BUNDLES_DIR}
		NO_DEFAULT_PATH
		)


if (CELIX_DM_STATIC_LIB)
    set(CELIX_DM_INCLUDE_DIR ${CELIX_INCLUDE_DIR}/dependency_manager)
endif()
if (CELIX_DM_LIB)
    set(CELIX_DM_INCLUDE_DIR ${CELIX_INCLUDE_DIR}/dependency_manager)
endif ()
if (CELIX_DM_STATIC_CXX_LIB)
	set(CELIX_DM_CXX_STATIC_LIB ${CELIX_DM_STATIC_CXX_LIB}) #Ensure that var name from verion 2.0.0 is still valid
    set(CELIX_DM_CXX_INCLUDE_DIR ${CELIX_INCLUDE_DIR}/dependency_manager_cxx)
endif ()

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CELIX_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(CELIX  DEFAULT_MSG
	CELIX_FRAMEWORK_LIBRARY CELIX_UTILS_LIBRARY CELIX_DFI_LIBRARY CELIX_DM_LIB CELIX_DM_STATIC_LIB CELIX_DM_STATIC_CXX_LIB CELIX_INCLUDE_DIR CELIX_LAUNCHER CELIX_CMAKECELIX_FILE)
mark_as_advanced(CELIX_INCLUDE_DIR CELIX_ETCD_INCLUDE_DIR CELIX_USECELIX_FILE)

if(CELIX_FOUND)
	set(CELIX_LIBRARIES ${CELIX_FRAMEWORK_LIBRARY} ${CELIX_UTILS_LIBRARY} ${CELIX_DFI_LIBRARY})
	set(CELIX_INCLUDE_DIRS ${CELIX_INCLUDE_DIR} ${CELIX_ETCD_INCLUDE_DIR} ${CELIX_DM_INCLUDE_DIR} ${CELIX_DM_CXX_INCLUDE_DIR})

	include(${CELIX_USECELIX_FILE})
endif()

