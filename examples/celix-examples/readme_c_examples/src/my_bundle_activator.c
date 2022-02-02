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

#include <celix_api.h>

typedef struct my_bundle_activator_data {
    /*the hello world bundle activator struct is empty*/
} my_bundle_activator_data_t;

static celix_status_t my_bundle_start(my_bundle_activator_data_t *data __attribute__((unused)), celix_bundle_context_t *ctx __attribute__((unused))) {
    printf("Hello world from bundle with id %li\n", celix_bundleContext_getBundleId(ctx));
    return CELIX_SUCCESS;
}

static celix_status_t my_bundle_stop(my_bundle_activator_data_t *data __attribute__((unused)), celix_bundle_context_t *ctx __attribute__((unused))) {
    printf("Goodbye world\n");
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(my_bundle_activator_data_t, my_bundle_start, my_bundle_stop)