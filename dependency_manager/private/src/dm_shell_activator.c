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
/*
 * dm_shell_activator.c
 *
 *  \date       16 Oct 2015
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "bundle_context.h"
#include "service_registration.h"
#include "command.h"

#include "dm_shell_list_command.h"

struct bundle_instance {
    service_registration_pt reg;
    command_pt dmListCmd;
};

typedef struct bundle_instance * bundle_instance_pt;

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {

    struct bundle_instance *bi = calloc(sizeof (struct bundle_instance), 1);
    celix_status_t status = CELIX_SUCCESS;

    if (!bi) {
        status = CELIX_ENOMEM;
    }
    else if (userData == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
        free(bi);
    }
    else {
        if (status != CELIX_SUCCESS) {
            printf("DM:LIST Create failed\n");
            free(bi);
        }

        (*userData) = bi;
    }

    return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
    celix_status_t status = CELIX_SUCCESS;
    bundle_instance_pt bi = (bundle_instance_pt) userData;
    command_service_pt commandService = calloc(1, sizeof(*commandService));

    command_pt command = calloc(sizeof(*command),1);
    command->executeCommand = dmListCommand_execute;
    command->bundleContext = context;
    command->handle = NULL;
    command->name ="dm:list";
    command->shortDescription ="not_used";
    command->usage="not_used";

    commandService->getName             = dmListCommand_getName;
    commandService->command             = command;
    commandService->executeCommand      = dmListCommand_execute;
    commandService->getShortDescription = dmListCommand_getShortDescription;
    commandService->getUsage            = dmListCommand_getUsage;

    bundleContext_registerService(context, (char *) OSGI_SHELL_COMMAND_SERVICE_NAME, commandService, NULL, &bi->reg);

    return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
    bundle_instance_pt bi = (bundle_instance_pt) userData;
    serviceRegistration_unregister(bi->reg);
    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
    free(userData);
    return CELIX_SUCCESS;
}

