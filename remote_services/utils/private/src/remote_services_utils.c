/*
 * remote_services_utils.c
 *
 *  Created on: Apr 23, 2012
 *      Author: alexander
 */
#include <stdlib.h>
#include <stdio.h>

#include <apr_strings.h>
#include <apr_uuid.h>

#include "remote_constants.h"
#include "remote_services_utils.h"

celix_status_t remoteServicesUtils_getUUID(apr_pool_t *pool, BUNDLE_CONTEXT context, char **uuidStr) {
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

