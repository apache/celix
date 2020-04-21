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

#include "celix_api.h"
#include "celix_log_admin.h"

typedef struct celix_log_admin_activator {
    celix_log_admin_t *admin;
} celix_log_admin_activator_t;


static celix_status_t celix_logAdminActivator_start(celix_log_admin_activator_t* act, celix_bundle_context_t* ctx) {
    act->admin = celix_logAdmin_create(ctx);
    return CELIX_SUCCESS;
}

static celix_status_t celix_logAdminActivator_stop(celix_log_admin_activator_t* act, celix_bundle_context_t* ctx) {
    celix_logAdmin_destroy(act->admin);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(celix_log_admin_activator_t, celix_logAdminActivator_start, celix_logAdminActivator_stop);