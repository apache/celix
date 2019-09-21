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
/**
 * activator.c
 *
 *  \date       Oct 20, 2014
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <celix_errno.h>

#include <stdlib.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "celix_constants.h"

#include "bonjour_shell.h"

#include <celix_errno.h>
#include <service_tracker.h>
#include <shell.h>

struct bundle_instance {
        bonjour_shell_pt shell;
        service_tracker_pt tracker;
};

typedef struct bundle_instance *bundle_instance_pt;

celix_status_t bundleActivator_create(bundle_context_pt context, void **userData) {
        celix_status_t status = CELIX_SUCCESS;
        struct bundle_instance *bi = calloc(1, sizeof(*bi));
        
        if (bi) {
                bi->shell = NULL;
                bi->tracker = NULL;
                (*userData) = bi;
        } else {
                status = CELIX_ENOMEM;
                
        }
        return status;
}

celix_status_t bundleActivator_start(void * userData, bundle_context_pt context) {
        celix_status_t status = CELIX_SUCCESS;
        bundle_instance_pt bi = (bundle_instance_pt) userData;

        const char *uuid = NULL;
        bundleContext_getProperty(context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
        const char *hostname = NULL;
        bundleContext_getProperty(context, "HOSTNAME", &hostname);
        const char *bonjourShellId = NULL;
        bundleContext_getProperty(context, "bonjour.shell.id", &bonjourShellId);

        char id[128];
        if (bonjourShellId != NULL) {
                snprintf(id, 128, "%s", bonjourShellId);
        } else if (hostname != NULL) {
                snprintf(id, 128, "Celix-%.8s@%s", uuid, hostname);
        } else {
                snprintf(id, 128, "Celix-%.8s", uuid);
        }
        status = bonjourShell_create(id, &bi->shell);

        service_tracker_customizer_pt cust = NULL;
        serviceTrackerCustomizer_create(bi->shell, NULL, bonjourShell_addShellService, NULL, bonjourShell_removeShellService, &cust);
        serviceTracker_create(context, (char *) OSGI_SHELL_SERVICE_NAME, cust, &bi->tracker);
        serviceTracker_open(bi->tracker);


        return status;
}

celix_status_t bundleActivator_stop(void * userData, bundle_context_pt context) {
        celix_status_t status = CELIX_SUCCESS;
        bundle_instance_pt bi = (bundle_instance_pt) userData;

        serviceTracker_close(bi->tracker);

        return status;
}

celix_status_t bundleActivator_destroy(void * userData, bundle_context_pt context) {
        celix_status_t status = CELIX_SUCCESS;
        bundle_instance_pt bi = (bundle_instance_pt) userData;

        serviceTracker_destroy(bi->tracker);
        bonjourShell_destroy(bi->shell);

        return status;
}
