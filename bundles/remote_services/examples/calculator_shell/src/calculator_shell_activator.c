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
#include <string.h>


#include "celix_shell_command.h"
#include "celix_api.h"

#include "add_command.h"
#include "sub_command.h"
#include "sqrt_command.h"

typedef struct calc_shell_activator {
    long addCmdSvcId;
    celix_shell_command_t addCmd;
    long subCmdSvcId;
    celix_shell_command_t subCmd;
    long sqrtCmdSvcId;
    celix_shell_command_t sqrtCmd;
} calc_shell_activator_t;

static celix_status_t calcShell_start(calc_shell_activator_t *activator, celix_bundle_context_t *ctx) {
    activator->addCmd.handle = ctx;
    activator->addCmd.executeCommand = addCommand_execute;
    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "add");
    activator->addCmdSvcId = celix_bundleContext_registerService(ctx, &activator->addCmd, CELIX_SHELL_COMMAND_SERVICE_NAME, props);

    activator->subCmd.handle = ctx;
    activator->subCmd.executeCommand = subCommand_execute;
    props = celix_properties_create();
    celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "sub");
    activator->subCmdSvcId = celix_bundleContext_registerService(ctx, &activator->subCmd, CELIX_SHELL_COMMAND_SERVICE_NAME, props);

    activator->sqrtCmd.handle = ctx;
    activator->sqrtCmd.executeCommand = sqrtCommand_execute;
    props = celix_properties_create();
    celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "sqrt");
    activator->sqrtCmdSvcId = celix_bundleContext_registerService(ctx, &activator->sqrtCmd, CELIX_SHELL_COMMAND_SERVICE_NAME, props);

    return CELIX_SUCCESS;
}

static celix_status_t calcShell_stop(calc_shell_activator_t *activator, celix_bundle_context_t *ctx) {
    celix_bundleContext_unregisterService(ctx, activator->addCmdSvcId);
    celix_bundleContext_unregisterService(ctx, activator->subCmdSvcId);
    celix_bundleContext_unregisterService(ctx, activator->sqrtCmdSvcId);
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(calc_shell_activator_t, calcShell_start, calcShell_stop);
