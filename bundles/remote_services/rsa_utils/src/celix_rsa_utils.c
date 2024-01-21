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

#include "celix_rsa_utils.h"
#include "celix_properties.h"
#include "celix_constants.h"

celix_status_t
celix_rsaUtils_createServicePropertiesFromEndpointProperties(const celix_properties_t* endpointProperties,
                                                             celix_properties_t** serviceProperties) {
    celix_autoptr(celix_properties_t) props =  celix_properties_copy(endpointProperties);
    if (!props) {
        return CELIX_ENOMEM;
    }

    celix_status_t status = CELIX_SUCCESS;
    long bundleId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_BUNDLE_ID, -1L);
    if (bundleId >= 0) {
        status = celix_properties_setLong(props, CELIX_FRAMEWORK_BUNDLE_ID, bundleId);
    }

    const celix_properties_entry_t* entry = celix_properties_getEntry(props, CELIX_FRAMEWORK_SERVICE_RANKING);
    if (status == CELIX_SUCCESS && entry && entry->valueType == CELIX_PROPERTIES_VALUE_TYPE_STRING) {
        long ranking = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_RANKING, 0L);
        status = celix_properties_setLong(props, CELIX_FRAMEWORK_SERVICE_RANKING, ranking);
    }

    const char* versionStr = celix_properties_get(props, CELIX_FRAMEWORK_SERVICE_VERSION, NULL);
    if (status == CELIX_SUCCESS && versionStr) {
        celix_autoptr(celix_version_t) version = NULL;
        status = celix_version_parse(versionStr, &version);
        if (status == CELIX_SUCCESS) {
            status = celix_properties_assignVersion(props, CELIX_FRAMEWORK_SERVICE_VERSION, celix_steal_ptr(version));
        }
    }

    if (status == CELIX_SUCCESS) {
        *serviceProperties = celix_steal_ptr(props);
    }
    return status;
}
