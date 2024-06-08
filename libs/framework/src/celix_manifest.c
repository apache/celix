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

#include "celix_manifest.h"

#include "celix_properties.h"
#include "celix_stdlib_cleanup.h"
#include "celix_err.h"

struct celix_manifest{
    celix_properties_t* attributes;
};

celix_status_t celix_manifest_create(celix_properties_t* attributes, celix_manifest_t** manifestOut) {
    celix_autofree celix_manifest_t* manifest = calloc(1, sizeof(*manifest));
    if (!manifest) {
        celix_properties_destroy(attributes);
        celix_err_push("Failed to allocate memory for manifest");
        return ENOMEM;
    }

    manifest->attributes = attributes ? attributes : celix_properties_create();
    if (!manifest->attributes) {
        return ENOMEM;
    }

    *manifestOut = celix_steal_ptr(manifest);
    return CELIX_SUCCESS;
}


celix_status_t celix_manifest_createFromFile(const char* filename, celix_manifest_t** manifestOut) {
    celix_properties_t* properties = NULL;
    celix_status_t status = celix_properties_load(filename, 0, &properties);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    return celix_manifest_create(properties, manifestOut);
}

void celix_manifest_destroy(celix_manifest_t* manifest) {
    if (manifest) {
        celix_properties_destroy(manifest->attributes);
        free(manifest);
    }
}

const celix_properties_t* celix_manifest_getAttributes(celix_manifest_t* manifest) {
    return manifest->attributes;
}

bool celix_manifest_validate(celix_manifest_t* manifest) {
    return true; //TODO implement
}