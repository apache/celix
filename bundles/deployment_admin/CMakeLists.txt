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
    find_package(ZLIB REQUIRED)

    add_library(deployment_admin_api INTERFACE)
    target_include_directories(deployment_admin_api INTERFACE 
	    $<BUILD_INTERFACE:${CMAKE_CURRENT_LIST_DIR}/api>
	    $<INSTALL_INTERFACE:include/celix/deployment_admin>
    )

    add_celix_bundle(deployment_admin
        SYMBOLIC_NAME "apache_celix_deployment_admin"
        VERSION "0.0.2"
        NAME "Apache Celix Deployment Admin"
        GROUP "Celix/DeploymentAdmin"
        SOURCES
            src/deployment_package
            src/deployment_admin
            src/deployment_admin_activator
            src/ioapi
            src/miniunz
            src/unzip
            src/log
            src/log_store
            src/log_sync
    )

    target_compile_definitions(deployment_admin PRIVATE -DUSE_FILE32API)
    target_include_directories(deployment_admin PRIVATE src)
    target_include_directories(deployment_admin SYSTEM PRIVATE
            ${CURL_INCLUDE_DIRS}
            ${UUID_INCLUDE_DIRS}
            ${ZLIB_INCLUDE_DIRS}
    )
    target_link_libraries(deployment_admin PRIVATE ${CURL_LIBRARIES} ${UUID_LIBRARIES} ${ZLIB_LIBRARIES} deployment_admin_api)

    install(TARGETS deployment_admin_api EXPORT celix COMPONENT deployment_admin)
    install(DIRECTORY api/ DESTINATION include/celix/deployment_admin COMPONENT deployment_admin)
    install_celix_bundle(deployment_admin EXPORT celix COMPONENT deployment_admin)

    #Setup target aliases to match external usage
    add_library(Celix::deployment_admin_api ALIAS deployment_admin_api)
    add_library(Celix::deployment_admin ALIAS deployment_admin)

    add_celix_container(deployment-admin
        BUNDLES Celix::deployment_admin Celix::shell Celix::shell_tui Celix::log_service Celix::log_writer_stdout
        PROPERTIES
    		"deployment_admin_url=http://localhost:8080"
    		"deployment_admin_identification=celix"
    		"org.osgi.framework.storage.clean=onFirstInit"
    )

endif (DEPLOYMENT_ADMIN)
