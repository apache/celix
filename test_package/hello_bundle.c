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
#include <celix_api.h>
#include <celix_log_helper.h>
#include <stdio.h>

typedef struct activator_data {
    celix_log_helper_t *logger;
} activator_data_t;


static celix_status_t activator_start(activator_data_t *data, celix_bundle_context_t *ctx) {
    const char *key = NULL;
    data->logger = celix_logHelper_create(ctx, "test");
    printf("Hello world from C bundle with id %li\n", celix_bundle_getId(celix_bundleContext_getBundle(ctx)));
    celix_properties_t *prop = celix_properties_create();
    celix_properties_set(prop, "name", "FOO");
    celix_properties_setLong(prop, "age", 39);
    celix_properties_setBool(prop, "married", true);
    celix_properties_setDouble(prop, "height", 1.75);
    CELIX_PROPERTIES_FOR_EACH(prop, key) {
        celix_logHelper_info(data->logger,"[Hello Bundle] |- %s=%s\n", key, celix_properties_get(prop, key, NULL));
    }
    celix_properties_store(prop, "prop.txt", NULL);
    celix_properties_destroy(prop);
    celix_framework_stopBundleAsync(celix_bundleContext_getFramework(ctx), CELIX_FRAMEWORK_BUNDLE_ID); // make to container quit immediately
    return CELIX_SUCCESS;
}

static celix_status_t activator_stop(activator_data_t *data, celix_bundle_context_t *ctx) {
    celix_logHelper_info(data->logger, "Goodbye world from C bundle with id %li\n", celix_bundle_getId(celix_bundleContext_getBundle(ctx)));
    celix_logHelper_destroy(data->logger);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(activator_data_t, activator_start, activator_stop)
