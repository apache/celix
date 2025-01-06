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
celix_subproject(EVENT_ADMIN_REMOTE_PROVIDER_MQTT "Option to enable building the event admin remote provider bundle which base on mqtt" OFF)
if (EVENT_ADMIN_REMOTE_PROVIDER_MQTT)

    find_package(mosquitto REQUIRED)
    find_package(jansson REQUIRED)
    find_package(libuuid REQUIRED)

    set(EARPM_SRC
            src/celix_earpm_activator.c
            src/celix_earpm_impl.c
            src/celix_earpm_client.c
            src/celix_earpm_event_deliverer.c
            src/celix_earpm_broker_discovery.c
    )

    set(EARPM_DEPS
            Celix::event_admin_api
            Celix::event_admin_spi
            Celix::shell_api
            Celix::c_rsa_spi
            Celix::rsa_common
            Celix::log_helper
            Celix::framework
            Celix::utils
            mosquitto::libmosquitto
            jansson::jansson
            libuuid::libuuid
            )

    add_celix_bundle(event_admin_remote_provider_mqtt
        SYMBOLIC_NAME "apache_celix_event_remote_provider_mqtt"
        VERSION "1.0.0"
        NAME "Apache Celix Event Remote Provider Base On MQTT"
        GROUP "Celix/event_admin"
        FILENAME celix_event_admin_remote_provider_mqtt
        SOURCES
        ${EARPM_SRC}
    )

    target_link_libraries(event_admin_remote_provider_mqtt PRIVATE ${EARPM_DEPS})

    target_include_directories(event_admin_remote_provider_mqtt PRIVATE ${CMAKE_CURRENT_LIST_DIR}/src ${CMAKE_CURRENT_LIST_DIR}/private)

    install_celix_bundle(event_admin_remote_provider_mqtt EXPORT celix COMPONENT event_admin)

    #Setup target aliases to match external usage
    add_library(Celix::event_admin_remote_provider_mqtt ALIAS event_admin_remote_provider_mqtt)

    if (ENABLE_TESTING)
        add_library(event_admin_remote_provider_mqtt_cut STATIC ${EARPM_SRC})
        target_include_directories(event_admin_remote_provider_mqtt_cut PUBLIC ${CMAKE_CURRENT_LIST_DIR}/src ${CMAKE_CURRENT_LIST_DIR}/private)
        target_link_libraries(event_admin_remote_provider_mqtt_cut PUBLIC ${EARPM_DEPS})
        add_subdirectory(gtest)
    endif(ENABLE_TESTING)

endif ()
