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
 * bundle_context.c
 *
 *  \date       Mar 26, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "bundle_context_private.h"
#include "framework_private.h"
#include "bundle.h"
#include "celix_log.h"

celix_status_t bundleContext_create(apr_pool_t *pool, framework_pt framework, framework_logger_pt logger, bundle_pt bundle, bundle_context_pt *bundle_context) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_context_pt context = NULL;

	if (*bundle_context != NULL && framework == NULL && bundle == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
        context = malloc(sizeof(*context));
        if (!context) {
            status = CELIX_ENOMEM;
        } else {
            context->pool = pool;
            context->framework = framework;
            context->bundle = bundle;
            context->logger = logger;

            *bundle_context = context;
        }
	}

	framework_logIfError(logger, status, NULL, "Failed to create context");

	return status;
}

celix_status_t bundleContext_destroy(bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	free(context);

	if (context != NULL) {
		context = NULL;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Failed to destroy context");

	return status;
}

celix_status_t bundleContext_getBundle(bundle_context_pt context, bundle_pt *bundle) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*bundle = context->bundle;
	}

	framework_logIfError(logger, status, NULL, "Failed to get bundle");

	return status;
}

celix_status_t bundleContext_getFramework(bundle_context_pt context, framework_pt *framework) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*framework = context->framework;
	}

	framework_logIfError(logger, status, NULL, "Failed to get framework");

	return status;
}

celix_status_t bundleContext_getMemoryPool(bundle_context_pt context, apr_pool_t **memory_pool) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*memory_pool = context->pool;
	}

	framework_logIfError(logger, status, NULL, "Failed to get memory pool");

	return status;
}

celix_status_t bundleContext_installBundle(bundle_context_pt context, char * location, bundle_pt *bundle) {
	return bundleContext_installBundle2(context, location, NULL, bundle);
}

celix_status_t bundleContext_installBundle2(bundle_context_pt context, char * location, char *inputFile, bundle_pt *bundle) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_pt b = NULL;

	if (context != NULL && *bundle == NULL) {
		if (fw_installBundle(context->framework, &b, location, inputFile) != CELIX_SUCCESS) {
            status = CELIX_FRAMEWORK_EXCEPTION;
		} else {
			*bundle = b;
		}
	} else {
        status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Failed to install bundle");

	return status;
}

celix_status_t bundleContext_registerService(bundle_context_pt context, char * serviceName, void * svcObj,
        properties_pt properties, service_registration_pt *service_registration) {
	service_registration_pt registration = NULL;
	celix_status_t status = CELIX_SUCCESS;

	if (context != NULL && *service_registration == NULL) {
	    fw_registerService(context->framework, &registration, context->bundle, serviceName, svcObj, properties);
	    *service_registration = registration;
	} else {
	    status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Failed to register service");

	return status;
}

celix_status_t bundleContext_registerServiceFactory(bundle_context_pt context, char * serviceName, service_factory_pt factory,
        properties_pt properties, service_registration_pt *service_registration) {
    service_registration_pt registration = NULL;
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && *service_registration == NULL) {
        fw_registerServiceFactory(context->framework, &registration, context->bundle, serviceName, factory, properties);
        *service_registration = registration;
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to register service factory");

    return status;
}

celix_status_t bundleContext_getServiceReferences(bundle_context_pt context, const char * serviceName, char * filter, array_list_pt *service_references) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && *service_references == NULL) {
        fw_getServiceReferences(context->framework, service_references, context->bundle, serviceName, filter);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to get service references");

	return status;
}

celix_status_t bundleContext_getServiceReference(bundle_context_pt context, char * serviceName, service_reference_pt *service_reference) {
    service_reference_pt reference = NULL;
    array_list_pt services = NULL;
    celix_status_t status = CELIX_SUCCESS;

    if (serviceName != NULL) {
        if (bundleContext_getServiceReferences(context, serviceName, NULL, &services) == CELIX_SUCCESS) {
            reference = (arrayList_size(services) > 0) ? arrayList_get(services, 0) : NULL;
            arrayList_destroy(services);
            *service_reference = reference;
        } else {
            status = CELIX_ILLEGAL_ARGUMENT;
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to get service reference");

	return status;
}

celix_status_t bundleContext_getService(bundle_context_pt context, service_reference_pt reference, void **service_instance) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL && *service_instance == NULL) {
	    status = fw_getService(context->framework, context->bundle, reference, service_instance);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to get service");

    return status;
}

celix_status_t bundleContext_ungetService(bundle_context_pt context, service_reference_pt reference, bool *result) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL) {
        status = framework_ungetService(context->framework, context->bundle, reference, result);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to unget service");

    return status;
}

celix_status_t bundleContext_getBundles(bundle_context_pt context, array_list_pt *bundles) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL || *bundles != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*bundles = framework_getBundles(context->framework);
	}

	framework_logIfError(logger, status, NULL, "Failed to get bundles");

	return status;
}

celix_status_t bundleContext_getBundleById(bundle_context_pt context, long id, bundle_pt *bundle) {
    celix_status_t status = CELIX_SUCCESS;

    if (context == NULL || *bundle != NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        *bundle = framework_getBundleById(context->framework, id);
    }

    framework_logIfError(logger, status, NULL, "Failed to get bundle [id=%ld]", id);

	return status;
}

celix_status_t bundleContext_addServiceListener(bundle_context_pt context, service_listener_pt listener, char * filter) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_addServiceListener(context->framework, context->bundle, listener, filter);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to add service listener");

    return status;
}

celix_status_t bundleContext_removeServiceListener(bundle_context_pt context, service_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_removeServiceListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to remove service listener");

    return status;
}

celix_status_t bundleContext_addBundleListener(bundle_context_pt context, bundle_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_addBundleListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to add bundle listener");

    return status;
}

celix_status_t bundleContext_removeBundleListener(bundle_context_pt context, bundle_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_removeBundleListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to remove bundle listener");

    return status;
}

celix_status_t bundleContext_addFrameworkListener(bundle_context_pt context, framework_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_addFrameworkListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to add framework listener");

    return status;
}

celix_status_t bundleContext_removeFrameworkListener(bundle_context_pt context, framework_listener_pt listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_removeFrameworkListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to remove framework listener");

    return status;
}

celix_status_t bundleContext_getProperty(bundle_context_pt context, const char *name, char **value) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL || name == NULL || *value != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		fw_getProperty(context->framework, name, value);
	}

	framework_logIfError(logger, status, NULL, "Failed to get property [name=%s]", name);

	return status;
}
