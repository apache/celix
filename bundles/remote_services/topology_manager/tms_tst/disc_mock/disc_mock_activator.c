/*
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
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <service_tracker_customizer.h>
#include <service_tracker.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_registration.h"
#include "service_reference.h"
#include "celix_errno.h"

#include "disc_mock_service.h"
#include "constants.h"
#include "remote_constants.h"

celix_status_t discovery_endpointAdded(void *handle, endpoint_description_t *endpoint, char *matchedFilter);
celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_t *endpoint, char *matchedFilter);

celix_status_t bundleActivator_create(celix_bundle_context_t *context, void **out) {
    celix_status_t status = CELIX_SUCCESS;
    struct disc_mock_activator *act = calloc(1, sizeof(*act));
    if (act != NULL) {
        act->context = context;
        discMockService_create(act, &act->serv);
        act->endpointListener = NULL;
        act->endpointListenerService = NULL;
        status = arrayList_create(&act->endpointList);
    } else {
        status = CELIX_ENOMEM;
    }

    if (status == CELIX_SUCCESS) {
        *out = act;
    } else if (act != NULL) {
        free(act);
    }

    return CELIX_SUCCESS;
}

celix_status_t bundleActivator_start(void * userData, celix_bundle_context_t *context) {
    celix_status_t status;
    struct disc_mock_activator * act = userData;
    const char *uuid = NULL;

    act->reg = NULL;
    status = bundleContext_registerService(context, DISC_MOCK_SERVICE_NAME, act->serv, NULL, &act->reg);

    bundleContext_getProperty(context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);

    if (!uuid) {
        return CELIX_ILLEGAL_STATE;
    }

    size_t len = 11 + strlen(OSGI_FRAMEWORK_OBJECTCLASS) + strlen(OSGI_RSA_ENDPOINT_FRAMEWORK_UUID) + strlen(uuid);
    char *scope = malloc(len + 1);
    if (!scope) {
        return CELIX_ENOMEM;
    }

    sprintf(scope, "(&(%s=*)(%s=%s))", OSGI_FRAMEWORK_OBJECTCLASS, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
    scope[len] = 0;

    celix_properties_t *props = celix_properties_create();
    celix_properties_set(props, "DISCOVERY", "true");
    celix_properties_set(props, (char *) OSGI_ENDPOINT_LISTENER_SCOPE, scope);

    if (status == CELIX_SUCCESS) {
        endpoint_listener_t *endpointListener = calloc(1, sizeof(struct endpoint_listener));

        if (endpointListener) {
            endpointListener->handle = act;
            endpointListener->endpointAdded = discovery_endpointAdded;
            endpointListener->endpointRemoved = discovery_endpointRemoved;

            status = bundleContext_registerService(context, (char *) OSGI_ENDPOINT_LISTENER_SERVICE, endpointListener, props, &act->endpointListenerService);

            if (status == CELIX_SUCCESS) {
                act->endpointListener = endpointListener;
            } else {
                free(endpointListener);
            }
        }
    }
    // We can release the scope, as celix_properties_set makes a copy of the key & value...
    free(scope);

    return status;
}

celix_status_t bundleActivator_stop(void * userData, celix_bundle_context_t *context) {
    celix_status_t status;
    struct disc_mock_activator * act = userData;

    status = serviceRegistration_unregister(act->reg);

    return status;
}

celix_status_t bundleActivator_destroy(void * userData, celix_bundle_context_t *context) {
    struct disc_mock_activator *act = userData;
    if (act != NULL) {
        discMockService_destroy(act->serv);

        free(act->endpointListener);
        arrayList_destroy(act->endpointList);
        free(act);
    }
    return CELIX_SUCCESS;
}

celix_status_t discovery_endpointAdded(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
    celix_status_t status = CELIX_SUCCESS;
    struct disc_mock_activator *act = handle;

    printf("%s\n", __func__);
    arrayList_add(act->endpointList, endpoint);

    return status;
}

celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_t *endpoint, char *matchedFilter) {
    celix_status_t status  = CELIX_SUCCESS;
    struct disc_mock_activator *act = handle;
    printf("%s\n", __func__);
    arrayList_removeElement(act->endpointList, endpoint);

    return status;
}

