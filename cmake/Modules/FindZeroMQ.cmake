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


# - Try to find ZMQ
# 	Once done this will define
#  ZeroMQ_FOUND - System has Zmq
#  ZEROMQ_INCLUDE_DIRS - The Zmq include directories
#  ZEROMQ_LIBRARIES - The libraries needed to use Zmq
#  ZEROMQ_DEFINITIONS - Compiler switches required for using Zmq
#  ZeroMQ::ZeroMQ - Imported target for UUID

find_path(ZEROMQ_INCLUDE_DIR zmq.h zmq_utils.h
          /usr/include
          /usr/local/include )

find_library(ZEROMQ_LIBRARY NAMES zmq
             PATHS /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib64 )

set(ZEROMQ_LIBRARIES ${ZEROMQ_LIBRARY} )
set(ZEROMQ_INCLUDE_DIRS ${ZEROMQ_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZeroMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(ZeroMQ  DEFAULT_MSG
                                  ZEROMQ_LIBRARY ZEROMQ_INCLUDE_DIR)

mark_as_advanced(ZEROMQ_INCLUDE_DIR ZEROMQ_LIBRARY )

if (ZeroMQ_FOUND AND NOT TARGET ZeroMQ::ZeroMQ)
    add_library(ZeroMQ::ZeroMQ SHARED IMPORTED)
    set_target_properties(ZeroMQ::ZeroMQ PROPERTIES
            IMPORTED_LOCATION "${ZEROMQ_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${ZEROMQ_INCLUDE_DIR}"
    )
endif ()
