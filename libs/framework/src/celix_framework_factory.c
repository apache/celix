/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include "celix_framework_factory.h"
#include "framework.h"

framework_t* celix_frameworkFactory_createFramework(celix_properties_t *config) {
    framework_t* fw = NULL;

    if (config == NULL) {
        config = celix_properties_create();
    }

    if (config == NULL) {
        return NULL;
    }

    celix_status_t status = framework_create(&fw, config);
    if (status != CELIX_SUCCESS) {
        celix_properties_destroy(config);
        return NULL;
    }

    status = framework_start(fw);
    if (status != CELIX_SUCCESS) {
        framework_destroy(fw);
        return NULL;
    }

    return fw;
}

void celix_frameworkFactory_destroyFramework(celix_framework_t *fw) {
    if (fw != NULL) {
        framework_stop(fw);
        framework_destroy(fw);
    }
}
