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
 * driver_locator.c
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>

#include "driver_locator_private.h"
#include "device.h"

celix_status_t driverLocator_findDrivers(driver_locator_pt locator, properties_pt props, array_list_pt *drivers) {
	celix_status_t status = CELIX_SUCCESS;

	const char* category = properties_get(props, OSGI_DEVICEACCESS_DEVICE_CATEGORY);

	status = arrayList_create(drivers);
	if (status == CELIX_SUCCESS) {
		DIR *dir;
		dir = opendir(locator->path);
		if (!dir) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			struct dirent *dp;
			while ((dp = readdir(dir)) != NULL) {
				char str1[256], str2[256], str3[256];
				if (sscanf(dp->d_name, "%[^_]_%[^.].%s", str1, str2, str3) == 3 &&
					strcmp(str1, category) == 0 &&
					strcmp(str3, "zip") == 0) {
					int length = strlen(str1) + strlen(str2) + 2;
					char driver[length];
					snprintf(driver, length, "%s_%s", str1, str2);
                    arrayList_add(*drivers, strdup(driver));
				}
			}
			closedir(dir);
		}
	}
	return status;
}

celix_status_t driverLocator_loadDriver(driver_locator_pt locator, char *id, char **stream) {
	celix_status_t status = CELIX_SUCCESS;
	*stream = NULL;

	DIR *dir;
	dir = opendir(locator->path);
	if (!dir) {
		status = CELIX_FILE_IO_EXCEPTION;
	} else {
		struct dirent *dp;
		while ((dp = readdir(dir)) != NULL) {
			char str1[256], str2[256];
			if (sscanf(dp->d_name, "%[^.].%s", str1, str2) == 2 &&
				strcmp(str1, id) == 0 &&
				strcmp(str2, "zip") == 0) {
				int length = strlen(locator->path) + strlen(dp->d_name) + 2;
				char stream_str[length];
				snprintf(stream_str, length, "%s/%s", locator->path, dp->d_name);
				*stream = strdup(stream_str);
				break;
			}
		}
		closedir(dir);
	}

	return status;
}

