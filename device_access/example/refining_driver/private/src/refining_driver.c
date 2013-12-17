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
 * refining_driver.c
 *
 *  \date       Jun 20, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <apr_strings.h>
#include <apr_pools.h>
#include <apr_thread_proc.h>
#include <apr_thread_cond.h>
#include <apr_thread_mutex.h>

#include <device.h>
#include <bundle_context.h>
#include <service_event.h>

#include "refining_driver_private.h"
#include "base_driver_device.h"

static const int MAX_BUFF_SIZE = 1024;

struct refining_driver {
	device_pt device;
	apr_pool_t *pool;
	bundle_context_pt context;
	int deviceCount;
	array_list_pt devices;
};

struct device {
	/*NOTE: for this example we use a empty device structure*/
};

struct refining_driver_device {
	device_pt device;
	apr_pool_t *pool;
	base_driver_device_service_pt baseDriverDeviceService;
	refining_driver_pt driver;
	service_reference_pt baseServiceReference;
	service_registration_pt deviceRegistration;
	service_listener_pt listener;
};

celix_status_t refiningDriver_destroy(refining_driver_pt driver) {
	apr_pool_destroy(driver->pool);
	return CELIX_SUCCESS;
}

static apr_status_t refiningDriver_cleanup(void *handler) {
	refining_driver_pt driver = handler;
	if (driver != NULL) {
		if (driver->devices != NULL) {
			arrayList_destroy(driver->devices);
			driver->devices=NULL;
		}
	}
	return CELIX_SUCCESS;
}

celix_status_t refiningDriver_create(bundle_context_pt context, apr_pool_t *pool, refining_driver_pt *driver) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *driverPool = NULL;
	apr_status_t aprStatus = apr_pool_create(&driverPool, pool);
	(*driver) = apr_palloc(driverPool, sizeof(**driver));
	if ((*driver) != NULL) {
		apr_pool_pre_cleanup_register(driverPool, (*driver), refiningDriver_cleanup);
		(*driver)->pool=driverPool;
		(*driver)->context=context;
		(*driver)->deviceCount=0;
		(*driver)->device = apr_palloc(driverPool, sizeof(*(*driver)->device));
		(*driver)->devices=NULL;
		status = arrayList_create(&(*driver)->devices);
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}

celix_status_t refiningDriver_createService(refining_driver_pt driver, driver_service_pt *service) {
	celix_status_t status = CELIX_SUCCESS;
	(*service) = apr_palloc(driver->pool, sizeof(**service));
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
	apr_pool_destroy(device->pool);
	return status;
}


static celix_status_t refiningDriver_serviceChanged(service_listener_pt listener, service_event_pt event) {
	celix_status_t status =  CELIX_SUCCESS;
	refining_driver_device_pt device = listener->handle;
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
	apr_pool_destroy(device->pool);
	return CELIX_SUCCESS;
}

static apr_status_t refining_driver_cleanup_device(void *handler) {
	apr_status_t status = APR_SUCCESS;
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
	apr_pool_t *devicePool = NULL;
	apr_status_t aprStatus = apr_pool_create(&devicePool, driver->pool);

	if (aprStatus == APR_SUCCESS) {
		(*device) = apr_palloc(devicePool, sizeof(**device));
			if ((*device) != NULL) {
				apr_pool_pre_cleanup_register(devicePool, (*device), refining_driver_cleanup_device);
				(*device)->driver=driver;
				(*device)->pool=devicePool;
				(*device)->baseDriverDeviceService=baseService;
				(*device)->baseServiceReference=reference;
				(*device)->deviceRegistration=NULL;
				(*device)->listener=NULL;

				service_listener_pt listener = apr_palloc(devicePool, sizeof(*listener));
				listener->pool=devicePool;
				listener->handle=(void *)(*device);
				listener->serviceChanged=(celix_status_t (*)(void * listener, service_event_pt event))refiningDriver_serviceChanged;
				bundleContext_addServiceListener(driver->context, listener, NULL);
				(*device)->listener=listener;

				arrayList_add(driver->devices, (*device));
			} else {
				status = CELIX_ENOMEM;
			}
	} else {
		status = CELIX_ENOMEM;
	}
	return status;
}


static celix_status_t refiningDriver_registerDevice(refining_driver_pt driver, refining_driver_device_pt device, char *serial) {
	celix_status_t status = CELIX_SUCCESS;
	refining_driver_device_service_pt service = NULL;
	status = refiningDriverDevice_createService(device, &service);
	if (status == CELIX_SUCCESS) {
		properties_pt props = properties_create();
		properties_set(props, OSGI_DEVICEACCESS_DEVICE_CATEGORY, REFINING_DRIVER_DEVICE_CATEGORY);
		properties_set(props, OSGI_DEVICEACCESS_DEVICE_SERIAL, serial);
		status = bundleContext_registerService(driver->context, OSGI_DEVICEACCESS_DEVICE_SERVICE_NAME, service, props, &device->deviceRegistration);
	}

	if (status == CELIX_SUCCESS) {
		printf("REFINING_DRIVER: registered refining device with serial %s\n", serial);
	}
	return status;
}

celix_status_t refiningDriver_attach(void * driverHandler, service_reference_pt reference, char **result) {
	printf("REFINING_DRIVER: attached called\n");
	celix_status_t status = CELIX_SUCCESS;
	refining_driver_pt driver = driverHandler;
	(*result) = NULL;
	device_service_pt device = NULL;
	base_driver_device_service_pt device_service = NULL;
	status = bundleContext_getService(driver->context, reference, (void **)&device_service);
	if (status == CELIX_SUCCESS) {
		refining_driver_device_pt refiningDevice = NULL;
		status = refiningDriver_createDevice(driver, reference, device_service, &refiningDevice);
		if (status == CELIX_SUCCESS) {
			driver->deviceCount+=1;
			char serial[5];
			sprintf(serial, "%4i", driver->deviceCount);
			status = refiningDriver_registerDevice(driver, refiningDevice, serial);
		}
	}
	return status;
}

celix_status_t refiningDriver_match(void *driverHandler, service_reference_pt reference, int *value) {
	printf("REFINING_DRIVER: match called\n");
	int match = 0;
	celix_status_t status = CELIX_SUCCESS;
	refining_driver_pt driver = driverHandler;

	service_registration_pt registration = NULL;
	properties_pt properties = NULL;
	status = serviceReference_getServiceRegistration(reference, &registration);
	if (status == CELIX_SUCCESS) {
		status = serviceRegistration_getProperties(registration, &properties);
		if (status == CELIX_SUCCESS) {
			char *category = properties_get(properties, OSGI_DEVICEACCESS_DEVICE_CATEGORY);
			if (strcmp(category, BASE_DRIVER_DEVICE_CATEGORY) == 0) {
				match = 10;
			}
		}
	}

	(*value) = match;
	return status;
}

celix_status_t refiningDriverDevice_createService(refining_driver_device_pt device, refining_driver_device_service_pt *service) {
	celix_status_t status = CELIX_SUCCESS;
	(*service) = apr_palloc(device->pool, sizeof(**service));
	if ((*service) != NULL) {
		(*service)->deviceService.device=apr_palloc(device->pool, sizeof(*(*service)->deviceService.device));
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

celix_status_t refiningDriverDevice_getNextWord(refining_driver_device_pt refiningDriverDevice, apr_pool_t *pool, char **word) {
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
		char *copy = apr_palloc(pool, (i+1));
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

