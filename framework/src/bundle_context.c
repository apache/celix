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


#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "constants.h"
#include "bundle_context_private.h"
#include "framework_private.h"
#include "bundle.h"
#include "celix_log.h"
#include "service_tracker.h"

celix_status_t bundleContext_create(framework_pt framework, framework_logger_pt logger, bundle_pt bundle, bundle_context_pt *bundle_context) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_context_pt context = NULL;

	if (*bundle_context != NULL && framework == NULL && bundle == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
        context = malloc(sizeof(*context));
        if (!context) {
            status = CELIX_ENOMEM;
        } else {
            context->framework = framework;
            context->bundle = bundle;
            context->mng = NULL;

            arrayList_create(&context->svcRegistrations);
            celixThreadMutex_create(&context->mutex, NULL);

            *bundle_context = context;

        }
	}

	framework_logIfError(logger, status, NULL, "Failed to create context");

	return status;
}

celix_status_t bundleContext_destroy(bundle_context_pt context) {
	celix_status_t status = CELIX_SUCCESS;

	if (context != NULL) {
	    //NOTE still present service registartion will be cleared during bundle stop in the
	    //service registry (serviceRegistry_clearServiceRegistrations).
	    celixThreadMutex_destroy(&context->mutex); 
	    arrayList_destroy(context->svcRegistrations);
	    free(context);
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

celix_status_t bundleContext_installBundle(bundle_context_pt context, const char * location, bundle_pt *bundle) {
	return bundleContext_installBundle2(context, location, NULL, bundle);
}

celix_status_t bundleContext_installBundle2(bundle_context_pt context, const char * location, const char *inputFile, bundle_pt *bundle) {
	celix_status_t status = CELIX_SUCCESS;
	bundle_pt b = NULL;

	if (context != NULL) {
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

celix_status_t bundleContext_registerService(bundle_context_pt context, const char * serviceName, const void * svcObj,
        properties_pt properties, service_registration_pt *service_registration) {
	service_registration_pt registration = NULL;
	celix_status_t status = CELIX_SUCCESS;

	if (context != NULL) {
	    fw_registerService(context->framework, &registration, context->bundle, serviceName, svcObj, properties);
	    *service_registration = registration;
	} else {
	    status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(logger, status, NULL, "Failed to register service. serviceName '%s'", serviceName);

	return status;
}

celix_status_t bundleContext_registerServiceFactory(bundle_context_pt context, const char * serviceName, service_factory_pt factory,
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

celix_status_t bundleContext_getServiceReferences(bundle_context_pt context, const char * serviceName, const char * filter, array_list_pt *service_references) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && *service_references == NULL) {
        fw_getServiceReferences(context->framework, service_references, context->bundle, serviceName, filter);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to get service references");

	return status;
}

celix_status_t bundleContext_getServiceReference(bundle_context_pt context, const char * serviceName, service_reference_pt *service_reference) {
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

FRAMEWORK_EXPORT celix_status_t bundleContext_retainServiceReference(bundle_context_pt context, service_reference_pt ref) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && ref != NULL) {
        serviceRegistry_retainServiceReference(context->framework->registry, context->bundle, ref);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to get service references");

    return status;
}

celix_status_t bundleContext_ungetServiceReference(bundle_context_pt context, service_reference_pt reference) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL) {
        status = framework_ungetServiceReference(context->framework, context->bundle, reference);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
    }

    framework_logIfError(logger, status, NULL, "Failed to unget service_reference");

    return status;
}

celix_status_t bundleContext_getService(bundle_context_pt context, service_reference_pt reference, void** service_instance) {
    celix_status_t status = CELIX_SUCCESS;

    if (context != NULL && reference != NULL && service_instance != NULL) {
        /*NOTE argument service_instance should be considerd a 'const void**'. 
        To ensure backwards compatiblity a cast is made instead.
        */
	    status = fw_getService(context->framework, context->bundle, reference, (const void**) service_instance);
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
        if (*bundle == NULL) {
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    framework_logIfError(logger, status, NULL, "Failed to get bundle [id=%ld]", id);

	return status;
}

celix_status_t bundleContext_addServiceListener(bundle_context_pt context, service_listener_pt listener, const char* filter) {
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

celix_status_t bundleContext_getProperty(bundle_context_pt context, const char *name, const char** value) {
	return bundleContext_getPropertyWithDefault(context, name, NULL, value);
}

celix_status_t bundleContext_getPropertyWithDefault(bundle_context_pt context, const char* name, const char* defaultValue, const char** value) {
    celix_status_t status = CELIX_SUCCESS;

    if (context == NULL || name == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        fw_getProperty(context->framework, name, defaultValue, value);
    }

    framework_logIfError(logger, status, NULL, "Failed to get property [name=%s]", name);

    return status;
}


long bundleContext_registerCService(bundle_context_t *ctx, const char *serviceName, void *svc, properties_t *properties, const char *serviceVersion) {
    return bundleContext_registerServiceForLang(ctx, serviceName, svc, properties, serviceVersion, CELIX_FRAMEWORK_SERVICE_C_LANGUAGE);
}


long bundleContext_registerServiceForLang(bundle_context_t *ctx, const char *serviceName, void *svc, properties_t *properties, const char *serviceVersion, const char* lang) {
    long svcId = -1;
    if (properties == NULL) {
        properties = properties_create();
    }
    service_registration_t *reg = NULL;
    if (serviceVersion != NULL) {
        properties_set(properties, CELIX_FRAMEWORK_SERVICE_VERSION, serviceVersion);
    }
    if (serviceName != NULL && lang != NULL) {
        properties_set(properties, CELIX_FRAMEWORK_SERVICE_LANGUAGE, lang);
        bundleContext_registerService(ctx, serviceName, svc, properties, &reg);
        svcId = serviceRegistration_getServiceId(reg); //save to call with NULL
    } else {
        if (serviceName == NULL) {
            framework_logIfError(logger, CELIX_ILLEGAL_ARGUMENT, NULL, "Required serviceName argument is NULL");
        }
        if (lang == NULL) {
            framework_logIfError(logger, CELIX_ILLEGAL_ARGUMENT, NULL, "Required lang argument is NULL");
        }
    }
    if (svcId < 0) {
        properties_destroy(properties);
    } else {
        celixThreadMutex_lock(&ctx->mutex);
        arrayList_add(ctx->svcRegistrations, reg);
        celixThreadMutex_unlock(&ctx->mutex);
    }
    return svcId;
}

void bundleContext_unregisterService(bundle_context_t *ctx, long serviceId) {
    service_registration_t *found = NULL;
    if (ctx != NULL && serviceId >= 0) {
        celixThreadMutex_lock(&ctx->mutex);
        unsigned int size = arrayList_size(ctx->svcRegistrations);
        for (unsigned int i = 0; i < size; ++i) {
            service_registration_t *reg = arrayList_get(ctx->svcRegistrations, i);
            if (reg != NULL) {
                long svcId = serviceRegistration_getServiceId(reg);
                if (svcId == serviceId) {
                    found = reg;
                    arrayList_remove(ctx->svcRegistrations, i);
                    break;
                }
            }
        }
        celixThreadMutex_unlock(&ctx->mutex);

        if (found != NULL) {
            serviceRegistration_unregister(found);
        } else {
            framework_logIfError(logger, CELIX_ILLEGAL_ARGUMENT, NULL, "Provided service id is not used to registered using bundleContext_registerCService/registerServiceForLang");
        }
    }
}

bool bundleContext_useServiceWithId(
        bundle_context_t *ctx,
        long serviceId,
        const char *serviceName,
        void *callbackHandle,
        void (*use)(void *handle, void *svc, const properties_t *props, const bundle_t *owner)) {
    bool called = false;
    char filter[64];
    snprintf(filter, 64, "(%s=%li)", OSGI_FRAMEWORK_SERVICE_ID, serviceId);
    service_tracker_t *trk = NULL;
    serviceTracker_createWithFilter(ctx, filter, NULL, &trk);
    if (trk != NULL) {
	serviceTracker_open(trk);
        bundle_t *bnd = NULL;
        properties_t *props = NULL;
        const char *registerServiceName = NULL;
        bool sameName = false;
        void *svc = serviceTracker_lockAndGetService(trk, &props, &bnd);
        if (props != NULL) {
            registerServiceName = properties_get(props, OSGI_FRAMEWORK_OBJECTCLASS);
            sameName = strncmp(serviceName, registerServiceName, 1024*1024*10) == 0;
        }
        if (svc != NULL && sameName) {
            use(callbackHandle, svc, props, bnd);
            called = true;
        } else if (svc != NULL) {
            framework_logIfError(logger, CELIX_ILLEGAL_ARGUMENT, NULL,
                                 "Service with id %li does not have service name '%s', but has service name '%s'",
                                 serviceId, serviceName, registerServiceName);
        }
        serviceTracker_unlockAndUngetService(trk, svc);
        serviceTracker_close(trk);
        serviceTracker_destroy(trk);
    }
    return called;
}


dm_dependency_manager_t* bundleContext_getDependencyManager(bundle_context_t *ctx) {
    dm_dependency_manager_t* result = NULL;
    if (ctx != NULL) {
        celixThreadMutex_lock(&ctx->mutex);
        if (ctx->mng == NULL) {
            dependencyManager_create(ctx, &ctx->mng);
        }
        if (ctx->mng == NULL) {
            framework_logIfError(logger, CELIX_BUNDLE_EXCEPTION, NULL, "Cannot create dependency manager");
        }
        result = ctx->mng;
        celixThreadMutex_unlock(&ctx->mutex);
    }
    return result;
}
