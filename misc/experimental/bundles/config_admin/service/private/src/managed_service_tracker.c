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
 * managed_service_tracker.c
 *
 *  \date       Aug 12, 2013
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

/* celix.config_admin.ManagedServiceTracker */
#include "managed_service_tracker.h"
#include "service_tracker_customizer.h"

/* celix.utils */
#include "hash_map.h"
/* celix.framework */
#include "celix_constants.h"
#include "properties.h"
#include "utils.h"
#include "service_reference.h"
#include "service_registration.h"
/* celix.framework.Patch*/
#include "framework_patch.h"
/* celix.config_admin.public */
#include "managed_service.h"
/* celix.config_admin.private */
#include "configuration_impl.h"
#include "updated_thread_pool.h"

struct managed_service_tracker {

    bundle_context_pt context;

    configuration_admin_factory_pt configurationAdminfactory;
    configuration_store_pt configurationStore;
    updated_thread_pool_pt updatedThreadPool; // according to org.equinox is our "SerializableTaskQueue"

    hash_map_pt managedServices;
    hash_map_pt managedServicesReferences;
    celix_thread_mutex_t managedServicesReferencesMutex;
};

static celix_status_t managedServiceTracker_createHandle(bundle_context_pt context, configuration_admin_factory_pt factory, configuration_store_pt store, managed_service_tracker_pt *tracker);
static celix_status_t managedServiceTracker_createCustomized(bundle_context_pt context, managed_service_tracker_pt trackerHandle, service_tracker_pt *tracker);

static celix_status_t managedServiceTracker_add(managed_service_tracker_pt tracker, service_reference_pt reference, char * pid, managed_service_service_pt service);
static celix_status_t managedServiceTracker_remove(managed_service_tracker_pt tracker, service_reference_pt reference, char * pid);
static celix_status_t managedServiceTracker_trackManagedService(managed_service_tracker_pt tracker, char *pid, service_reference_pt reference, managed_service_service_pt service);
static celix_status_t managedServiceTracker_untrackManagedService(managed_service_tracker_pt tracker, char *pid, service_reference_pt reference);
static celix_status_t managedServiceTracker_getManagedService(managed_service_tracker_pt tracker, char *pid, managed_service_service_pt *service);
static celix_status_t managedServiceTracker_getManagedServiceReference(managed_service_tracker_pt tracker, char *pid, service_reference_pt *reference);
//static celix_status_t managedServiceTracker_getPidForManagedService(managed_service_service_pt *service, char **pid);
celix_status_t managedServiceTracker_asynchUpdated(managed_service_tracker_pt trackerHandle, managed_service_service_pt service, properties_pt properties);

static celix_status_t managedServiceTracker_getBundleContext(managed_service_tracker_pt trackerHandle, bundle_context_pt *context);

static celix_status_t managedServiceTracker_lockManagedServicesReferences(managed_service_tracker_pt handle);
static celix_status_t managedServiceTracker_unlockManagedServicesReferences(managed_service_tracker_pt handle);

/* ========== CONSTRUCTOR ========== */

/* ---------- public ---------- */

celix_status_t managedServiceTracker_create(bundle_context_pt context, configuration_admin_factory_pt factory, configuration_store_pt store, managed_service_tracker_pt *trackerHandle, service_tracker_pt *tracker) {

    celix_status_t status;

    managed_service_tracker_pt managedServiceTrackerHandle;
    service_tracker_pt managedServiceTrackerCustomized;

    status = managedServiceTracker_createHandle(context, factory, store, &managedServiceTrackerHandle);
    if (status != CELIX_SUCCESS) {
        *trackerHandle = NULL;
        *tracker = NULL;
        return status;
    }

    status = managedServiceTracker_createCustomized(context, managedServiceTrackerHandle, &managedServiceTrackerCustomized);
    if (status != CELIX_SUCCESS) {
        *trackerHandle = NULL;
        *tracker = NULL;
        return status;
    }
    *trackerHandle = managedServiceTrackerHandle;
    *tracker = managedServiceTrackerCustomized;

    printf("[ SUCCESS ]: Tracker - Initialized \n");
    return CELIX_SUCCESS;

}

/* ---------- private ---------- */

celix_status_t managedServiceTracker_createHandle(bundle_context_pt context, configuration_admin_factory_pt factory, configuration_store_pt store, managed_service_tracker_pt *tracker) {

    celix_status_t status;

    updated_thread_pool_pt updatedThreadPool = NULL;
    managed_service_tracker_pt this = calloc(1, sizeof(*this));

    if (!this) {
        printf("[ ERROR ]: TrackerInstance - Not initialized (ENOMEM) \n");
        *tracker = NULL;
        return CELIX_ENOMEM;
    }

    status = updatedThreadPool_create(context, MAX_THREADS, &updatedThreadPool);
    if (status != CELIX_SUCCESS) {
        return status;
    }

    this->context = context;

    this->configurationAdminfactory = factory;
    this->configurationStore = store;
    this->updatedThreadPool = updatedThreadPool;

    this->managedServices = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    this->managedServicesReferences = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

    celix_status_t mutexStatus = celixThreadMutex_create(&this->managedServicesReferencesMutex, NULL);
    if (mutexStatus != CELIX_SUCCESS) {
        printf("[ ERROR ]: TrackerInstance - Not initialized (MUTEX) \n");
        // TODO destroy threadpool?
        return CELIX_ILLEGAL_ARGUMENT;
    }

    *tracker = this;
    return CELIX_SUCCESS;

}

celix_status_t managedServiceTracker_createCustomized(bundle_context_pt context, managed_service_tracker_pt trackerHandle, service_tracker_pt *tracker) {
    celix_status_t status;

    service_tracker_customizer_pt customizer = NULL;
    service_tracker_pt managedServiceTracker = NULL;

    status = serviceTrackerCustomizer_create(trackerHandle, managedServiceTracker_addingService, managedServiceTracker_addedService, managedServiceTracker_modifiedService, managedServiceTracker_removedService, &customizer);

    if (status != CELIX_SUCCESS) {
        printf("[ ERROR ]: TrackerCustomized - Not initialized(ENOMEM) \n");
        *tracker = NULL;
        return CELIX_ENOMEM;
    }

    serviceTracker_create(context, (char *) MANAGED_SERVICE_SERVICE_NAME, customizer, &managedServiceTracker);

    if (status != CELIX_SUCCESS) {
        printf("[ ERROR ]: TrackerCustomized - Not created \n");
        *tracker = NULL;
        return status;
    }

    *tracker = managedServiceTracker;
    return CELIX_SUCCESS;
}

celix_status_t managedServiceTracker_destroy(bundle_context_pt context, managed_service_tracker_pt mgServTr, service_tracker_pt tracker) {
	updatedThreadPool_destroy(mgServTr->updatedThreadPool);
	celixThreadMutex_destroy(&mgServTr->managedServicesReferencesMutex);
	serviceTracker_destroy(tracker);

	hashMap_destroy(mgServTr->managedServices, true, true);
	hashMap_destroy(mgServTr->managedServicesReferences, true, true);

    free(mgServTr);

	return CELIX_SUCCESS;
}



/* ========== IMPLEMENTS CUSTOMIZED TRACKER ========== */

/* ---------- public ---------- */

celix_status_t managedServiceTracker_addingService(void * handle, service_reference_pt reference, void **service) {


    celix_status_t status;

    const char* pid = NULL;

    bundle_context_pt context = NULL;

    managed_service_tracker_pt managedServiceTracker_i = handle;	//instance
    managed_service_service_pt managedService_s = NULL;			//service

    // (1) reference.getPid

    status = serviceReference_getProperty(reference, "service.pid", &pid);
    if (status != CELIX_SUCCESS || pid == NULL) {
        *service = NULL;
        printf(" [ ERROR ]: Tracker - PID is NULL \n");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    // (2) context.getManagedServiceService

    // (2.1) trackerInstance.getBundleContext

    if (managedServiceTracker_getBundleContext(managedServiceTracker_i, &context) != CELIX_SUCCESS) {
        *service = NULL;
        printf(" [ ERROR ]: Tracker - NULL bundleContext \n");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    // (2.2) context.getManagedServiceService

    if (bundleContext_getService(context, reference, (void*) &managedService_s) != CELIX_SUCCESS) {
        printf("[ ERROR ]: Tracker - AddingService ( BundleContext - getService{PID=%s} ) \n", pid);
        *service = NULL;
        return CELIX_ILLEGAL_ARGUMENT;
    }
    if (managedService_s == NULL) {
        printf("[ WARNING ]: Tracker - AddingService (none Service{PID=%s}) \n", pid);
        *service = NULL;
        return CELIX_ILLEGAL_ARGUMENT;
    }

    /* DEBUG CODE *

    service_registration_pt registration = NULL;
    serviceReference_getServiceRegistration(reference, &registration);
    char *serviceName = NULL;
    serviceRegistration_getServiceName(registration, &serviceName);

    printf("[ DEBUG ]: Tracker - AddingService ( SUCCESS BundleCtxt - getService{Name=%s,PID=%s}  ) \n", serviceName, pid);

    * ENF OF DEBUG CODE */

    // (3) trackerInstance.AddManagedServiceToLocalList
    configurationStore_lock(managedServiceTracker_i->configurationStore);

    status = managedServiceTracker_add(managedServiceTracker_i, reference, (char*)pid, managedService_s);
    if (status != CELIX_SUCCESS) {
        bundleContext_ungetService(context, reference, NULL);
    }
    configurationStore_unlock(managedServiceTracker_i->configurationStore);

    if (status != CELIX_SUCCESS) {
        *service = NULL;
    } else {
        *service = &managedService_s;
    }

    return status;

}

celix_status_t managedServiceTracker_addedService(void * handle, service_reference_pt reference, void * service) {
    return CELIX_SUCCESS;
}

celix_status_t managedServiceTracker_modifiedService(void * handle, service_reference_pt reference, void * service) {
    return CELIX_SUCCESS;
}

celix_status_t managedServiceTracker_removedService(void * handle, service_reference_pt reference, void * service) {
    celix_status_t status = CELIX_SUCCESS;
    const char* pid;
    managed_service_tracker_pt managedServiceTracker_i = handle;	//instance
    bundle_context_pt context;


    status = serviceReference_getProperty(reference, "service.pid", &pid);
    if (status != CELIX_SUCCESS || pid == NULL){
	return CELIX_ILLEGAL_ARGUMENT;
    }
    if ( managedServiceTracker_getBundleContext(managedServiceTracker_i, &context) != CELIX_SUCCESS ){
	return CELIX_ILLEGAL_ARGUMENT;
    }
    status = managedServiceTracker_remove(managedServiceTracker_i, reference, (char*)pid);

    return status;

}

/* ---------- private ---------- */
// org.eclipse.equinox.internal.cm.ManagedServiceTracker
celix_status_t managedServiceTracker_add(managed_service_tracker_pt tracker, service_reference_pt reference, char *pid, managed_service_service_pt service) {

    celix_status_t status;

    bundle_pt bundle = NULL;
    const char* bundleLocation;

    configuration_pt configuration = NULL;
    properties_pt properties = NULL;

    configurationStore_findConfiguration(tracker->configurationStore, pid, &configuration);

    if (configuration == NULL) {

        if (managedServiceTracker_trackManagedService(tracker, pid, reference, service) == CELIX_SUCCESS) {

            // TODO : this is new code, it hasn't been tested yet

            if (serviceReference_getBundle(reference, &bundle) != CELIX_SUCCESS) {
                return CELIX_ILLEGAL_ARGUMENT;
            }

            if (bundle_getBundleLocation(bundle, &bundleLocation) != CELIX_SUCCESS) {
                return CELIX_ILLEGAL_ARGUMENT;
            }

            // (1) creates a new Configuration for the ManagedService
            if (configurationStore_getConfiguration(tracker->configurationStore, pid, (char*)bundleLocation, &configuration) != CELIX_SUCCESS || configuration == NULL) {
                return CELIX_ILLEGAL_ARGUMENT;
            }

            // (2) bind the Configuration with the ManagedService
            bool dummy;
            if ((configuration_bind(configuration->handle, bundle, &dummy) != CELIX_SUCCESS)) {
                return CELIX_ILLEGAL_ARGUMENT;
            }

            // (3) the new Configuration is persisted and visible for other ConfigAdmin instances
            if (configurationStore_saveConfiguration(tracker->configurationStore, pid, configuration) != CELIX_SUCCESS) {
                return CELIX_ILLEGAL_STATE;
            }

            // End of new code

            // TODO: It must be considered in case of fail if untrack the ManagedService

            return managedServiceTracker_asynchUpdated(tracker, service, NULL);

        } else {
            return CELIX_ILLEGAL_ARGUMENT; // the service was already tracked
        }

    } else {

        configuration_lock(configuration->handle);

        if (managedServiceTracker_trackManagedService(tracker, pid, reference, service) == CELIX_SUCCESS) {

            if (serviceReference_getBundle(reference, &bundle) != CELIX_SUCCESS) {
                configuration_unlock(configuration->handle);
                printf("[ERROR ]: Tracker - Add (Service{PID=%s} Reference - getBundle NULL)", pid);
                return CELIX_ILLEGAL_ARGUMENT;
            }

            // TODO configuration.isDeleted ? - with only using one calling bundle OK

            bool isBind;
            if ((configuration_bind(configuration->handle, bundle, &isBind) == CELIX_SUCCESS) && (isBind == true)) { // config.bind(bundle)

                if (configuration_getProperties(configuration->handle, &properties) != CELIX_SUCCESS) {
                    configuration_unlock(configuration->handle);
                    return CELIX_ILLEGAL_ARGUMENT;
                }

                if (configurationAdminFactory_modifyConfiguration(tracker->configurationAdminfactory, reference, properties) != CELIX_SUCCESS) {
                    configuration_unlock(configuration->handle);
                    return CELIX_ILLEGAL_ARGUMENT;
                }

                status = managedServiceTracker_asynchUpdated(tracker, service, properties);

                configuration_unlock(configuration->handle);

                return status;

            } else {
                configuration_unlock(configuration->handle);
                return CELIX_ILLEGAL_STATE;
            }

        } else {
            configuration_unlock(configuration->handle);
            return CELIX_ILLEGAL_ARGUMENT; // the service was already tracked
        }
    }
}

celix_status_t managedServiceTracker_remove(managed_service_tracker_pt tracker, service_reference_pt reference, char * pid){
    configuration_pt configuration = NULL;
    bundle_pt bundle = NULL;

    configurationStore_findConfiguration(tracker->configurationStore, pid, &configuration);
    if (configuration != NULL) {
        if (serviceReference_getBundle(reference, &bundle) == CELIX_SUCCESS) {
			configuration_unbind(configuration->handle, bundle);
		}	
	}
	return managedServiceTracker_untrackManagedService(tracker, pid, reference);
}

celix_status_t managedServiceTracker_trackManagedService(managed_service_tracker_pt tracker, char *pid, service_reference_pt reference, managed_service_service_pt service) {

    managedServiceTracker_lockManagedServicesReferences(tracker);

    if (hashMap_containsKey(tracker->managedServicesReferences, pid)) {
        printf("[ WARNING ]: Tracker - Track ( Service{PID=%s} already registered ) ", pid);
        managedServiceTracker_unlockManagedServicesReferences(tracker);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    hashMap_put(tracker->managedServicesReferences, pid, reference);
    hashMap_put(tracker->managedServices, pid, service);

    managedServiceTracker_unlockManagedServicesReferences(tracker);

    return CELIX_SUCCESS;
}

celix_status_t managedServiceTracker_untrackManagedService(managed_service_tracker_pt tracker, char *pid, service_reference_pt reference){
    managedServiceTracker_lockManagedServicesReferences(tracker);

    if ( hashMap_containsKey(tracker->managedServicesReferences, pid) ){
	hashMap_remove(tracker->managedServicesReferences, pid);
	hashMap_remove(tracker->managedServices, pid);
    }
    managedServiceTracker_unlockManagedServicesReferences(tracker);
    return CELIX_SUCCESS;

}

celix_status_t managedServiceTracker_getManagedService(managed_service_tracker_pt tracker, char *pid, managed_service_service_pt *service) {

    celix_status_t status;
    managed_service_service_pt serv = NULL;

    managedServiceTracker_lockManagedServicesReferences(tracker);

    serv = hashMap_get(tracker->managedServices, pid);
    if (serv == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        status = CELIX_SUCCESS;
    }

    managedServiceTracker_unlockManagedServicesReferences(tracker);

    *service = serv;
    return status;
}

celix_status_t managedServiceTracker_getManagedServiceReference(managed_service_tracker_pt tracker, char *pid, service_reference_pt *reference) {

    celix_status_t status;
    service_reference_pt ref = NULL;

    managedServiceTracker_lockManagedServicesReferences(tracker);

    ref = hashMap_get(tracker->managedServicesReferences, pid);
    if (ref == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        status = CELIX_SUCCESS;
    }

    managedServiceTracker_unlockManagedServicesReferences(tracker);

    *reference = ref;
    return status;
}

/* TODO
 celix_status_t managedServiceTracker_getPidForManagedService(managed_service_service_pt *service, char **pid){
 return CELIX_SUCCESS;
 }
 */

celix_status_t managedServiceTracker_asynchUpdated(managed_service_tracker_pt trackerHandle, managed_service_service_pt service, properties_pt properties) {

    return updatedThreadPool_push(trackerHandle->updatedThreadPool, service, properties);

}

/* ========== IMPLEMENTATION  ========== */

/* ---------- public ---------- */

celix_status_t managedServiceTracker_notifyDeleted(managed_service_tracker_pt tracker, configuration_pt configuration) {
    return CELIX_SUCCESS;
}

celix_status_t managedServiceTracker_notifyUpdated(managed_service_tracker_pt tracker, configuration_pt configuration) {


    char *pid;

    service_reference_pt reference = NULL;
    bundle_pt bundle = NULL;
    properties_pt properties = NULL;

    managed_service_service_pt service = NULL;

    // (1) config.checkLocked
    if (configuration_checkLocked(configuration->handle) != CELIX_SUCCESS) { //TODO not yet implemented
        return CELIX_ILLEGAL_ARGUMENT;
    }

    // (2) config.getPid
    if (configuration_getPid(configuration->handle, &pid) != CELIX_SUCCESS) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    // (3) reference = getManagedServiceReference(pid)
    if (managedServiceTracker_getManagedServiceReference(tracker, pid, &reference) != CELIX_SUCCESS || reference == NULL) {
        printf("[ ERROR ]: Tracker - Notify (NULL Reference Service{PID=%s}) \n", pid);
        return CELIX_ILLEGAL_ARGUMENT; // Eclipse ignores, but according to Specs, callback is delayed
    }

    //  (4.1) reference.getBundle
    if (serviceReference_getBundle(reference, &bundle) != CELIX_SUCCESS || bundle == NULL) {
        printf("[ ERROR ]: Tracker - Notify (NULL Bundle Service{PID=%s}) \n", pid);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    //	(4.2) config.bind(reference.getBundle)
    bool isBind;
    if (configuration_bind(configuration->handle, bundle, &isBind) != CELIX_SUCCESS || isBind == false) {
        printf("[ ERROR ]: Tracker - Notify (Service{PID=%s} Permission Error) \n", pid);
        return CELIX_ILLEGAL_STATE;
    }

    // (5) if (reference != null && config.bind(reference.getBundle()))

    // (5.1) properties = config.getProperties
    if (configuration_getProperties(configuration->handle, &properties) != CELIX_SUCCESS) {
        printf("[ ERROR ]: Tracker - Notify (Service{PID=%s} Wrong Properties) \n", pid);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    // (5.2) modifyConfiguration
    if (configurationAdminFactory_modifyConfiguration(tracker->configurationAdminfactory, reference, properties) != CELIX_SUCCESS) {
        return CELIX_ILLEGAL_ARGUMENT; //TODO no yet implemented modifyConfiguration
    }

    // (5.3) service = getManagedService(pid)
    if (managedServiceTracker_getManagedService(tracker, pid, &service) != CELIX_SUCCESS) {
        printf("[ ERROR ]: Tracker - Notify (NULL Service{PID=%s}) \n", pid);
        return CELIX_ILLEGAL_ARGUMENT;
    }

    // (5.4) asynchUpdate(service,properties)
    if ((properties == NULL) || (properties != NULL && hashMap_size(properties) == 0)) {
        return managedServiceTracker_asynchUpdated(tracker, service, NULL);
    } else {
        return managedServiceTracker_asynchUpdated(tracker, service, properties);
    }
    return CELIX_ILLEGAL_ARGUMENT;
}

/* ---------- private ---------- */

celix_status_t managedServiceTracker_getBundleContext(managed_service_tracker_pt trackerHandle, bundle_context_pt *context) {

    if (trackerHandle->context != NULL) {
        *context = trackerHandle->context;
    } else {
        printf("[ ERROR ]: Tracker - getBundleContext (NULL context) \n");
        *context = NULL;
        return CELIX_ILLEGAL_ARGUMENT;
    }
    return CELIX_SUCCESS;
}

celix_status_t managedServiceTracker_lockManagedServicesReferences(managed_service_tracker_pt handle) {

    celixThreadMutex_lock(&handle->managedServicesReferencesMutex);
    return CELIX_SUCCESS;

}

celix_status_t managedServiceTracker_unlockManagedServicesReferences(managed_service_tracker_pt handle) {

    celixThreadMutex_unlock(&handle->managedServicesReferencesMutex);
    return CELIX_SUCCESS;

}
