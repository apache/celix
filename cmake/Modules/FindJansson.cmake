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


# - Try to find Jansson
# Once done this will define
#  Jansson - System has Jansson
#  Jansson_INCLUDE_DIRS - The Jansson include directories
#  Jansson_LIBRARIES - The libraries needed to use Jansson
#  Jansson_DEFINITIONS - Compiler switches required for using Jansson
#  Jansson::lib - Imported target for Jansson

find_path(Jansson_INCLUDE_DIR jansson.h
          /usr/include
          /usr/local/include )

find_library(Jansson_LIBRARY NAMES jansson
             PATHS /usr/lib /usr/local/lib )

set(Jansson_LIBRARIES ${Jansson_LIBRARY} )
set(Jansson_INCLUDE_DIRS ${Jansson_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set JANSSON_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(Jansson  DEFAULT_MSG
                                  Jansson_LIBRARY Jansson_INCLUDE_DIR)

mark_as_advanced(Jansson_INCLUDE_DIR Jansson_LIBRARY)

if (Jansson_FOUND AND NOT TARGET Jansson::lib)
    add_library(Jansson::lib SHARED IMPORTED)
    set_target_properties(Jansson::lib PROPERTIES
            IMPORTED_LOCATION "${Jansson_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${Jansson_INCLUDE_DIR}"
    )

endif ()

#For backward compatability
set(JANSSON_LIBRARY ${Jansson_LIBRARY})
set(JANSSON_LIBRARIES ${Jansson_LIBRARY})
set(JANSSON_INCLUDE_DIR ${Jansson_INCLUDE_DIR})
set(JANSSON_INCLUDE_DIRS ${Jansson_INCLUDE_DIR})
