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
#include <stdio.h>
#include <celix_api.h>
#include <celix_shell_command.h>

typedef struct use_command_service_example_data {
    //nop
} use_command_service_example_data_t;

static void useShellCommandCallback(void *handle __attribute__((unused)), void *svc) {
    celix_shell_command_t* cmdSvc = (celix_shell_command_t*)svc;
    cmdSvc->executeCommand(cmdSvc->handle, "my_command test call from C", stdout, stderr);
}

static celix_status_t use_command_service_example_start(use_command_service_example_data_t *data __attribute__((unused)), celix_bundle_context_t *ctx) {
    celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;
    opts.callbackHandle = NULL;
    opts.use = useShellCommandCallback;
    opts.filter.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
    opts.filter.filter = "(command.name=my_command)";
    bool called = celix_bundleContext_useServicesWithOptions(ctx, &opts);
    if (!called) {
        fprintf(stderr, "%s: Command service not called!\n", __PRETTY_FUNCTION__);
    }
    return CELIX_SUCCESS;
}

static celix_status_t use_command_service_example_stop(use_command_service_example_data_t *data __attribute__((unused)), celix_bundle_context_t *ctx __attribute__((unused))) {
    return CELIX_SUCCESS;
}

CELIX_GEN_BUNDLE_ACTIVATOR(use_command_service_example_data_t, use_command_service_example_start, use_command_service_example_stop)