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
 * import_registration_impl.c
 *
 *  \date       Oct 14, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include <constants.h>

#include "celix_errno.h"

#include "import_registration_impl.h"
#include "remote_service_admin_impl.h"
#include "remote_proxy.h"
#include "service_tracker.h"
#include "bundle_context.h"
#include "bundle.h"


celix_status_t importRegistration_proxyFactoryAdding(void * handle, service_reference_pt reference, void **service);
celix_status_t importRegistration_proxyFactoryAdded(void * handle, service_reference_pt reference, void *service);
celix_status_t importRegistration_proxyFactoryModified(void * handle, service_reference_pt reference, void *service);
celix_status_t importRegistration_proxyFactoryRemoved(void * handle, service_reference_pt reference, void *service);

celix_status_t importRegistration_create(endpoint_description_pt endpoint, remote_service_admin_pt rsa, 	sendToHandle sendToCallback, bundle_context_pt context, import_registration_pt *registration) {
	celix_status_t status = CELIX_SUCCESS;

	*registration = calloc(1, sizeof(**registration));
	if (!*registration) {
		status = CELIX_ENOMEM;
	} else {
		(*registration)->context = context;
		(*registration)->closed = false;
		(*registration)->endpointDescription = endpoint;
		(*registration)->rsa = rsa;
		(*registration)->sendToCallback = sendToCallback;
		(*registration)->reference = NULL;
		(*registration)->importReference = NULL;
	}

	return status;
}

celix_status_t importRegistration_destroy(import_registration_pt registration)
{
	return CELIX_SUCCESS;
}


celix_status_t importRegistrationFactory_create(log_helper_pt helper, char* serviceName, bundle_context_pt context, import_registration_factory_pt *registration_factory) {
	celix_status_t status = CELIX_SUCCESS;

	*registration_factory = calloc(1, sizeof(**registration_factory));
	if (!*registration_factory) {
		status = CELIX_ENOMEM;
	} else {
		(*registration_factory)->serviceName = strdup(serviceName);
		(*registration_factory)->context = context;
		(*registration_factory)->bundle = NULL;
		(*registration_factory)->loghelper = helper;

		arrayList_create(&(*registration_factory)->registrations);
	}

	return status;
}



celix_status_t importRegistrationFactory_destroy(import_registration_factory_pt* registration_factory) {
	celix_status_t status = CELIX_SUCCESS;

	if (*registration_factory != NULL)
	{
		free((*registration_factory)->serviceName);
		arrayList_destroy((*registration_factory)->registrations);

		serviceTracker_destroy((*registration_factory)->proxyFactoryTracker);
		free(*registration_factory);

		*registration_factory = NULL;
	}


	return status;
}


celix_status_t importRegistrationFactory_open(import_registration_factory_pt registration_factory)
{
	celix_status_t status = CELIX_SUCCESS;

	char *bundleStore = NULL;
	bundleContext_getProperty(registration_factory->context, BUNDLE_STORE_PROPERTY_NAME, &bundleStore);

	if (bundleStore == NULL) {
		bundleStore = DEFAULT_BUNDLE_STORE;
	}

	char name[256];
	snprintf(name, 256, "%s/%s_proxy.zip", bundleStore, registration_factory->serviceName);

	status = bundleContext_installBundle(registration_factory->context, name, &registration_factory->bundle);

	if (status == CELIX_SUCCESS) {
		status = bundle_start(registration_factory->bundle);
		if (status == CELIX_SUCCESS) {
			logHelper_log(registration_factory->loghelper, OSGI_LOGSERVICE_INFO, "%s successfully started.", name);
		}
	}
	else {
		logHelper_log(registration_factory->loghelper, OSGI_LOGSERVICE_ERROR, "%s could not be installed.", name);
	}

	return status;
}

celix_status_t importRegistrationFactory_close(import_registration_factory_pt registration_factory)
{
	celix_status_t status = CELIX_SUCCESS;


	if (registration_factory->proxyFactoryTracker != NULL) {
		serviceTracker_close(registration_factory->proxyFactoryTracker);
	}

	if (registration_factory->bundle != NULL) {
		bundle_uninstall(registration_factory->bundle);
	}

	return status;
}


celix_status_t importRegistration_createProxyFactoryTracker(import_registration_factory_pt registration_factory, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;
	service_tracker_customizer_pt customizer = NULL;

	status = serviceTrackerCustomizer_create(registration_factory, importRegistration_proxyFactoryAdding, importRegistration_proxyFactoryAdded, importRegistration_proxyFactoryModified, importRegistration_proxyFactoryRemoved, &customizer);

	if (status == CELIX_SUCCESS) {
		char filter[512];

		snprintf(filter, 512, "(&(%s=%s)(proxy.interface=%s))", (char*) OSGI_FRAMEWORK_OBJECTCLASS, (char*) OSGI_RSA_REMOTE_PROXY_FACTORY, registration_factory->serviceName);
		status = serviceTracker_createWithFilter(registration_factory->context, filter, customizer, tracker);

		if (status == CELIX_SUCCESS)
		{
			serviceTracker_open(*tracker);
		}
	}

	return status;
}

celix_status_t importRegistration_proxyFactoryAdding(void * handle, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	import_registration_factory_pt registration_factory = (import_registration_factory_pt) handle;

	bundleContext_getService(registration_factory->context, reference, service);

	return status;
}

celix_status_t importRegistration_proxyFactoryAdded(void * handle, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;

	import_registration_factory_pt registration_factory = (import_registration_factory_pt) handle;
	registration_factory->trackedFactory = (remote_proxy_factory_service_pt) service;

	return status;
}

celix_status_t importRegistration_proxyFactoryModified(void * handle, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;

	return status;
}

celix_status_t importRegistration_proxyFactoryRemoved(void * handle, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;

	import_registration_factory_pt registration_factory = (import_registration_factory_pt) handle;
	registration_factory->trackedFactory = NULL;


	return status;
}



celix_status_t importRegistrationFactory_install(log_helper_pt helper, char* serviceName, bundle_context_pt context, import_registration_factory_pt *registration_factory)
{
	celix_status_t status = CELIX_BUNDLE_EXCEPTION;

	if ( (status = importRegistrationFactory_create(helper, serviceName, context, registration_factory)) == CELIX_SUCCESS) {
		// starting the proxy tracker first allows us to pick up already available proxy factories
		importRegistration_createProxyFactoryTracker(*registration_factory, &((*registration_factory)->proxyFactoryTracker));
		logHelper_log((*registration_factory)->loghelper, OSGI_LOGSERVICE_INFO, "remoteServiceAdmin_importService: new registration_factory added for %s at %p", serviceName, (*registration_factory)->proxyFactoryTracker);

		// check whether factory is available
		if (((*registration_factory)->trackedFactory == NULL) && ((status = importRegistrationFactory_open(*registration_factory)) != CELIX_SUCCESS)) {
			logHelper_log((*registration_factory)->loghelper, OSGI_LOGSERVICE_ERROR, "remoteServiceAdmin_importService: cannot open registration_factory for %s.", serviceName);

			importRegistrationFactory_close(*registration_factory);
			importRegistrationFactory_destroy(registration_factory);
		}
	}

	return status;
}




celix_status_t importRegistration_getException(import_registration_pt registration) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}


celix_status_t importRegistration_getImportReference(import_registration_pt registration, import_reference_pt *reference) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration->importReference == NULL) {
		registration->importReference = calloc(1, sizeof(*registration->importReference));
		if (registration->importReference == NULL) {
			status = CELIX_ENOMEM;
		} else {
			registration->importReference->endpoint = registration->endpointDescription;
			registration->importReference->reference = registration->reference;
		}
	}

	*reference = registration->importReference;

	return status;
}
