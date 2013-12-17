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
 * driver_locator.h
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <apr_general.h>
#include <apr_strings.h>
#include <apr_file_info.h>

#include "driver_locator_private.h"
#include "device.h"

celix_status_t driverLocator_findDrivers(driver_locator_pt locator, apr_pool_t *pool, properties_pt props, array_list_pt *drivers) {
	celix_status_t status = CELIX_SUCCESS;

	char *category = properties_get(props, OSGI_DEVICEACCESS_DEVICE_CATEGORY);

	status = arrayList_create(drivers);
	if (status == CELIX_SUCCESS) {
		apr_pool_t *spool;
		apr_status_t aprStatus = apr_pool_create(&spool, pool);
		if (aprStatus != APR_SUCCESS) {
			status = CELIX_ENOMEM;
		} else {
			apr_dir_t *dir;
			aprStatus = apr_dir_open(&dir, locator->path, spool);
			if (aprStatus != APR_SUCCESS) {
				status = CELIX_FILE_IO_EXCEPTION;
			} else {
				apr_finfo_t dd;
				while ((apr_dir_read(&dd, APR_FINFO_TYPE, dir)) == APR_SUCCESS) {
					char str1[256], str2[256], str3[256];
					if (sscanf(dd.name, "%[^_]_%[^.].%s", str1, str2, str3) == 3 &&
						strcmp(str1, category) == 0 &&
						strcmp(str3, "zip") == 0) {
						char *driver = apr_psprintf(pool, "%s_%s", str1, str2);
						if (driver != NULL) {
							arrayList_add(*drivers, driver);
						}
					}
				}
				apr_dir_close(dir);
			}
			apr_pool_destroy(spool);
		}
	}
	return status;
}

celix_status_t driverLocator_loadDriver(driver_locator_pt locator, apr_pool_t *pool, char *id, char **stream) {
	celix_status_t status = CELIX_SUCCESS;
	*stream = NULL;

	apr_pool_t *spool;
	apr_status_t aprStatus = apr_pool_create(&spool, pool);
	if (aprStatus != APR_SUCCESS) {
		status = CELIX_ENOMEM;
	} else {
		apr_dir_t *dir;
		aprStatus = apr_dir_open(&dir, locator->path, spool);
		if (aprStatus != APR_SUCCESS) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			apr_finfo_t dd;
			while ((apr_dir_read(&dd, APR_FINFO_TYPE, dir)) == APR_SUCCESS) {
				char str1[256], str2[256];
				if (sscanf(dd.name, "%[^.].%s", str1, str2) == 2 &&
					strcmp(str1, id) == 0 &&
					strcmp(str2, "zip") == 0) {
					*stream = apr_psprintf(pool, "%s/%s", locator->path, dd.name);
					break;
				}
			}
			apr_dir_close(dir);
		}
		apr_pool_destroy(spool);
	}

	return status;
}

