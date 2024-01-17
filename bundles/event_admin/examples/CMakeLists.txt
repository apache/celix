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

celix_subproject(EVENT_ADMIN_EXAMPLES "Option to enable building the Event Admin examples" OFF)
if (EVENT_ADMIN_EXAMPLES)
    add_subdirectory(event_publisher)
    add_subdirectory(event_handler)

    add_celix_container(event_admin_example
            LAUNCHER Celix::launcher
            NAME "event_admin_example"
            GROUP "event_admin"
            BUNDLES
            Celix::shell
            Celix::shell_tui
            Celix::log_admin
            Celix::event_admin
            event_publisher_example
            event_handler_example
            USE_CONFIG
    )
endif (EVENT_ADMIN_EXAMPLES)




