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

    add_bundle(log_writer_syslog 
        VERSION 1.1.0
        SYMBOLIC_NAME "apache_celix_log_writer_syslog"
        NAME "Apache Celix Log Writer Syslog"
        SOURCES
        ${PROJECT_SOURCE_DIR}/log_writer/log_writer/private/src/log_writer_activator
        ${PROJECT_SOURCE_DIR}/log_writer/log_writer/private/src/log_writer
        ${PROJECT_SOURCE_DIR}/log_writer/log_writer/private/include/log_writer.h

        private/src/log_writer_syslog
    )

    install_bundle(log_writer_syslog)
        
    target_link_libraries(log_writer_syslog celix_framework)
        
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_writer/log_writer/private/include")
endif (LOG_WRITER_SYSLOG)
