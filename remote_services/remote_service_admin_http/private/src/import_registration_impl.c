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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include <apr_strings.h>

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

celix_status_t importRegistration_create(apr_pool_t *pool, endpoint_description_pt endpoint, remote_service_admin_pt rsa, 	sendToHandle sendToCallback, bundle_context_pt context, import_registration_pt *registration) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *mypool = NULL;
	apr_pool_create(&mypool, pool);

	*registration = apr_palloc(mypool, sizeof(**registration));
	if (!*registration) {
		status = CELIX_ENOMEM;
	} else {
		(*registration)->pool = mypool;
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


celix_status_t importRegistrationFactory_create(apr_pool_t *pool, char* serviceName, bundle_context_pt context, import_registration_factory_pt *registration_factory) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *mypool = NULL;
	apr_pool_create(&mypool, pool);

	*registration_factory = calloc(1, sizeof(**registration_factory));
	if (!*registration_factory) {
		status = CELIX_ENOMEM;
	} else {
		(*registration_factory)->serviceName = strdup(serviceName);
		(*registration_factory)->pool = mypool;
		(*registration_factory)->context = context;
		(*registration_factory)->bundle = NULL;
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

		free(*registration_factory);
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

	char *name = apr_pstrcat(registration_factory->pool, bundleStore, "/", registration_factory->serviceName, "_proxy.zip", NULL);
	status = bundleContext_installBundle(registration_factory->context, name, &registration_factory->bundle);

	if (status == CELIX_SUCCESS) {
		status = bundle_start(registration_factory->bundle);
		if (status == CELIX_SUCCESS) {
			printf("%s sucessfully started\n", name);
		}
	}
	else
	{

		printf("%s could not be installed.\n", name);
	}

	return status;
}

celix_status_t importRegistrationFactory_close(import_registration_factory_pt registration_factory)
{
	celix_status_t status = CELIX_SUCCESS;

	if (registration_factory->bundle != NULL) {
		bundle_stop(registration_factory->bundle);
		bundle_uninstall(registration_factory->bundle);
	}

	return status;
}


celix_status_t importRegistration_createProxyFactoryTracker(import_registration_factory_pt registration_factory, service_tracker_pt *tracker) {
	celix_status_t status = CELIX_SUCCESS;
	service_tracker_customizer_pt customizer = NULL;

	status = serviceTrackerCustomizer_create(registration_factory, importRegistration_proxyFactoryAdding, importRegistration_proxyFactoryAdded, importRegistration_proxyFactoryModified, importRegistration_proxyFactoryRemoved, &customizer);

	if (status == CELIX_SUCCESS) {
		char *filter = apr_pstrcat(registration_factory->pool, "(&(", OSGI_FRAMEWORK_OBJECTCLASS, "=", OSGI_RSA_REMOTE_PROXY_FACTORY,")(proxy.interface=", registration_factory->serviceName, "))", NULL);
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
	import_registration_factory_pt registration_factory = (import_registration_factory_pt) handle;

	return status;
}

celix_status_t importRegistration_proxyFactoryRemoved(void * handle, service_reference_pt reference, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	import_registration_pt registration = handle;

	import_registration_factory_pt registration_factory = (import_registration_factory_pt) handle;
	registration_factory->trackedFactory = NULL;


	return status;
}



celix_status_t importRegistrationFactory_install(apr_pool_t *pool, char* serviceName, bundle_context_pt context, import_registration_factory_pt *registration_factory)
{
	celix_status_t status = CELIX_SUCCESS;

	if ( (status = importRegistrationFactory_create(pool, serviceName, context, registration_factory)) != CELIX_SUCCESS)
	{
		printf("remoteServiceAdmin_importService: error while creating importRegistrationFactory\n");
	}
	else if ((status = importRegistrationFactory_open(*registration_factory)) != CELIX_SUCCESS)
	{
		printf("remoteServiceAdmin_importService: cannot open registration_factory for %s \n  ", serviceName);
		importRegistrationFactory_destroy(registration_factory);
	}
	else
	{
		importRegistration_createProxyFactoryTracker(*registration_factory, &((*registration_factory)->proxyFactoryTracker));
		printf("remoteServiceAdmin_importService: new registration_factory added for %s at %p\n  ", serviceName, (*registration_factory)->proxyFactoryTracker);
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
		registration->importReference = apr_palloc(registration->pool, sizeof(*registration->importReference));
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
