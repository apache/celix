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

#include "shell_private.h"
#include "bundle_activator.h"
#include "std_commands.h"
#include "service_tracker.h"
#include "celix_constants.h"
#include "celix_shell_command.h"
#include "std_commands.h"


struct shell_bundle_activator {
    shell_t *shell;
    celix_shell_t shellService;
    celix_std_commands_t* stdCommands;

	long shellSvcId;
	long trackerId;
	long legacyTrackerId;
};

typedef struct shell_bundle_activator shell_bundle_activator_t;

celix_status_t bundleActivator_create(celix_bundle_context_t* ctx, void **_pptr) {
	celix_status_t status = CELIX_SUCCESS;

    shell_bundle_activator_t* activator = NULL;

    if (!_pptr || !ctx) {
        status = CELIX_ENOMEM;
    }

    if (status == CELIX_SUCCESS) {
        activator = calloc(1, sizeof(*activator));
        if (!activator) {
            status = CELIX_ENOMEM;
        }
    }

    if (status == CELIX_SUCCESS) {
        activator->shell = shell_create(ctx);
    }

    if (status == CELIX_SUCCESS) {
        *_pptr = activator;
    }


    if (status != CELIX_SUCCESS) {
        bundleActivator_destroy(activator, ctx);
    }

	return status;
}

celix_status_t bundleActivator_start(void *activatorData, celix_bundle_context_t* ctx) {
	celix_status_t status = CELIX_SUCCESS;

    shell_bundle_activator_t* activator  = (shell_bundle_activator_t*) activatorData;

    if (!activator || !ctx) {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    if (status == CELIX_SUCCESS) {
        activator->stdCommands = celix_stdCommands_create(ctx);
    }

    if (status == CELIX_SUCCESS) {
        activator->shellService.handle = activator->shell;
        activator->shellService.executeCommand = (void*)shell_executeCommand;
        activator->shellService.getCommandDescription = (void*)shell_getCommandDescription;
        activator->shellService.getCommandUsage = (void*)shell_getCommandUsage;
        activator->shellService.getCommands = (void*)shell_getCommands;

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.serviceName = CELIX_SHELL_SERVICE_NAME;
        opts.serviceVersion = CELIX_SHELL_SERVICE_VERSION;
        opts.svc = &activator->shellService;

        activator->shellSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);
    }

	if (status == CELIX_SUCCESS) {
	    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
	    opts.callbackHandle = activator->shell;
	    opts.addWithProperties = (void*) shell_addCommand;
	    opts.removeWithProperties = (void*) shell_removeCommand;
	    opts.filter.ignoreServiceLanguage = true;
	    opts.filter.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
	    activator->trackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    activator->legacyTrackerId = -1L;
#ifdef CELIX_INSTALL_DEPRECATED_API
    if (status == CELIX_SUCCESS) {
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.callbackHandle = activator->shell;
        opts.addWithProperties = (void*) shell_addLegacyCommand;
        opts.removeWithProperties = (void*) shell_removeLegacyCommand;
        opts.filter.ignoreServiceLanguage = true;
        opts.filter.serviceName = OSGI_SHELL_COMMAND_SERVICE_NAME;
        activator->legacyTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }
#endif

	return status;
}

celix_status_t bundleActivator_stop(void *activatorData, celix_bundle_context_t* ctx) {
    celix_status_t status = CELIX_SUCCESS;

    shell_bundle_activator_t* activator = activatorData;

    if (activator) {
        celix_stdCommands_destroy(activator->stdCommands);
        celix_bundleContext_unregisterService(ctx, activator->shellSvcId);
        celix_bundleContext_stopTracker(ctx, activator->trackerId);
        celix_bundleContext_stopTracker(ctx, activator->legacyTrackerId);

    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t bundleActivator_destroy(void *activatorData, celix_bundle_context_t* __attribute__((__unused__)) ctx) {
    celix_status_t status = CELIX_SUCCESS;
    shell_bundle_activator_t* activator = activatorData;


    if (activator) {
        shell_destroy(activator->shell);
        free(activator);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}
