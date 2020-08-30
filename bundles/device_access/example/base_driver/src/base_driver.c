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
 * base_driver.c
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>
#include <celix_errno.h>
#include <bundle_activator.h>
#include <bundle_context.h>
#include <celixbool.h>
#include <device.h>

#include "base_driver_device.h"
#include "base_driver_private.h"

static const char * INPUT_STRING = "Lorem ipsum dolor sit amet consectetur adipiscing elit ";

struct device {
	/*NOTE: for this example we use a empty device structure*/
};

struct base_driver_device {
	device_pt device;
	char *input;
	int inputLength;
	int currentChar;
};

celix_status_t baseDriver_noDriverFound(device_pt device) {
	printf("BASE_DRIVER: No driver found\n");
	return CELIX_SUCCESS;
}

celix_status_t baseDriver_create(base_driver_device_pt *baseDriverDevice) {
	celix_status_t status = CELIX_SUCCESS;
	(*baseDriverDevice) = calloc(1, sizeof(struct base_driver_device));
	if (*baseDriverDevice != NULL) {
		(*baseDriverDevice)->currentChar = 0;
		(*baseDriverDevice)->input = (char *)INPUT_STRING;
		(*baseDriverDevice)->inputLength=strlen(INPUT_STRING);
		(*baseDriverDevice)->device = calloc(1, sizeof(*(*baseDriverDevice)->device));
		if ((*baseDriverDevice)->device == NULL) {
			status = CELIX_ENOMEM;
		}
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t baseDriver_createService(base_driver_device_pt baseDriverDevice, base_driver_device_service_pt *service) {
	celix_status_t status = CELIX_SUCCESS;
	(*service) = calloc(1, sizeof(struct base_driver_device_service));
	if ((*service) != NULL) {
		(*service)->deviceService.noDriverFound = baseDriver_noDriverFound;
		(*service)->deviceService.device = baseDriverDevice->device;
		(*service)->getNextChar=baseDriver_getNextChar;
		(*service)->baseDriverDevice = baseDriverDevice;
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t baseDriver_getNextChar(base_driver_device_pt device, char *c) {
	(*c) = device->input[device->currentChar];
	device->currentChar+=1;
	if (device->currentChar >= device->inputLength) {
		device->currentChar = 0;
	}
	return CELIX_SUCCESS;
}

celix_status_t baseDriver_destroy(base_driver_device_pt device){
	if(device != NULL){
		if(device->device != NULL){
			free(device->device);
		}
		free(device);
	}
	return CELIX_SUCCESS;
}

celix_status_t baseDriver_destroyService(base_driver_device_service_pt service){
	if(service != NULL){
		free(service);
	}

	return CELIX_SUCCESS;
}
