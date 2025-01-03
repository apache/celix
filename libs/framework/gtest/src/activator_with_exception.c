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

#include "celix_bundle_activator.h"
#include "celix_compiler.h"
#include <stdio.h>

struct bundle_act {

};

static celix_status_t act_start(struct bundle_act *act CELIX_UNUSED, celix_bundle_context_t *ctx CELIX_UNUSED) {
    printf("Return a bundle 'exception'\n");
    return CELIX_BUNDLE_EXCEPTION;
}

static celix_status_t act_stop(struct bundle_act *act CELIX_UNUSED, celix_bundle_context_t *ctx CELIX_UNUSED) {
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(struct bundle_act, act_start, act_stop);