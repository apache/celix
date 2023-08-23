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

# - Try to find DNSSD
#
# Once done this will define
#  DNSSD_FOUND - System has dns_sd
#  DNSSD::DNSSD target (if found)

find_library(DNS_SD_LIBRARY NAMES dns_sd dns_services
        PATHS $ENV{DNS_SD_DIR} ${DNS_SD_DIR} /usr /usr/local /opt/local
        PATH_SUFFIXES lib lib64 x86_64-linux-gnu lib/x86_64-linux-gnu
        )

find_path(DNS_SD_INCLUDE_DIR dns_sd.h
        PATHS $ENV{DNS_SD_DIR} ${DNS_SD_DIR} /usr /usr/local /opt/local
        PATH_SUFFIXES include include/ffi include/x86_64-linux-gnu x86_64-linux-gnu
        )

include(FindPackageHandleStandardArgs)
# handle the QUIETLY and REQUIRED arguments and set libzip_FOUND to TRUE
# if all listed variables are TRUE
find_package_handle_standard_args(DNSSD  DEFAULT_MSG
        DNS_SD_LIBRARY DNS_SD_INCLUDE_DIR)

if(DNSSD_FOUND AND NOT TARGET DNSSD::DNSSD)
    add_library(DNSSD::DNSSD SHARED IMPORTED)
    set_target_properties(DNSSD::DNSSD PROPERTIES
            INTERFACE_INCLUDE_DIRECTORIES "${DNS_SD_INCLUDE_DIR}"
            IMPORTED_LOCATION "${DNS_SD_LIBRARY}"
            )
endif()

unset(DNS_SD_LIBRARY)
unset(DNS_SD_INCLUDE_DIR)
