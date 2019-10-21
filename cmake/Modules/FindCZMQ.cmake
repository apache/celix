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


# - Try to find CZMQ
# 	Once done this will define
#  CZMQ_FOUND - System has Zmq
#  CZMQ_INCLUDE_DIRS - The Zmq include directories
#  CZMQ_LIBRARIES - The libraries needed to use Zmq
#  CZMQ_DEFINITIONS - Compiler switches required for using Zmq
#  CZMQ::lib - Imported CMake target for the library (include path + library)

find_path(CZMQ_INCLUDE_DIR czmq.h
          /usr/include
          /usr/local/include )

find_library(CZMQ_LIBRARY NAMES czmq
             PATHS /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib64 )

set(CZMQ_LIBRARIES ${CZMQ_LIBRARY} )
set(CZMQ_INCLUDE_DIRS ${CZMQ_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set CZMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Czmq  DEFAULT_MSG
                                  CZMQ_LIBRARY CZMQ_INCLUDE_DIR)

mark_as_advanced(CZMQ_INCLUDE_DIR CZMQ_LIBRARY)

if (CZMQ_FOUND AND NOT TARGET CZMQ::lib)
    add_library(CZMQ::lib SHARED IMPORTED)
    set_target_properties(CZMQ::lib PROPERTIES
            IMPORTED_LOCATION "${CZMQ_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${CZMQ_INCLUDE_DIR}"
    )
endif ()
