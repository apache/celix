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


# - Try to find NanoMsg
# 	Once done this will define
#  NANOMSG_FOUND - System has Nanomsg
#  NANOMSG_INCLUDE_DIRS - The Nanomsg include directories
#  NANOMSG_LIBRARIES - The libraries needed to use Nanomsg
#  NanoMsg::lib - Imported target for Nanomsg

find_path(NANOMSG_INCLUDE_DIR nanomsg/nn.h
          /usr/include
          /usr/local/include )

find_library(NANOMSG_LIBRARY NAMES nanomsg
             PATHS /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib64 )

set(NANOMSG_LIBRARIES ${NANOMSG_LIBRARY} )
set(NANOMSG_INCLUDE_DIRS ${NANOMSG_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(NanoMsg  DEFAULT_MSG
        NANOMSG_LIBRARY NANOMSG_INCLUDE_DIR)

mark_as_advanced(NANOMSG_INCLUDE_DIR NANOMSG_LIBRARY )

if (NANOMSG_FOUND AND NOT TARGET NanoMsg::lib)
    add_library(NanoMsg::lib SHARED IMPORTED)
    set_target_properties(NanoMsg::lib PROPERTIES
            IMPORTED_LOCATION "${NANOMSG_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${NANOMSG_INCLUDE_DIR}"
    )
endif ()