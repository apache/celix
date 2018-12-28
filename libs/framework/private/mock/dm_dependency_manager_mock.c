/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#include <CppUTestExt/MockSupport_c.h>
#include "CppUTestExt/MockSupport_c.h"

#include "dm_dependency_manager.h"


celix_dependency_manager_t* celix_private_dependencyManager_create(celix_bundle_context_t *context) {
    mock_c()->actualCall("celix_private_dependencyManager_create")
            ->withPointerParameters("context", context);
    return mock_c()->returnValue().value.pointerValue;
}

void celix_private_dependencyManager_destroy(celix_dependency_manager_t *manager) {
    mock_c()->actualCall("celix_private_dependencyManager_destroy")
            ->withPointerParameters("manager", manager);
}

celix_status_t celix_dependencyManager_removeAllComponents(celix_dependency_manager_t *manager) {
    mock_c()->actualCall("celix_dependencyManager_removeAllComponents")
            ->withPointerParameters("manager", manager);
    return mock_c()->returnValue().value.intValue;
}