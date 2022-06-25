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
#  jansson_FOUND - System has Jansson
#  JANSSON_INCLUDE_DIRS - The Jansson include directories
#  JANSSON_LIBRARIES - The libraries needed to use Jansson
#  jansson::jansson - Imported target for Jansson

find_path(JANSSON_INCLUDE_DIR jansson.h
          /usr/include
          /usr/local/include )

find_library(JANSSON_LIBRARY NAMES jansson
             PATHS /usr/lib /usr/local/lib )

set(JANSSON_LIBRARIES ${JANSSON_LIBRARY} )
set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set jansson_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(jansson  DEFAULT_MSG
                                  JANSSON_LIBRARY JANSSON_INCLUDE_DIR)

mark_as_advanced(JANSSON_INCLUDE_DIR JANSSON_LIBRARY)

if (jansson_FOUND AND NOT TARGET jansson::jansson)
    add_library(jansson::jansson SHARED IMPORTED)
    set_target_properties(jansson::jansson PROPERTIES
            IMPORTED_LOCATION "${JANSSON_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${JANSSON_INCLUDE_DIR}"
    )

endif ()

#For backward compatability
set(JANSSON_LIBRARY ${JANSSON_LIBRARY})
set(JANSSON_LIBRARIES ${JANSSON_LIBRARY})
set(JANSSON_INCLUDE_DIR ${JANSSON_INCLUDE_DIR})
set(JANSSON_INCLUDE_DIRS ${JANSSON_INCLUDE_DIR})
