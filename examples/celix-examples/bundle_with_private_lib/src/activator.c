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

#include <stdlib.h>
#include <stdio.h>

#include <celix_api.h>
#include <test.h>


struct userData {
    const char * word;
};

static celix_status_t activator_start(struct userData *data, celix_bundle_context_t *ctx) {
    data->word = "Import";
    const char *bndName = celix_bundle_getSymbolicName(celix_bundleContext_getBundle(ctx));
    printf("Hello %s\n", data->word);
    doo(bndName);
    return CELIX_SUCCESS;
}

static celix_status_t activator_stop(struct userData *data, celix_bundle_context_t *ctx __attribute__((unused))) {
    printf("Goodbye %s\n", data->word);
    return CELIX_SUCCESS;
}



CELIX_GEN_BUNDLE_ACTIVATOR(struct userData, activator_start, activator_stop)