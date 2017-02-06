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
#  ZMQ_FOUND - System has Zmq
#  ZMQ_INCLUDE_DIRS - The Zmq include directories
#  ZMQ_LIBRARIES - The libraries needed to use Zmq
#  ZMQ_DEFINITIONS - Compiler switches required for using Zmq

find_path(ZMQ_INCLUDE_DIR zmq.h zmq_utils.h
          /usr/include
          /usr/local/include )

find_library(ZMQ_LIBRARY NAMES zmq
             PATHS /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib64 )

set(ZMQ_LIBRARIES ${ZMQ_LIBRARY} )
set(ZMQ_INCLUDE_DIRS ${ZMQ_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set ZMQ_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Zmq  DEFAULT_MSG
                                  ZMQ_LIBRARY ZMQ_INCLUDE_DIR)

mark_as_advanced(ZMQ_INCLUDE_DIR ZMQ_LIBRARY )
