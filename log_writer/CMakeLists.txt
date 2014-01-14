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

celix_subproject(LOG_WRITER "Option to enable building the Log Writer bundle" "OFF" DEPS FRAMEWORK LOG_SERVICE)
if (LOG_WRITER)

    SET_HEADER(BUNDLE_VERSION "1.0.0.incubating")
	SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_log_writer")
	SET_HEADERS("Bundle-Name: Apache Celix Log Writer")

    bundle(log_writer SOURCES 
    	private/src/log_writer_activator 
    	private/src/log_writer
    
        private/include/log_writer.h    
    )
    
    install_bundle(log_writer)
    
	include_directories("private/include")
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
    target_link_libraries(log_writer celix_framework)
endif (LOG_WRITER)