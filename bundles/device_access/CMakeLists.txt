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

celix_subproject(DEVICE_ACCESS "Option to enable building the Device Access Service bundles" ON)
if(DEVICE_ACCESS)
    message(WARNING "DEVICE_ACCESS is considered unstable, because develop of these bundles has been dormant for a while")

    add_subdirectory(device_access)
    add_subdirectory(driver_locator)
    add_subdirectory(example)
endif(DEVICE_ACCESS)
