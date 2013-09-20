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


# - Try to find Slp
# Once done this will define
#  SLP_FOUND - System has Slp
#  SLP_INCLUDE_DIRS - The Slp include directories
#  SLP_LIBRARIES - The libraries needed to use Slp
#  SLP_DEFINITIONS - Compiler switches required for using Slp

find_path(SLP_INCLUDE_DIR slp.h
          /usr/include
          /usr/local/include )

find_library(SLP_LIBRARY NAMES slp
             PATHS /usr/lib /usr/local/lib )

set(SLP_LIBRARIES ${SLP_LIBRARY} )
set(SLP_INCLUDE_DIRS ${SLP_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set SLP_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Slp  DEFAULT_MSG
                                  SLP_LIBRARY SLP_INCLUDE_DIR)

mark_as_advanced(SLP_INCLUDE_DIR SLP_LIBRARY )