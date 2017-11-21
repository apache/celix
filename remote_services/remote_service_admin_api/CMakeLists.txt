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

add_library(remote_service_admin_api INTERFACE)
target_include_directories(remote_service_admin_api INTERFACE include)

install (FILES 
        include/remote_endpoint_impl.h
        include/remote_endpoint.h
        include/remote_proxy.h
        include/remote_service_admin.h
        include/export_registration.h
        include/import_registration.h
        include/endpoint_description.h
        include/endpoint_listener.h
        include/remote_constants.h
    DESTINATION 
        include/celix/remote_service_admin 
    COMPONENT 
        remote_service_admin
)
install (FILES 
        public/include/endpoint_listener.h
    DESTINATION 
        include/celix/endpoint_listener 
    COMPONENT 
        remote_service_admin
)

#Setup target aliases to match external usage
add_library(Celix::remote_service_admin_api ALIAS remote_service_admin_api)
