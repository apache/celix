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
#include <assert.h>
#include <celix_bundle_context.h>
#include <celix_constants.h>
#include <celix_framework.h>
#include <celix_framework_factory.h>
#include <celix_properties.h>
#include <celix_shell_command.h>
#include <celix_shell.h>
#include <stdio.h>

static void use(void *handle, void *svc) {
    celix_shell_t *shell = svc;
    celix_status_t status = shell->executeCommand(shell->handle, "lb", stdout, stderr);
    assert(status == CELIX_SUCCESS );
    status = shell->executeCommand(shell->handle, "3rdparty::test", stdout, stderr);
    assert(status == CELIX_SUCCESS );
}

static bool executeCommand(void *handle, const char *commandLine, FILE *outStream, FILE *errorStream) {
    fprintf(outStream, "Executing %s\n", commandLine);
    return true;
}

int main() {
    celix_framework_t* fw = NULL;
    celix_bundle_context_t *ctx = NULL;
    celix_properties_t *properties = NULL;

    properties = celix_properties_create();
    celix_properties_setBool(properties, "LOGHELPER_ENABLE_STDOUT_FALLBACK", true);
    celix_properties_setBool(properties, CELIX_FRAMEWORK_CLEAN_CACHE_DIR_ON_CREATE, true);
    celix_properties_set(properties, CELIX_FRAMEWORK_CACHE_DIR, ".cacheBundleContextTestFramework");

    fw = celix_frameworkFactory_createFramework(properties);
    ctx = celix_framework_getFrameworkContext(fw);
    long bndId = celix_bundleContext_installBundle(ctx, SHELL_BUNDLE_LOCATION, true);
    assert(bndId >= 0);

    celix_shell_command_t cmdService;
    cmdService.handle = NULL;
    cmdService.executeCommand = executeCommand;
    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "3rdparty::test");
    long svcId = celix_bundleContext_registerService(ctx, &cmdService, CELIX_SHELL_COMMAND_SERVICE_NAME, props);

    celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;
    opts.filter.serviceName = CELIX_SHELL_SERVICE_NAME;
    opts.callbackHandle = NULL;
    opts.waitTimeoutInSeconds = 1.0;
    opts.flags = CELIX_SERVICE_USE_DIRECT | CELIX_SERVICE_USE_SOD;
    opts.use = use;
    bool called = celix_bundleContext_useServiceWithOptions(ctx, &opts);
    assert(called);
    celix_bundleContext_unregisterService(ctx, svcId);
    celix_frameworkFactory_destroyFramework(fw);
    return 0;
}


