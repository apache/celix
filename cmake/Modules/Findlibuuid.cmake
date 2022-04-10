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

# - Try to find UUID
# Once done this will define
#  libuuid_FOUND - System has UUID
#  UUID_INCLUDE_DIRS - The UUID include directories
#  UUID_LIBRARIES - The libraries needed to use UUID
#  libuuid::libuuid - Imported target for UUID

if (APPLE)
    set(UUID_INCLUDE_DIRS )
    set(UUID_LIBRARIES )

    find_package_handle_standard_args(libuuid DEFAULT_MSG)

    if (NOT TARGET libuuid::libuuid)
        add_library(libuuid::libuuid INTERFACE IMPORTED)
    endif ()
else ()

    find_path(UUID_INCLUDE_DIR uuid/uuid.h
              /usr/include
              /usr/local/include )

    find_library(UUID_LIBRARY NAMES uuid
                 PATHS /usr/lib /usr/local/lib /usr/lib64 /usr/local/lib64 /lib/i386-linux-gnu /lib/x86_64-linux-gnu /usr/lib/x86_64-linux-gnu)

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(libuuid DEFAULT_MSG
                                      UUID_LIBRARY UUID_INCLUDE_DIR)

    mark_as_advanced(UUID_INCLUDE_DIR UUID_LIBRARY)
    set(UUID_INCLUDE_DIRS ${UUID_INCLUDE_DIR})
    set(UUID_LIBRARIES ${UUID_LIBRARY})
    if (libuuid_FOUND AND NOT TARGET libuuid::libuuid)
        add_library(libuuid::libuuid SHARED IMPORTED)
        set_target_properties(libuuid::libuuid PROPERTIES
                IMPORTED_LOCATION "${UUID_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${UUID_INCLUDE_DIR}"
        )
    endif ()
endif ()