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

celix_subproject(LOG_SERVICE "Option to enable building the Log Service bundles" "OFF" DEPS framework)
if (LOG_SERVICE)
	
	SET_HEADER(BUNDLE_VERSION "1.0.0.incubating")
	SET_HEADER(BUNDLE_SYMBOLICNAME "apache_celix_log_service")
	SET_HEADERS("Bundle-Name: Apache Celix Log Service")
	
    bundle(log_service 
    	SOURCES
			private/src/log
			private/src/log_entry
			private/src/log_factory  
			private/src/log_service_impl 
			private/src/log_service_activator
			private/src/log_reader_service_impl
		    
		    private/include/log.h
		    private/include/log_factory.h
		    private/include/log_reader_service_impl.h
		    private/include/log_service_impl.h
    )
    
    install_bundle(log_service
        HEADERS
        	public/include/log_service.h
    )
    
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_service/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/log_service/private/include")
    target_link_libraries(log_service celix_framework)
endif (LOG_SERVICE)
