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
 * remote_services_utils.c
 *
 *  \date       Apr 23, 2012
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>

#include <apr_strings.h>
#include <apr_uuid.h>

#include "remote_constants.h"
#include "remote_services_utils.h"

celix_status_t remoteServicesUtils_getUUID(apr_pool_t *pool, bundle_context_pt context, char **uuidStr) {
	celix_status_t status = CELIX_SUCCESS;

	status = bundleContext_getProperty(context, ENDPOINT_FRAMEWORK_UUID, uuidStr);
	if (status == CELIX_SUCCESS) {
		if (*uuidStr == NULL) {
			*uuidStr = apr_palloc(pool, APR_UUID_FORMATTED_LENGTH + 1);
			if (!*uuidStr) {
				status = CELIX_ENOMEM;
			} else {
				apr_uuid_t uuid;
				apr_uuid_get(&uuid);
				apr_uuid_format(*uuidStr, &uuid);
				setenv(ENDPOINT_FRAMEWORK_UUID, *uuidStr, 1);
			}
		}
	}

	return status;
}

