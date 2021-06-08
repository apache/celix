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

celix_subproject(REMOTE_SERVICE_ADMIN "Option to enable building the C++17 Remote Service Admin Service bundles" OFF)
if (REMOTE_SERVICE_ADMIN)
    message(WARNING "The C++ Remote Service Admin is still experimental; The API, SPI and implementation is not stable and will change")
    add_subdirectory(rsa_spi)
    add_subdirectory(admin)
    add_subdirectory(discovery_configured)

    find_package(ZMQ)
    find_package(CZMQ)
    if (ZMQ_FOUND AND CZMQ_FOUND)
        add_subdirectory(integration)
    else ()
        message(STATUS "C++ RSA integration test will not be build, because they require zmq and czmq")
    endif ()

    #NOTE the topology manager is not yet used. The discovery and RSA need to be refactor for this
    #add_subdirectory(topology_manager)
endif()