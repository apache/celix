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


# - Try to find OpenSSL
# Once done this will define
#  OPENSSL_FOUND - System has OpenSSL
#  OPENSSL_INCLUDE_DIRS - The OpenSSL include directories
#  OPENSSL_LIBRARIES - The libraries needed to use OpenSSL
#  OPENSSL_DEFINITIONS - Compiler switches required for using OpenSSL

find_path(OPENSSL_INCLUDE_DIR ssl.h crypto.h
          /usr/include/openssl
          /usr/local/include/openssl 
          /usr/local/opt/openssl/include/openssl
          ${OPENSSL_DIR}/include/openssl)

find_library(OPENSSL_LIBRARY NAMES ssl
             PATHS /usr/lib /usr/local/lib  /usr/local/opt/openssl/lib ${OPENSSL_DIR}/lib)

set(OPENSSL_LIBRARIES ${OPENSSL_LIBRARY} )
set(OPENSSL_INCLUDE_DIRS ${OPENSSL_INCLUDE_DIR} )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set OPENSSL_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(OpenSSL  DEFAULT_MSG
                                  OPENSSL_LIBRARY OPENSSL_INCLUDE_DIR)

mark_as_advanced(OPENSSL_INCLUDE_DIR OPENSSL_LIBRARY)
