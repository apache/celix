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
 *  Unless required by applicable law or agreed to in writing,
 *  software distributed under the License is distributed on an
 *  "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 *  specific language governing permissions and limitations
 *  under the License.
 */
#include <celix_bundle_activator.h>
#include <celix_bundle_context.h>
#include <celix_framework.h>
#include <stdio.h>

typedef struct activator_data {
    void* data;
} activator_data_t;


static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    const char *key = NULL;
    printf("\nHello world from C bundle with id %li\n", celix_bundle_getId(celix_bundleContext_getBundle(ctx)));
    celix_array_list_t *bundles = celix_bundleContext_listInstalledBundles(ctx);
    for (int i = 0; i < celix_arrayList_size(bundles); i++) {
        long bndId = celix_arrayList_getLong(bundles, i);
        printf("Bundle %ld: %s\n", bndId, celix_bundleContext_getBundleSymbolicName(ctx, bndId));
    }
    celix_arrayList_destroy(bundles);
    celix_framework_stopBundleAsync(celix_bundleContext_getFramework(ctx), CELIX_FRAMEWORK_BUNDLE_ID); // to make container quit immediately
    return CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    printf("Goodbye world from C bundle with id %li\n", celix_bundle_getId(celix_bundleContext_getBundle(ctx)));
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
