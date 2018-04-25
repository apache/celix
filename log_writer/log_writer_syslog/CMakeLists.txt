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
celix_subproject(LOG_WRITER_SYSLOG "Option to enable building the Syslog Writer" OFF DEPS FRAMEWORK LOG_SERVICE)
if (LOG_WRITER_SYSLOG)
    find_package(Syslog REQUIRED)

    add_celix_bundle(log_writer_syslog
        VERSION 1.1.0
        SYMBOLIC_NAME "apache_celix_log_writer_syslog"
        NAME "Apache Celix Log Writer Syslog"
        SOURCES
            private/src/log_writer_syslog
    )

    IF(APPLE)
        target_link_libraries(log_writer_syslog PRIVATE -Wl,-all_load log_writer_common)
    else()
        if(ENABLE_ADDRESS_SANITIZER)
            #With asan there can be undefined symbols
            target_link_libraries(log_writer_syslog PRIVATE -Wl,--whole-archive log_writer_common -Wl,--no-whole-archive)
        else()
            target_link_libraries(log_writer_syslog PRIVATE -Wl,--no-undefined -Wl,--whole-archive log_writer_common -Wl,--no-whole-archive)
        endif()
    endif()

    target_link_libraries(log_writer_syslog PRIVATE Celix::log_service_ap)

    install_celix_bundle(log_writer_syslog EXPORT celix)
    add_library(Celix::log_writer_syslog ALIAS log_writer_syslog)
endif (LOG_WRITER_SYSLOG)
