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
 * endpoint_description.c
 *
 *  \date       25 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <assert.h>

#include "celix_errno.h"
#include "celix_log.h"
#include "celix_stdlib_cleanup.h"

#include "endpoint_description.h"
#include "remote_constants.h"
#include "celix_constants.h"
#include "celix_utils.h"

celix_status_t endpointDescription_create(celix_properties_t *properties, endpoint_description_t **endpointDescription) {
    celix_status_t status = CELIX_SUCCESS;
    if (properties == NULL || endpointDescription == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    endpoint_description_t *ep = calloc(1,sizeof(*ep));
    if (ep == NULL) {
        return CELIX_ENOMEM;
    }
    ep->properties = properties;
    ep->frameworkUUID = (char*)celix_properties_get(properties, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
    ep->id = (char*)celix_properties_get(properties, OSGI_RSA_ENDPOINT_ID, NULL);
    ep->serviceName = celix_utils_strdup(celix_properties_get(properties, CELIX_FRAMEWORK_SERVICE_NAME, NULL));
    ep->serviceId = celix_properties_getAsLong(properties, OSGI_RSA_ENDPOINT_SERVICE_ID, -1);

    if (!(ep->frameworkUUID) || !(ep->id) || !(ep->serviceName) || ep->serviceId < 0) {
    	fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "ENDPOINT_DESCRIPTION: incomplete description!.");
    	status = CELIX_BUNDLE_EXCEPTION;
    }

    if (status == CELIX_SUCCESS) {
        *endpointDescription = ep;
    } else {
        *endpointDescription = NULL;
        free(ep->serviceName);
        free(ep);
    }

    return status;
}

celix_status_t endpointDescription_destroy(endpoint_description_t *description) {
    celix_properties_destroy(description->properties);
    free(description->serviceName);
    free(description);
    return CELIX_SUCCESS;
}

bool endpointDescription_isInvalid(const endpoint_description_t *description) {
    return description == NULL || description->properties == NULL || description->serviceId < 0
            || description->serviceName == NULL || strlen(description->serviceName) > NAME_MAX
            || description->frameworkUUID == NULL || description->id == NULL;
}

endpoint_description_t *endpointDescription_clone(const endpoint_description_t *description) {
    if (endpointDescription_isInvalid(description)) {
        return NULL;
    }
    celix_autofree endpoint_description_t *newDesc = (endpoint_description_t*)calloc(1, sizeof(endpoint_description_t));
    if (newDesc == NULL) {
        return NULL;
    }
    celix_autoptr(celix_properties_t) properties = newDesc->properties = celix_properties_copy(description->properties);
    if (newDesc->properties == NULL) {
        return NULL;
    }
    newDesc->frameworkUUID = (char*)celix_properties_get(newDesc->properties,OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
    newDesc->serviceId = description->serviceId;
    newDesc->id = (char*)celix_properties_get(newDesc->properties, OSGI_RSA_ENDPOINT_ID, NULL);
    newDesc->serviceName = celix_utils_strdup(description->serviceName);
    if (newDesc->serviceName == NULL) {
        return NULL;
    }

    celix_steal_ptr(properties);
    return celix_steal_ptr(newDesc);
}
