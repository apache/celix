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

celix_subproject(DEPLOYMENT_ADMIN "Option to enable building the Deployment Admin Service bundles" ON DEPS framework launcher shell_tui log_writer)
if (DEPLOYMENT_ADMIN)
	
    find_package(CURL REQUIRED)
    find_package(UUID REQUIRED)

    add_definitions(-DUSE_FILE32API)

    include_directories("${CURL_INCLUDE_DIR}")
    include_directories("${UUID_INCLUDE_DIR}")
    include_directories("${PROJECT_SOURCE_DIR}/utils/public/include")
    include_directories("${PROJECT_SOURCE_DIR}/deployment_admin/private/include")
    include_directories("${PROJECT_SOURCE_DIR}/deployment_admin/public/include")
    
    add_bundle(deployment_admin
        SYMBOLIC_NAME "apache_celix_deployment_admin"
        VERSION "0.0.2"
        NAME "Apache Celix Deployment Admin"
        SOURCES
            private/src/deployment_package
            private/src/deployment_admin
            private/src/deployment_admin_activator
            private/src/ioapi
            private/src/miniunz
            private/src/unzip
            private/src/log
            private/src/log_store
            private/src/log_sync

            private/include/deployment_admin.h
            private/include/deployment_package.h
            private/include/ioapi.h
            private/include/log.h
            private/include/log_event.h
            private/include/log_store.h
            private/include/log_sync.h
            private/include/miniunz.h
            private/include/unzip.h
    )

    
    install_bundle(deployment_admin
    	HEADERS
    		public/include/resource_processor.h
	)
    
    target_link_libraries(deployment_admin celix_framework ${CURL_LIBRARIES})


    add_deploy(deployment-admin
        BUNDLES deployment_admin shell shell_tui log_service log_writer
        PROPERTIES
    		"deployment_admin_url=http://localhost:8080"
    		"deployment_admin_identification=celix"
    		"org.osgi.framework.storage.clean=onFirstInit"
    )
endif (DEPLOYMENT_ADMIN)
