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

#include "bundle_context.h"
#include "framework.h"
#include "bundle.h"

struct bundleContext {
	struct framework * framework;
	struct bundle * bundle;
	apr_pool_t *pool;
};

celix_status_t bundleContext_create(FRAMEWORK framework, BUNDLE bundle, BUNDLE_CONTEXT *bundle_context) {
	celix_status_t status = CELIX_SUCCESS;
	BUNDLE_CONTEXT context = NULL;

	if (*bundle_context == NULL) {
	    context = malloc(sizeof(*context));

		if (context != NULL) {
			apr_pool_t *pool = NULL;

            context->pool = NULL;
			context->framework = framework;
			context->bundle = bundle;

			status = bundle_getMemoryPool(bundle, &pool);
			if (status == CELIX_SUCCESS) {
				if (apr_pool_create(&context->pool, pool) != APR_SUCCESS) {
					status = CELIX_ENOMEM;
				}

				*bundle_context = context;
			}
		} else {
			status = CELIX_ENOMEM;
		}
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t bundleContext_destroy(BUNDLE_CONTEXT context) {
	celix_status_t status = CELIX_SUCCESS;

	if (context != NULL) {
		if (context->pool) {
			apr_pool_destroy(context->pool);
		    context->pool = NULL;
		}
		context->bundle = NULL;
		context->framework = NULL;
		free(context);
		context = NULL;
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t bundleContext_getBundle(BUNDLE_CONTEXT context, BUNDLE *bundle) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*bundle = context->bundle;
	}

	return status;
}

celix_status_t bundleContext_getFramework(BUNDLE_CONTEXT context, FRAMEWORK *framework) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*framework = context->framework;
	}

	return status;
}

celix_status_t bundleContext_getMemoryPool(BUNDLE_CONTEXT context, apr_pool_t **memory_pool) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*memory_pool = context->pool;
	}

	return status;
}

celix_status_t bundleContext_installBundle(BUNDLE_CONTEXT context, char * location, BUNDLE *bundle) {
	return bundleContext_installBundle2(context, location, NULL, bundle);
}

celix_status_t bundleContext_installBundle2(BUNDLE_CONTEXT context, char * location, char *inputFile, BUNDLE *bundle) {
	celix_status_t status = CELIX_SUCCESS;
	BUNDLE b = NULL;

	if (context != NULL && *bundle == NULL) {
		if (fw_installBundle(context->framework, &b, location, inputFile) != CELIX_SUCCESS) {
            status = CELIX_FRAMEWORK_EXCEPTION;
		} else {
			*bundle = b;
		}
	} else {
        status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t bundleContext_registerService(BUNDLE_CONTEXT context, char * serviceName, void * svcObj,
        PROPERTIES properties, SERVICE_REGISTRATION *service_registration) {
	SERVICE_REGISTRATION registration = NULL;
	celix_status_t status = CELIX_SUCCESS;

	if (context != NULL && *service_registration == NULL) {
	    fw_registerService(context->framework, &registration, context->bundle, serviceName, svcObj, properties);
	    *service_registration = registration;
	} else {
	    status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t bundleContext_registerServiceFactory(BUNDLE_CONTEXT context, char * serviceName, service_factory_t factory,
        PROPERTIES properties, SERVICE_REGISTRATION *service_registration) {
    SERVICE_REGISTRATION registration = NULL;
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && *service_registration == NULL) {
        fw_registerServiceFactory(context->framework, &registration, context->bundle, serviceName, factory, properties);
        *service_registration = registration;
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t bundleContext_getServiceReferences(BUNDLE_CONTEXT context, const char * serviceName, char * filter, ARRAY_LIST *service_references) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && *service_references == NULL) {
        fw_getServiceReferences(context->framework, service_references, context->bundle, serviceName, filter);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

	return status;
}

celix_status_t bundleContext_getServiceReference(BUNDLE_CONTEXT context, char * serviceName, SERVICE_REFERENCE *service_reference) {
    SERVICE_REFERENCE reference = NULL;
    ARRAY_LIST services = NULL;
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

	return status;
}

celix_status_t bundleContext_getService(BUNDLE_CONTEXT context, SERVICE_REFERENCE reference, void **service_instance) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL && *service_instance == NULL) {
	    *service_instance = fw_getService(context->framework, context->bundle, reference);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t bundleContext_ungetService(BUNDLE_CONTEXT context, SERVICE_REFERENCE reference, bool *result) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL) {
        *result = framework_ungetService(context->framework, context->bundle, reference);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t bundleContext_getBundles(BUNDLE_CONTEXT context, ARRAY_LIST *bundles) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL || *bundles != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*bundles = framework_getBundles(context->framework);
	}

	return status;
}

celix_status_t bundleContext_getBundleById(BUNDLE_CONTEXT context, long id, BUNDLE *bundle) {
    celix_status_t status = CELIX_SUCCESS;

    if (context == NULL || *bundle != NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        *bundle = framework_getBundleById(context->framework, id);
    }

	return status;
}

celix_status_t bundleContext_addServiceListener(BUNDLE_CONTEXT context, SERVICE_LISTENER listener, char * filter) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_addServiceListener(context->framework, context->bundle, listener, filter);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t bundleContext_removeServiceListener(BUNDLE_CONTEXT context, SERVICE_LISTENER listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_removeServiceListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t bundleContext_addBundleListener(BUNDLE_CONTEXT context, bundle_listener_t listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_addBundleListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t bundleContext_removeBundleListener(BUNDLE_CONTEXT context, bundle_listener_t listener) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && listener != NULL) {
        fw_removeBundleListener(context->framework, context->bundle, listener);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    return status;
}

celix_status_t bundleContext_getProperty(BUNDLE_CONTEXT context, const char *name, char **value) {
	celix_status_t status = CELIX_SUCCESS;

	if (context == NULL || name == NULL || *value != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		fw_getProperty(context->framework, name, value);
	}

	return status;
}
