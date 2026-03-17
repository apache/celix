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

# - Try to find libuv
# 	Once done this will define
#  libuv_FOUND - System has libuv
#  LIBUV_INCLUDE_DIR - The libuv include directory
#  LIBUV_LIBRARY - The libuv library
#  uv - Imported target for libuv

find_package(libuv CONFIG QUIET)

if (NOT libuv_FOUND)
    find_package(PkgConfig QUIET)
    if (PkgConfig_FOUND)
        pkg_check_modules(LIBUV QUIET libuv)
    endif ()
endif ()

if (NOT libuv_FOUND)
    find_path(LIBUV_INCLUDE_DIR
        NAMES uv.h
        HINTS ${LIBUV_INCLUDEDIR} ${LIBUV_INCLUDE_DIRS}
    )

    find_library(LIBUV_LIBRARY
        NAMES uv libuv
        HINTS ${LIBUV_LIBDIR} ${LIBUV_LIBRARY_DIRS}
    )

    include(FindPackageHandleStandardArgs)
    find_package_handle_standard_args(libuv DEFAULT_MSG LIBUV_LIBRARY LIBUV_INCLUDE_DIR)
    mark_as_advanced(LIBUV_INCLUDE_DIR LIBUV_LIBRARY)
endif ()

if (libuv_FOUND AND NOT TARGET libuv::uv AND TARGET libuv::libuv)
    #Note: libuv cmake config possible defines libuv::libuv target
    #      and conan libuv package defines uv target
    #      so create an alias target uv for consistency
    add_library(libuv::uv ALIAS libuv::libuv)
elseif (libuv_FOUND AND NOT TARGET libuv::uv AND LIBUV_LIBRARY AND LIBUV_INCLUDE_DIR)
    add_library(libuv::uv SHARED IMPORTED)
    set_target_properties(libuv::uv PROPERTIES
        IMPORTED_LOCATION "${LIBUV_LIBRARY}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBUV_INCLUDE_DIR}"
    )
endif ()
