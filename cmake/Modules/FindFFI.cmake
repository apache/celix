
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


# - Try to find libffi define the variables for the binaries/headers and include 
#
# Once done this will define
#  FFI_FOUND - System has libffi
#  ffi::lib - Imported target for the libffi

# try using pkg-config if available
find_package(PkgConfig QUIET)

if (PkgConfig_FOUND)
    if (APPLE)
        #set brew location for pkg-config
        set(ENV{PKG_CONFIG_PATH} "/usr/local/opt/libffi/lib/pkgconfig:$ENV{PKG_CONFIG_PATH}")
    endif ()
    pkg_check_modules(LIBFFI REQUIRED libffi)

    #use found LIBFFI pkg config info to search for the abs path fo the libffi lib.
    #Note that the abs path info  is not present in the pkg-config results (only the -l and -L arguments)
    find_library(FFI_ABS_LIB NAMES ffi PATHS ${LIBFFI_LIBRARY_DIRS} NO_DEFAULT_PATH NO_PACKAGE_ROOT_PATH NO_CMAKE_PATH NO_CMAKE_SYSTEM_PATH)
    if (NOT FFI_ABS_LIB)
        message(FATAL_ERROR "Cannot find abs path of libffi based on pkgconfig results")
    endif ()

    add_library(ffi::lib SHARED IMPORTED)
    set_target_properties(ffi::lib PROPERTIES
        IMPORTED_LOCATION "${FFI_ABS_LIB}"
        INTERFACE_INCLUDE_DIRECTORIES "${LIBFFI_INCLUDE_DIRS}"
    )

    unset(FFI_ABS_LIB)
else()
    mark_as_advanced(GNUTLS_INCLUDE_DIR GNUTLS_LIBRARY)

    find_library(FFI_LIBRARY NAMES ffi libffi
            PATHS $ENV{FFI_DIR} ${FFI_DIR} /usr /usr/local /opt/local
            PATH_SUFFIXES lib lib64 x86_64-linux-gnu lib/x86_64-linux-gnu
            HINTS ${PC_LIBFFI_LIBDIR} ${PC_LIBFFI_LIBRARY_DIRS}
    )

    find_path(FFI_INCLUDE_DIR ffi.h
            PATHS $ENV{FFI_DIR} ${FFI_DIR} /usr /usr/local /opt/local
            PATH_SUFFIXES include include/ffi include/x86_64-linux-gnu x86_64-linux-gnu
            HINTS ${PC_LIBFFI_INCLUDEDIR} ${PC_LIBFFI_INCLUDE_DIRS}
    )

    include(FindPackageHandleStandardArgs)
    # handle the QUIETLY and REQUIRED arguments and set FFI_FOUND to TRUE
    # if all listed variables are TRUE
    find_package_handle_standard_args(FFI  DEFAULT_MSG
                                      FFI_LIBRARY FFI_INCLUDE_DIR)
    mark_as_advanced(FFI_INCLUDE_DIR FFI_LIBRARY)

    if(FFI_FOUND)
        set(FFI_LIBRARIES ${FFI_LIBRARY})
        set(FFI_INCLUDE_DIRS ${FFI_INCLUDE_DIR})
        add_library(ffi::lib SHARED IMPORTED)
        set_target_properties(ffi::lib PROPERTIES
                IMPORTED_LOCATION "${FFI_LIBRARY}"
                INTERFACE_INCLUDE_DIRECTORIES "${FFI_INCLUDE_DIR}"
        )
    endif()
endif ()