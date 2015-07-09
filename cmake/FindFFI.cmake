
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


# - Try to find libffi define the variables for the binaries/headers and include 
#
# Once done this will define
#  FFI_FOUND - System has libffi
#  FFI_INCLUDE_DIRS - The package include directories
#  FFI_LIBRARIES - The libraries needed to use this package

find_library(FFI_LIBRARY NAMES ffi
             	PATHS $ENV{FFI_DIR} ${FFI_DIR} /usr/local /opt/local ENV FFI_DIR
             	PATH_SUFFIXES lib lib/x86_64-linux-gnu NO_DEFAULT_PATH)

find_library(FFI_LIBRARY NAMES ffi
             	PATH_SUFFIXES lib)

#NOTE on OSX ffi.h from macport is located at /opt/local/lib/libffi-<version>/ffi.h 
#Using FFI_LIBRARY location as hint to find it
get_filename_component(FFI_LIB_DIR ${FFI_LIBRARY} DIRECTORY)
find_path(FFI_INCLUDE_DIR ffi.h
		HINTS ${FFI_LIB_DIR}/*
        PATH_SUFFIXES include include/ffi) 
unset(FFI_LIB_DIR)

find_path(FFI_INCLUDE_DIR ffi.h
		PATHS $ENV{FFI_DIR} ${FFI_DIR} /usr /usr/local /opt/local 
        PATH_SUFFIXES include include/ffi include/x86_64-linux-gnu) 


include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set FFI_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(FFI  DEFAULT_MSG
                                  FFI_LIBRARY FFI_INCLUDE_DIR)
mark_as_advanced(FFI_INCLUDE_DIR FFI_LIBRARY) 

if(FFI_FOUND)
	set(FFI_LIBRARIES ${FFI_LIBRARY})
	set(FFI_INCLUDE_DIRS ${FFI_INCLUDE_DIR})
endif()
