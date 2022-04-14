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

# - Try to find libzip
#
# Once done this will define
#  libzip_FOUND - System has libffi
#  libzip::libzip target (if found)

find_library(LIBZIP_LIBRARY NAMES zip
        PATHS $ENV{LIBZIP_DIR} ${LIBZIP_DIR} /usr /usr/local /opt/local
        PATH_SUFFIXES lib lib64 x86_64-linux-gnu lib/x86_64-linux-gnu
        )

find_path(LIBZIP_INCLUDE_DIR zip.h
        PATHS $ENV{LIBZIP_DIR} ${LIBZIP_DIR} /usr /usr/local /opt/local
        PATH_SUFFIXES include include/ffi include/x86_64-linux-gnu x86_64-linux-gnu
        )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set libzip_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(libzip  DEFAULT_MSG
        LIBZIP_LIBRARY LIBZIP_INCLUDE_DIR)

if(libzip_FOUND AND NOT TARGET libzip::libzip)
    add_library(libzip::libzip IMPORTED STATIC GLOBAL)
    set_target_properties(libzip::libzip PROPERTIES
            IMPORTED_LOCATION "${LIBZIP_LIBRARY}"
            INTERFACE_INCLUDE_DIRECTORIES "${LIBZIP_INCLUDE_DIR}"
    )
endif()

unset(LIBZIP_LIBRARY)
unset(LIBZIP_INCLUDE_DIR)