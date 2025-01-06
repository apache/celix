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

add_library(mosquitto_ei STATIC src/mosquitto_ei.cc)

find_package(mosquitto REQUIRED)

target_include_directories(mosquitto_ei PUBLIC ${CMAKE_CURRENT_LIST_DIR}/include)
target_link_libraries(mosquitto_ei PUBLIC Celix::error_injector mosquitto::libmosquitto)

target_link_options(mosquitto_ei INTERFACE
        LINKER:--wrap,mosquitto_new
        LINKER:--wrap,mosquitto_property_add_int32
        LINKER:--wrap,mosquitto_property_add_string_pair
        LINKER:--wrap,mosquitto_property_add_string
        LINKER:--wrap,mosquitto_property_add_binary
        LINKER:--wrap,mosquitto_int_option
        LINKER:--wrap,mosquitto_will_set_v5
        LINKER:--wrap,mosquitto_subscribe_v5
        LINKER:--wrap,mosquitto_unsubscribe
        LINKER:--wrap,mosquitto_publish_v5
        LINKER:--wrap,mosquitto_property_copy_all
        LINKER:--wrap,mosquitto_property_read_string
        LINKER:--wrap,mosquitto_property_read_binary
        LINKER:--wrap,mosquitto_property_read_string_pair
)
add_library(Celix::mosquitto_ei ALIAS mosquitto_ei)
