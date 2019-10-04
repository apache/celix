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
#  NanoMsg_FOUND - System has Nanomsg
#  NanoMsg_INCLUDE_DIRS - The Nanomsg include directories
#  NanoMsg_LIBRARIES - The libraries needed to use Nanomsg
#  NanoMsg::lib - Imported target for Nanomsg

find_path(NanoMsg_INCLUDE_DIR nanomsg/nn.h
          /usr/include
          /usr/local/include )

find_library(NanoMsg_LIBRARY NAMES nanomsg
             PATHS /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib64 )

set(NanoMsg_LIBRARIES ${NanoMsg_LIBRARY} )
set(NanoMsg_INCLUDE_DIRS ${NanoMsg_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(NanoMsg  DEFAULT_MSG
        NanoMsg_LIBRARY NanoMsg_INCLUDE_DIR)

mark_as_advanced(NanoMsg_INCLUDE_DIR NanoMsg_LIBRARY )

if (NanoMsg_FOUND AND NOT TARGET NanoMsg::lib)
    add_library(NanoMsg::lib SHARED IMPORTED)
    set_target_properties(NanoMsg::lib PROPERTIES
            IMPORTED_LOCATION "${NanoMsg_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${NanoMsg_INCLUDE_DIR}"
    )
endif ()