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
 * refining_driver.c
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <device.h>
#include <bundle_context.h>
#include <service_event.h>

#include "refining_driver_private.h"
#include "base_driver_device.h"

static const int MAX_BUFF_SIZE = 1024;

struct refining_driver {
	device_pt device;
	bundle_context_pt context;
	int deviceCount;
	array_list_pt devices;
};

struct device {
	/*NOTE: for this example we use a empty device structure*/
};

struct refining_driver_device {
	device_pt device;
	base_driver_device_service_pt baseDriverDeviceService;
	refining_driver_pt driver;
	service_reference_pt baseServiceReference;
	service_registration_pt deviceRegistration;
	celix_service_listener_t *listener;
};

celix_status_t refiningDriver_destroy(refining_driver_pt driver) {
	if (driver != NULL) {
		if (driver->devices != NULL) {
			arrayList_destroy(driver->devices);
			driver->devices=NULL;
		}
	}
	return CELIX_SUCCESS;
}

celix_status_t refiningDriver_create(bundle_context_pt context, refining_driver_pt *driver) {
	celix_status_t status = CELIX_SUCCESS;
	(*driver) = calloc(1, sizeof(**driver));
	if ((*driver) != NULL) {
		(*driver)->context=context;
		(*driver)->deviceCount=0;
		(*driver)->device = calloc(1, sizeof(*(*driver)->device));
		(*driver)->devices=NULL;
		status = arrayList_create(&(*driver)->devices);
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t refiningDriver_createService(refining_driver_pt driver, driver_service_pt *service) {
	celix_status_t status = CELIX_SUCCESS;
	(*service) = calloc(1, sizeof(**service));
	if ((*service) != NULL) {
		(*service)->driver = driver;
		(*service)->attach = refiningDriver_attach;
		(*service)->match = refiningDriver_match;
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

static celix_status_t refiningDriver_stopDevice(refining_driver_device_pt device) {
	printf("REFINING_DRIVER: stopping device, parent device is unregistered\n");
	celix_status_t status =  CELIX_SUCCESS;

	if (device->deviceRegistration != NULL) {
		status = serviceRegistration_unregister(device->deviceRegistration);
		if (status == CELIX_SUCCESS) {
			printf("unregistered refining device\n");
		}
	}

	arrayList_removeElement(device->driver->devices, device);
	return status;
}


static celix_status_t refiningDriver_serviceChanged(void *handle, celix_service_event_t *event) {
	celix_status_t status =  CELIX_SUCCESS;
	refining_driver_device_pt device = handle;
	if (event->type == OSGI_FRAMEWORK_SERVICE_EVENT_UNREGISTERING) {
		bool equal = false;
		status = serviceReference_equals(device->baseServiceReference, event->reference, &equal);
		if (status == CELIX_SUCCESS && equal) {
			refiningDriver_stopDevice(device);
		}
	}
	return status;
}

celix_status_t refiningDriver_destroyDevice(refining_driver_device_pt device) {
	return CELIX_SUCCESS;
}

celix_status_t refining_driver_cleanup_device(refining_driver_device_pt handler) {
	celix_status_t status = CELIX_SUCCESS;;
	refining_driver_device_pt device = handler;
	if (device != NULL) {
		if (device->listener != NULL) {
			bundleContext_removeServiceListener(device->driver->context, device->listener);
		}
	}
	return status;
}

celix_status_t refiningDriver_createDevice(refining_driver_pt driver, service_reference_pt reference, base_driver_device_service_pt baseService, refining_driver_device_pt *device) {
	celix_status_t status = CELIX_SUCCESS;

	(*device) = calloc(1, sizeof(**device));
	if ((*device) != NULL) {
		(*device)->driver=driver;
		(*device)->baseDriverDeviceService=baseService;
		(*device)->baseServiceReference=reference;
		(*device)->deviceRegistration=NULL;
		(*device)->listener=NULL;

		celix_service_listener_t *listener = calloc(1, sizeof(*listener));
		listener->handle=(void *)(*device);
		listener->serviceChanged=(celix_status_t (*)(void * listener, celix_service_event_t *event))refiningDriver_serviceChanged;
		bundleContext_addServiceListener(driver->context, listener, NULL);
		(*device)->listener=listener;

		arrayList_add(driver->devices, (*device));
	} else {
		status = CELIX_ENOMEM;
	}

	return status;
}


static celix_status_t refiningDriver_registerDevice(refining_driver_pt driver, refining_driver_device_pt device, char *serial) {
	celix_status_t status = CELIX_SUCCESS;
	refining_driver_device_service_pt service = NULL;
	status = refiningDriverDevice_createService(device, &service);
	properties_pt props = properties_create();

	if (status == CELIX_SUCCESS) {
		properties_set(props, OSGI_DEVICEACCESS_DEVICE_CATEGORY, REFINING_DRIVER_DEVICE_CATEGORY);
		properties_set(props, OSGI_DEVICEACCESS_DEVICE_SERIAL, serial);
		status = bundleContext_registerService(driver->context, OSGI_DEVICEACCESS_DEVICE_SERVICE_NAME, service, props, &device->deviceRegistration);
	}

	if (status == CELIX_SUCCESS) {
		printf("REFINING_DRIVER: registered refining device with serial %s\n", serial);
	}
	else{
		properties_destroy(props);
		refiningDriverDevice_destroyService(service);
	}
	return status;
}

celix_status_t refiningDriver_attach(void * driverHandler, service_reference_pt reference, char **result) {
    printf("REFINING_DRIVER: attached called\n");
    celix_status_t status = CELIX_SUCCESS;
    refining_driver_pt driver = driverHandler;
    (*result) = NULL;
    base_driver_device_service_pt device_service = NULL;
    status = bundleContext_getService(driver->context, reference, (void **)&device_service);
    if (status == CELIX_SUCCESS) {
        refining_driver_device_pt refiningDevice = NULL;
        status = refiningDriver_createDevice(driver, reference, device_service, &refiningDevice);
        if (status == CELIX_SUCCESS) {
            driver->deviceCount+=1;
            char *serial;
            asprintf(&serial, "%4i", driver->deviceCount);
            status = refiningDriver_registerDevice(driver, refiningDevice, serial);
            free(serial);
        }
    }
    return status;
}

celix_status_t refiningDriver_match(void *driverHandler, service_reference_pt reference, int *value) {
    printf("REFINING_DRIVER: match called\n");
    int match = 0;
    celix_status_t status = CELIX_SUCCESS;

    const char* category = NULL;
    status = serviceReference_getProperty(reference, OSGI_DEVICEACCESS_DEVICE_CATEGORY, &category);
    if (status == CELIX_SUCCESS) {
        if (strcmp(category, BASE_DRIVER_DEVICE_CATEGORY) == 0) {
            match = 10;
        }
    }

    (*value) = match;
    return status;
}

celix_status_t refiningDriverDevice_createService(refining_driver_device_pt device, refining_driver_device_service_pt *service) {
	celix_status_t status = CELIX_SUCCESS;
	(*service) = calloc(1, sizeof(**service));
	if ((*service) != NULL) {
		(*service)->deviceService.device=calloc(1, sizeof(*(*service)->deviceService.device));
		if ((*service)->deviceService.device != NULL) {
			(*service)->deviceService.noDriverFound=refiningDriverDevice_noDriverFound;
			(*service)->refiningDriverDevice=device;
			(*service)->getNextWord=refiningDriverDevice_getNextWord;
		} else {
			status = CELIX_ENOMEM;
		}
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t refiningDriverDevice_destroyService(refining_driver_device_service_pt service){
	if(service != NULL){
		if(service->deviceService.device != NULL){
			free(service->deviceService.device);
		}
		free(service);
	}
	return CELIX_SUCCESS;
}

celix_status_t refiningDriverDevice_getNextWord(refining_driver_device_pt refiningDriverDevice, char **word) {
	celix_status_t status = CELIX_SUCCESS;
	base_driver_device_pt baseDevice = refiningDriverDevice->baseDriverDeviceService->baseDriverDevice;
	char buff[MAX_BUFF_SIZE];
	int i=0;
	status = refiningDriverDevice->baseDriverDeviceService->getNextChar(baseDevice, &buff[i]);
	while (buff[i] != ' ' && i < (MAX_BUFF_SIZE-1) && status == CELIX_SUCCESS) {
		i+=1;
		status = refiningDriverDevice->baseDriverDeviceService->getNextChar(baseDevice, &buff[i]);
	}
	if (status == CELIX_SUCCESS) {
		buff[i] = '\0';
		char *copy = calloc(1, (i+1));
		if (copy != NULL) {
			strcpy(copy, buff);
			(*word)=copy;
		} else {
			status = CELIX_ENOMEM;
		}
	}

	return status;
}

celix_status_t refiningDriverDevice_noDriverFound(device_pt device) {
	printf("REFINING_DRIVER: no driver found");
	return CELIX_SUCCESS;
}

