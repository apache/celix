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
 * endpoint_description.c
 *
 *  \date       25 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include "celix_errno.h"
#include "celix_log.h"

#include "endpoint_description.h"
#include "remote_constants.h"
#include "constants.h"

static celix_status_t endpointDescription_verifyLongProperty(properties_pt properties, char *propertyName, long *longProperty);

celix_status_t endpointDescription_create(properties_pt properties, endpoint_description_pt *endpointDescription) {
	celix_status_t status = CELIX_SUCCESS;

    *endpointDescription = malloc(sizeof(**endpointDescription));

    long serviceId = 0L;
    status = endpointDescription_verifyLongProperty(properties, (char *) OSGI_RSA_ENDPOINT_SERVICE_ID, &serviceId);
    if (status != CELIX_SUCCESS) {
    	return status;
    }

    (*endpointDescription)->properties = properties;
    (*endpointDescription)->frameworkUUID = properties_get(properties, (char *) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID);
    (*endpointDescription)->id = properties_get(properties, (char *) OSGI_RSA_ENDPOINT_ID);
    (*endpointDescription)->service = properties_get(properties, (char *) OSGI_FRAMEWORK_OBJECTCLASS);
    (*endpointDescription)->serviceId = serviceId;

    if (!(*endpointDescription)->frameworkUUID || !(*endpointDescription)->id || !(*endpointDescription)->service) {
    	fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "ENDPOINT_DESCRIPTION: incomplete description!.");
    	status = CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

celix_status_t endpointDescription_destroy(endpoint_description_pt description) {
    properties_destroy(description->properties);
    free(description);
    return CELIX_SUCCESS;
}

static celix_status_t endpointDescription_verifyLongProperty(properties_pt properties, char *propertyName, long *longProperty) {
    celix_status_t status = CELIX_SUCCESS;

    char *value = properties_get(properties, propertyName);
    if (value == NULL) {
        *longProperty = 0l;
    } else {
        *longProperty = atol(value);
    }

    return status;
}
