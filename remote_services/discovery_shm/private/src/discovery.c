
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
 * discovery.c
 *
 *  \date       Oct 4, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include <apr_strings.h>
#include <apr_thread_proc.h>

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "bundle_context.h"
#include "constants.h"
#include "array_list.h"
#include "utils.h"
#include "celix_errno.h"
#include "filter.h"
#include "service_reference.h"
#include "service_registration.h"
#include "netstring.h"

#include "discovery.h"

celix_status_t discovery_informListener(discovery_pt discovery, endpoint_listener_pt listener, endpoint_description_pt endpoint);
celix_status_t discovery_informListenerOfRemoval(discovery_pt discovery, endpoint_listener_pt listener, endpoint_description_pt endpoint);

celix_status_t discovery_addService(discovery_pt discovery, endpoint_description_pt endpoint);
celix_status_t discovery_removeService(discovery_pt discovery, endpoint_description_pt endpoint);

static void *APR_THREAD_FUNC discovery_pollSHMServices(apr_thread_t *thd, void *data);
celix_status_t discovery_lock(int semId, int semNr);
celix_status_t discovery_unlock(int semId, int semNr);
celix_status_t discovery_broadcast(int semId, int semNr);
celix_status_t discovery_stillAlive(char* pid, bool* stillAlive);

celix_status_t discovery_updateLocalSHMServices(discovery_pt discovery);
celix_status_t discovery_updateSHMServices(discovery_pt discovery, endpoint_description_pt endpoint, bool addService);

celix_status_t discovery_registerSHMService(discovery_pt discovery, endpoint_description_pt endpoint);
celix_status_t discovery_deregisterSHMService(discovery_pt discovery, endpoint_description_pt endpoint);
celix_status_t discovery_createOrAttachShm(discovery_pt discovery);
celix_status_t discovery_stopOrDetachShm(discovery_pt discovery);

celix_status_t discovery_create(apr_pool_t *pool, bundle_context_pt context, discovery_pt *discovery) {
	celix_status_t status = CELIX_SUCCESS;

	printf("DISCOVERY started.\n");

	*discovery = apr_palloc(pool, sizeof(**discovery));
	if (!*discovery) {
		status = CELIX_ENOMEM;
	} else {
		(*discovery)->context = context;
		(*discovery)->pool = pool;
		(*discovery)->listenerReferences = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
		(*discovery)->shmServices = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

		(*discovery)->running = true;
		(*discovery)->registered = NULL;
		arrayList_create(&(*discovery)->registered);

		(*discovery)->shmId = -1;
		(*discovery)->shmBaseAdress = NULL;

		if ((status = discovery_createOrAttachShm(*discovery)) != CELIX_SUCCESS)
		{
			printf("DISCOVERY: Shared Memory initialization failed.");
		}
		else
		{
			apr_thread_create(&(*discovery)->shmPollThread, NULL, discovery_pollSHMServices, *discovery, (*discovery)->pool);
		}
	}

	return status;
}

celix_status_t discovery_destroy(discovery_pt discovery) {
	celix_status_t status = CELIX_SUCCESS;

	arrayList_destroy(discovery->registered);
	hashMap_destroy(discovery->shmServices, false, false);
	hashMap_destroy(discovery->listenerReferences, false, false);

	return status;
}

celix_status_t discovery_stop(discovery_pt discovery) {
	celix_status_t status = CELIX_SUCCESS;
	apr_status_t tstat;
	discovery->running = false;

	ipc_shmData_pt shmData = (ipc_shmData_pt) discovery->shmBaseAdress;

	if (shmData != NULL)
	{
		discovery_broadcast(shmData->semId, 1);

		apr_status_t stat = apr_thread_join(&tstat, discovery->shmPollThread);

		if (stat != APR_SUCCESS && tstat != APR_SUCCESS)
				{
			printf("DISCOVERY: An error occured while stopping the SHM polling thread.\n");
			status = CELIX_BUNDLE_EXCEPTION;
		}
		else
		{
			printf("DISCOVERY: SHM polling thread sucessfully stopped.\n");
			int i;
			for (i = 0; i < arrayList_size(discovery->registered); i++) {

				endpoint_description_pt endpoint = (endpoint_description_pt) arrayList_get(discovery->registered, i);
				printf("DISCOVERY: deregistering service %s.\n", endpoint->service);
				status = discovery_deregisterSHMService(discovery, endpoint);
			}

			// detach from shm
			status = discovery_stopOrDetachShm(discovery);
		}
	}

	return status;
}

celix_status_t discovery_removeService(discovery_pt discovery, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Remove service (%s)\n", endpoint->service);

	// Inform listeners of new endpoint
	hash_map_iterator_pt iter = hashMapIterator_create(discovery->listenerReferences);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		service_reference_pt reference = hashMapEntry_getKey(entry);
		endpoint_listener_pt listener = NULL;
		bundleContext_getService(discovery->context, reference, (void**) &listener);
		discovery_informListenerOfRemoval(discovery, listener, endpoint);
	}
	hashMapIterator_destroy(iter);

	return status;
}

celix_status_t discovery_addService(discovery_pt discovery, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	// Inform listeners of new endpoint
	hash_map_iterator_pt iter = hashMapIterator_create(discovery->listenerReferences);

	printf("DISCOVERY: Add service (%s)\n", endpoint->service);

	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		service_reference_pt reference = hashMapEntry_getKey(entry);
		endpoint_listener_pt listener = NULL;

		service_registration_pt registration = NULL;
		serviceReference_getServiceRegistration(reference, &registration);
		properties_pt serviceProperties = NULL;
		serviceRegistration_getProperties(registration, &serviceProperties);
		char *scope = properties_get(serviceProperties, (char *) OSGI_ENDPOINT_LISTENER_SCOPE);
		filter_pt filter = filter_create(scope);
		bool matchResult = false;
		filter_match(filter, endpoint->properties, &matchResult);
		if (matchResult) {
			printf("DISCOVERY: Add service (%s)\n", endpoint->service);
			bundleContext_getService(discovery->context, reference, (void**) &listener);
			discovery_informListener(discovery, listener, endpoint);
		}
		filter_destroy(filter);
	}
	hashMapIterator_destroy(iter);

	return status;
}

celix_status_t discovery_informListener(discovery_pt discovery, endpoint_listener_pt listener, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	listener->endpointAdded(listener->handle, endpoint, NULL);
	return status;
}

celix_status_t discovery_informListenerOfRemoval(discovery_pt discovery, endpoint_listener_pt listener, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	listener->endpointRemoved(listener->handle, endpoint, NULL);
	return status;
}

celix_status_t discovery_lock(int semId, int semNr)
{
	celix_status_t status = CELIX_SUCCESS;
	int semOpStatus = 0;
	struct sembuf semOperation;

	semOperation.sem_num = semNr;
	semOperation.sem_op = -1;
	semOperation.sem_flg = 0;

	do
	{
		status = CELIX_SUCCESS;

		if ((semOpStatus = semop(semId, &semOperation, 1)) != 0)
		{
			status = CELIX_BUNDLE_EXCEPTION;
		}
	} while (semOpStatus == -1 && errno == EINTR);

	return status;
}

celix_status_t discovery_unlock(int semId, int semNr)
{
	celix_status_t status = CELIX_SUCCESS;
	int semOpStatus = 0;
	struct sembuf semOperation;

	semOperation.sem_num = semNr;
	semOperation.sem_op = 1;
	semOperation.sem_flg = 0;

	do
	{
		status = CELIX_SUCCESS;

		if ((semOpStatus = semop(semId, &semOperation, 1)) != 0)
		{
			status = CELIX_BUNDLE_EXCEPTION;
		}
	} while (semOpStatus == -1 && errno == EINTR);

	return status;
}

celix_status_t discovery_broadcast(int semId, int semNr)
{
	celix_status_t status = CELIX_SUCCESS;
	int semOpStatus = 0;
	struct sembuf semOperation;

	semOperation.sem_num = semNr;
	semOperation.sem_op = semctl(semId, semNr, GETNCNT, 0) + 1; /* + 1 cause we also want to include out own process */
	semOperation.sem_flg = 0;

	do
	{
		status = CELIX_SUCCESS;

		if ((semOpStatus = semop(semId, &semOperation, 1)) != 0)
				{
			status = CELIX_BUNDLE_EXCEPTION;
		}
	} while (semOpStatus == -1 && errno == EINTR);

	return status;
}

celix_status_t discovery_decShmMapService(discovery_pt discovery, char* encServiceMap, hash_map_pt outShmMap)
{
	celix_status_t status = CELIX_SUCCESS;

	if ((status = netstring_decodeToHashMap(discovery->pool, encServiceMap, outShmMap)) != CELIX_SUCCESS)
	{
		printf("DISCOVERY: discovery_decShmMapService : decoding service data to hashmap failed\n");
	}
	else
	{
		// decode service properties as well
		char* encServiceProps = hashMap_get(outShmMap, DISCOVERY_SHM_SRVC_PROPERTIES);
		hash_map_pt props = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);

		if ((status = netstring_decodeToHashMap(discovery->pool, encServiceProps, props)) != CELIX_SUCCESS)
		{
			printf("DISCOVERY: discovery_decShmMapService : Decoding of endpointProperties failed\n");
		}

		hashMap_put(outShmMap, DISCOVERY_SHM_SRVC_PROPERTIES, props);
	}

	return status;
}

celix_status_t discovery_encShmMapService(discovery_pt discovery, hash_map_pt inShmMap, char** outEncServiceMap)
{
	celix_status_t status = CELIX_SUCCESS;

	// encode service properties as well
	char* encServiceProps = NULL;
	hash_map_pt props = hashMap_get(inShmMap, DISCOVERY_SHM_SRVC_PROPERTIES);

	if ((status = netstring_encodeFromHashMap(discovery->pool, props, &encServiceProps)) != CELIX_SUCCESS)
	{
		printf("DISCOVERY: discovery_encShmMapService : encoding of endpointProperties failed\n");
	}
	else
	{
		hashMap_put(inShmMap, DISCOVERY_SHM_SRVC_PROPERTIES, encServiceProps);

		if ((status = netstring_encodeFromHashMap(discovery->pool, inShmMap, outEncServiceMap)) != CELIX_SUCCESS)
		{
			printf("DISCOVERY: discovery_encShmMapService : encoding service data to hashmap failed\n");
		}
	}

	// we can only free that if not allocated via apr
	hashMap_destroy(props, false, false);

	return status;
}

celix_status_t discovery_decShmMapDiscoveryInstance(discovery_pt discovery, char* encDiscInstance, hash_map_pt outRegServices)
{
	celix_status_t status = CELIX_BUNDLE_EXCEPTION;

	if ((status = netstring_decodeToHashMap(discovery->pool, encDiscInstance, outRegServices)) != CELIX_SUCCESS)
	{
		printf("DISCOVERY: discovery_decShmMapDiscoveryInstance : decoding data to properties failed\n");
	}
	else {

		char* encServices = hashMap_get(outRegServices, DISCOVERY_SHM_FW_SERVICES);
		hash_map_pt fwServices = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);

		if ((status = netstring_decodeToHashMap(discovery->pool, encServices, fwServices)) != CELIX_SUCCESS)
		{
			printf("DISCOVERY: discovery_decShmMapDiscoveryInstance : decoding services failed\n");
		}
		else
		{
			hash_map_iterator_pt shmItr = hashMapIterator_create(fwServices);

			while ((status == CELIX_SUCCESS) && (hashMapIterator_hasNext(shmItr) == true))
			{
				hash_map_pt regShmService = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
				hash_map_entry_pt shmSrvc = hashMapIterator_nextEntry(shmItr);

				char *serviceName = hashMapEntry_getKey(shmSrvc);
				char *encServiceMap = hashMapEntry_getValue(shmSrvc);

				if ((status = discovery_decShmMapService(discovery, encServiceMap, regShmService)) == CELIX_SUCCESS)
				{
					hashMap_put(fwServices, serviceName, regShmService);
				}
			}

			hashMapIterator_destroy(shmItr);

			hashMap_put(outRegServices, DISCOVERY_SHM_FW_SERVICES, fwServices);
		}
	}

	return status;
}

// fwId -> hm_services
celix_status_t discovery_encShmMapDiscoveryInstance(discovery_pt discovery, hash_map_pt inFwAttr, char** outEncDiscoveryInstance)
{
	celix_status_t status = CELIX_SUCCESS;

	hash_map_pt inRegServices = hashMap_get(inFwAttr, DISCOVERY_SHM_FW_SERVICES);
	hash_map_iterator_pt shmItr = hashMapIterator_create(inRegServices);

	while ((status == CELIX_SUCCESS) && (hashMapIterator_hasNext(shmItr) == true))
	{
		hash_map_entry_pt shmSrvc = hashMapIterator_nextEntry(shmItr);

		char *encServiceMap = NULL;
		char *serviceName = hashMapEntry_getKey(shmSrvc);

		hash_map_pt regShmService = hashMapEntry_getValue(shmSrvc);

		if ((status = discovery_encShmMapService(discovery, regShmService, &encServiceMap)) == CELIX_SUCCESS)
		{
			hashMap_put(inRegServices, serviceName, encServiceMap);
		}

		hashMap_destroy(regShmService, false, false);
	}

	hashMapIterator_destroy(shmItr);

	if (status == CELIX_SUCCESS)
	{
		char* outEncServices = NULL;

		if ((status = netstring_encodeFromHashMap(discovery->pool, inRegServices, &outEncServices)) != CELIX_SUCCESS)
		{
			printf("DISCOVERY: discovery_encShmMapDiscoveryInstance : encode services failed\n");
		}
		else
		{
			hashMap_put(inFwAttr, DISCOVERY_SHM_FW_SERVICES, outEncServices);

			if ((status = netstring_encodeFromHashMap(discovery->pool, inFwAttr, outEncDiscoveryInstance)) != CELIX_SUCCESS)
			{
				printf("DISCOVERY: discovery_encShmMapDiscoveryInstance : encode discovery instances failed\n");
			}

		}
	}

	hashMap_destroy(inRegServices, false, false);

	return status;
}

celix_status_t discovery_decShmMap(discovery_pt discovery, char* encMap, hash_map_pt outRegDiscInstances)
{
	celix_status_t status = CELIX_BUNDLE_EXCEPTION;

	if ((status = netstring_decodeToHashMap(discovery->pool, encMap, outRegDiscInstances)) != CELIX_SUCCESS)
	{
		printf("DISCOVERY: discovery_updateLocalSHMServices : decoding data to properties failed\n");
	}
	else
	{
		hash_map_iterator_pt regDiscoveryInstancesItr = hashMapIterator_create(outRegDiscInstances);

		while ((status == CELIX_SUCCESS) && (hashMapIterator_hasNext(regDiscoveryInstancesItr) == true))
		{
			hash_map_pt regDiscoveryInstance = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
			hash_map_entry_pt regDiscoveryEntry = hashMapIterator_nextEntry(regDiscoveryInstancesItr);

			char* fwPid = hashMapEntry_getKey(regDiscoveryEntry);
			char* encDiscoveryInstance = hashMapEntry_getValue(regDiscoveryEntry);

			if ((status = discovery_decShmMapDiscoveryInstance(discovery, encDiscoveryInstance, regDiscoveryInstance)) == CELIX_SUCCESS)
			{
				hashMap_put(outRegDiscInstances, fwPid, regDiscoveryInstance);
			}
		}

		hashMapIterator_destroy(regDiscoveryInstancesItr);

	}

	return status;
}

celix_status_t discovery_encShmMap(discovery_pt discovery, hash_map_pt inRegDiscInstances, char** outEncMap)
{
	celix_status_t status = CELIX_SUCCESS;

	hash_map_iterator_pt regDiscoveryInstancesItr = hashMapIterator_create(inRegDiscInstances);

	while ((status == CELIX_SUCCESS) && (hashMapIterator_hasNext(regDiscoveryInstancesItr) == true))
	{
		hash_map_entry_pt regDiscoveryEntry = hashMapIterator_nextEntry(regDiscoveryInstancesItr);

		char* encDiscoveryInstance = NULL;
		char* fwPid = hashMapEntry_getKey(regDiscoveryEntry);
		hash_map_pt regDiscoveryInstance = hashMapEntry_getValue(regDiscoveryEntry);

		if ((status = discovery_encShmMapDiscoveryInstance(discovery, regDiscoveryInstance, &encDiscoveryInstance)) == CELIX_SUCCESS)
		{
			hashMap_put(inRegDiscInstances, fwPid, encDiscoveryInstance);
		}

		hashMap_destroy(regDiscoveryInstance, false, false);
	}

	hashMapIterator_destroy(regDiscoveryInstancesItr);

	if ((status == CELIX_SUCCESS) && ((status = netstring_encodeFromHashMap(discovery->pool, inRegDiscInstances, outEncMap)) != CELIX_SUCCESS))
			{
		printf("DISCOVERY: discovery_encShmMapDiscoveryInstance : encode shm map failed\n");
	}

	return status;
}

celix_status_t discovery_updateSHMServices(discovery_pt discovery, endpoint_description_pt endpoint, bool addService)
{
	celix_status_t status = CELIX_SUCCESS;

	if ((discovery->shmId < 0) || (discovery->shmBaseAdress == NULL))
			{
		printf("DISCOVERY : shared memory not initialized.\n");
		status = CELIX_BUNDLE_EXCEPTION;
	}
	else
	{
		ipc_shmData_pt shmData = (ipc_shmData_pt) discovery->shmBaseAdress;

		if ((status = discovery_lock(shmData->semId, 0)) == CELIX_SUCCESS)
		{
			hash_map_pt regDiscoveryInstances = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);

			if ((status = discovery_decShmMap(discovery, &(shmData->data[0]), regDiscoveryInstances)) != CELIX_SUCCESS)
			{
				printf("DISCOVERY : discovery_registerSHMService : decoding data to Properties failed\n");
			}
			else
			{
				char *uuid = NULL;
				bundleContext_getProperty(discovery->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);

				hash_map_pt ownFramework = hashMap_get(regDiscoveryInstances, uuid);

				if (ownFramework == NULL)
				{
					ownFramework = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
					hashMap_put(regDiscoveryInstances, uuid, ownFramework);
				}

				hash_map_pt ownServices = hashMap_get(ownFramework, DISCOVERY_SHM_FW_SERVICES);

				if (ownServices == NULL)
				{
					ownServices = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
					hashMap_put(ownFramework, DISCOVERY_SHM_FW_SERVICES, ownServices);
				}

				// check whether we want to add or remove a service
				if (addService == true)
				{
					// add service
					hash_map_pt newService = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);

					// we need to make a copy of the properties
					properties_pt endpointProperties = properties_create();
					hash_map_iterator_pt epItr = hashMapIterator_create(endpoint->properties);

					while (hashMapIterator_hasNext(epItr) == true)
					{
						hash_map_entry_pt epEntry = hashMapIterator_nextEntry(epItr);
						properties_set(endpointProperties, (char*) hashMapEntry_getKey(epEntry), (char*) hashMapEntry_getValue(epEntry));
					}

					hashMapIterator_destroy(epItr);

					hashMap_put(newService, DISCOVERY_SHM_SRVC_PROPERTIES, endpointProperties);
					hashMap_put(ownServices, endpoint->service, newService);
				}
				else
				{
					printf("remove Services %s\n", endpoint->service);

					hashMap_remove(ownServices, endpoint->service);

					// check if other services are exported, otherwise remove framework/pid as well
					// this is also important to ensure a correct reference-counting (we assume that a discovery bundle crashed if we can find
					// the structure but the process with the pid does not live anymore)
					if (hashMap_size(ownServices) == 0)
							{
						printf("removing framework w/ uuid %s\n", uuid);

						hashMap_remove(ownFramework, DISCOVERY_SHM_FW_SERVICES);
						hashMap_remove(regDiscoveryInstances, uuid);
					}

				}

				// write back to shm
				char* encShmMemStr = NULL;

				if ((status = discovery_encShmMap(discovery, regDiscoveryInstances, &encShmMemStr)) == CELIX_SUCCESS)
				{
					strcpy(&(shmData->data[0]), encShmMemStr);
				}
				else
				{
					printf("DISCOVERY : discovery_registerSHMService : encoding data from HashMap failed\n");
				}
			}

			hashMap_destroy(regDiscoveryInstances, false, false);

			discovery_unlock(shmData->semId, 0);
			discovery_broadcast(shmData->semId, 1);
		}
	}
	return status;
}

celix_status_t discovery_registerSHMService(discovery_pt discovery, endpoint_description_pt endpoint)
{
	return discovery_updateSHMServices(discovery, endpoint, true);
}

celix_status_t discovery_deregisterSHMService(discovery_pt discovery, endpoint_description_pt endpoint)
{
	return discovery_updateSHMServices(discovery, endpoint, false);
}

celix_status_t discovery_endpointAdded(void *handle, endpoint_description_pt endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Endpoint for %s, with filter \"%s\" added\n", endpoint->service, machtedFilter);
	discovery_pt discovery = handle;

	if (status == CELIX_SUCCESS) {

		if ((status = discovery_registerSHMService(discovery, endpoint)) != CELIX_SUCCESS)
		{
			printf("DISCOVERY: Error registering service (%s) within shared memory \n", endpoint->service);
		}

		arrayList_add(discovery->registered, endpoint);
	}

	return status;
}

celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_pt endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Endpoint for %s, with filter \"%s\" removed\n", endpoint->service, machtedFilter);

	discovery_pt discovery = handle;

	if (status == CELIX_SUCCESS) {
		status = discovery_deregisterSHMService(discovery, endpoint);
		int i;
		for (i = 0; i < arrayList_size(discovery->registered); i++) {
			char *url = arrayList_get(discovery->registered, i);
			if (strcmp(url, endpoint->service) == 0) {
				arrayList_remove(discovery->registered, i);
				free(url);
			}
		}
	}

	return status;
}

celix_status_t discovery_endpointListenerAdding(void * handle, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	bundleContext_getService(discovery->context, reference, service);

	return status;
}

celix_status_t discovery_endpointListenerAdded(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	service_registration_pt registration = NULL;
	serviceReference_getServiceRegistration(reference, &registration);
	properties_pt serviceProperties = NULL;
	serviceRegistration_getProperties(registration, &serviceProperties);
	char *discoveryListener = properties_get(serviceProperties, "DISCOVERY");

	if (discoveryListener != NULL && strcmp(discoveryListener, "true") == 0) {
		printf("DISCOVERY: EndpointListener Ignored - Discovery listener\n");
	} else {
		discovery_updateEndpointListener(discovery, reference, (endpoint_listener_pt) service);
	}

	return status;
}

celix_status_t discovery_endpointListenerModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	discovery_updateEndpointListener(discovery, reference, (endpoint_listener_pt) service);

	return status;
}

celix_status_t discovery_updateEndpointListener(discovery_pt discovery, service_reference_pt reference, endpoint_listener_pt service) {
	celix_status_t status = CELIX_SUCCESS;
	char *scope = "createScopeHere";

	array_list_pt scopes = hashMap_get(discovery->listenerReferences, reference);
	if (scopes == NULL) {
		scopes = NULL;
		arrayList_create(&scopes);
		hashMap_put(discovery->listenerReferences, reference, scopes);
	}

	if (!arrayList_contains(scopes, scope)) {
		arrayList_add(scopes, scope);
	}

	hash_map_iterator_pt fwIter = hashMapIterator_create(discovery->shmServices);

	while (hashMapIterator_hasNext(fwIter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(fwIter);
		hash_map_pt fwServices = hashMapEntry_getValue(entry);

		hash_map_iterator_pt fwServicesIter = hashMapIterator_create(fwServices);

		while (hashMapIterator_hasNext(fwServicesIter))
		{
			endpoint_description_pt value = (endpoint_description_pt) hashMapIterator_nextValue(fwServicesIter);
			discovery_informListener(discovery, service, value);
		}
		hashMapIterator_destroy(fwServicesIter);
	}
	hashMapIterator_destroy(fwIter);

	return status;
}

celix_status_t discovery_endpointListenerRemoved(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	discovery_pt discovery = handle;

	printf("DISCOVERY: EndpointListener Removed\n");
	hashMap_remove(discovery->listenerReferences, reference);

	return status;
}

celix_status_t discovery_createOrAttachShm(discovery_pt discovery)
{
	celix_status_t status = CELIX_SUCCESS;

	key_t shmKey = ftok(DISCOVERY_SHM_FILENAME, DISCOVERY_SHM_FTOK_ID);

	if ((discovery->shmId = shmget(shmKey, DISCOVERY_SHM_MEMSIZE, 0666)) < 0)
			{
		printf("DISCOVERY : Could not attach to shared memory. Trying to create shared memory segment. \n");

		// trying to create shared memory
		if ((discovery->shmId = shmget(shmKey, DISCOVERY_SHM_MEMSIZE, IPC_CREAT | 0666)) < 0)
				{
			printf("DISCOVERY : Creation of shared memory segment failed\n");
			status = CELIX_BUNDLE_EXCEPTION;
		}
		else if ((discovery->shmBaseAdress = shmat(discovery->shmId, 0, 0)) == (char*) -1)
				{
			printf("DISCOVERY : Attaching of shared memory segment failed\n");
			status = CELIX_BUNDLE_EXCEPTION;
		}
		else
		{
			int semId = -1;
			key_t semKey = -1;
			ipc_shmData_pt shmData = NULL;
			printf("DISCOVERY : Shared memory segment successfully created at %p\n", discovery->shmBaseAdress);

			// create structure
			shmData = calloc(1, sizeof(*shmData));
			semKey = ftok(DISCOVERY_SEM_FILENAME, DISCOVERY_SEM_FTOK_ID);

			if ((semId = semget(semKey, 2, 0666 | IPC_CREAT)) == -1)
					{
				printf("DISCOVERY : Creation of semaphores failed %i\n", semId);
			}
			else
			{
				// set
				if (semctl(semId, 0, SETVAL, (int) 1) < 0)
						{
					printf(" DISCOVERY : error while initializing semaphore 0 \n");
				}
				else
				{
					printf(" DISCOVERY : semaphore 0 initialized w/ %d\n", semctl(semId, 0, GETVAL, 0));
				}

				if (semctl(semId, 1, SETVAL, (int) 0) < 0)
						{
					printf(" DISCOVERY : error while initializing semaphore 1\n");
				}
				else
				{
					printf(" DISCOVERY : semaphore 1 initialized w/ %d\n", semctl(semId, 1, GETVAL, 0));
				}

				shmData->semId = semId;
				shmData->numListeners = 1;
				printf(" numListeners is initalized: %d \n", shmData->numListeners);

				memcpy(discovery->shmBaseAdress, shmData, sizeof(*shmData));
			}

			free(shmData);
		}
	}
	else if ((discovery->shmBaseAdress = shmat(discovery->shmId, 0, 0)) < 0)
			{
		printf("DISCOVERY : Attaching of shared memory segment failed\n");
		status = CELIX_BUNDLE_EXCEPTION;
	}
	else
	{
		ipc_shmData_pt shmData = (ipc_shmData_pt) discovery->shmBaseAdress;

		discovery_lock(shmData->semId, 0);
		shmData->numListeners++;
		discovery_unlock(shmData->semId, 0);
		discovery_updateLocalSHMServices(discovery);
	}

	return status;
}

celix_status_t discovery_stopOrDetachShm(discovery_pt discovery)
{
	celix_status_t status = CELIX_SUCCESS;

	if ((discovery->shmId < 0) || (discovery->shmBaseAdress == NULL))
			{
		printf("DISCOVERY : discovery_addNewEntry : shared memory not initialized.\n");
		status = CELIX_BUNDLE_EXCEPTION;
	}
	else
	{
		int listener = 0;
		ipc_shmData_pt shmData = (ipc_shmData_pt) discovery->shmBaseAdress;

		discovery_lock(shmData->semId, 0);
		shmData->numListeners--;
		printf(" numListeners decreased: %d \n", shmData->numListeners);
		discovery_unlock(shmData->semId, 0);

		if (shmData->numListeners > 0)
				{
			printf("DISCOVERY: Detaching from Shared memory\n");
			shmdt(discovery->shmBaseAdress);
		}
		else
		{
			printf("DISCOVERY: Removing semaphore w/ id \n");
			ipc_shmData_pt shmData = (ipc_shmData_pt) discovery->shmBaseAdress;
			semctl(shmData->semId, 0, IPC_RMID);
			printf("DISCOVERY: Detaching from Shared memory\n");
			shmdt(discovery->shmBaseAdress);
			printf("DISCOVERY: Destroying Shared memory\n");
			shmctl(discovery->shmId, IPC_RMID, 0);
		}
	}

	return status;
}

celix_status_t discovery_updateLocalSHMServices(discovery_pt discovery)
{
	celix_status_t status = CELIX_SUCCESS;
	ipc_shmData_pt shmData = (ipc_shmData_pt) discovery->shmBaseAdress;

	if ((status = discovery_lock(shmData->semId, 0)) != CELIX_SUCCESS)
	{
		printf("cannot acquire semaphore");
	}
	else
	{
		hash_map_pt regDiscoveryInstances = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);

		if ((status = discovery_decShmMap(discovery, &(shmData->data[0]), regDiscoveryInstances)) == CELIX_SUCCESS)
		{
			hash_map_iterator_pt regDiscoveryInstancesItr = hashMapIterator_create(regDiscoveryInstances);

			while (hashMapIterator_hasNext(regDiscoveryInstancesItr) == true)
			{
				hash_map_entry_pt regDiscoveryEntry = hashMapIterator_nextEntry(regDiscoveryInstancesItr);

				char* uuid = hashMapEntry_getKey(regDiscoveryEntry);
				hash_map_pt fwAttr = hashMapEntry_getValue(regDiscoveryEntry);
				hash_map_pt services = hashMap_get(fwAttr, DISCOVERY_SHM_FW_SERVICES);

				/* check for new services */
				hash_map_iterator_pt srvcItr = hashMapIterator_create(services);

				while (hashMapIterator_hasNext(srvcItr) == true)
				{
					hash_map_entry_pt srvc = hashMapIterator_nextEntry(srvcItr);

					char *srvcName = hashMapEntry_getKey(srvc);
					hash_map_pt srvcAttr = hashMapEntry_getValue(srvc);
					hash_map_pt fwServices = NULL;

					// check whether we have a service from that fw at all
					if ((fwServices = hashMap_get(discovery->shmServices, uuid)) == NULL)
					{
						fwServices = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
						hashMap_put(discovery->shmServices, uuid, fwServices);
					}

					if (hashMap_get(fwServices, srvcName) != NULL)
					{
						printf("DISCOVERY : discovery_updateLocalSHMServices : service with url %s from %s already registered", srvcName, uuid);
					}
					else
					{
						endpoint_description_pt endpoint = apr_palloc(discovery->pool, sizeof(*endpoint));
						endpoint->id = apr_pstrdup(discovery->pool, srvcName);
						endpoint->serviceId = 42;
						endpoint->service = apr_pstrdup(discovery->pool, srvcName);
						endpoint->properties = (properties_pt) hashMap_get(srvcAttr, DISCOVERY_SHM_SRVC_PROPERTIES);
						endpoint->frameworkUUID = uuid;

						discovery_addService(discovery, endpoint);
						hashMap_put(fwServices, srvcName, endpoint);
					}
				}

				hashMapIterator_destroy(srvcItr);

				/* check for obsolete services for this uuid */
				hash_map_pt fwServices = hashMap_get(discovery->shmServices, uuid);
				hash_map_iterator_pt shmServicesItr = hashMapIterator_create(fwServices);

				// iterate over frameworks
				while (hashMapIterator_hasNext(shmServicesItr) == true)
				{
					hash_map_entry_pt shmService = hashMapIterator_nextEntry(shmServicesItr);
					char *fwurl = hashMapEntry_getKey(shmService);

					if (hashMap_get(services, fwurl) == NULL)
					{
						printf("DISCOVERY: service with url %s from %s already unregistered", fwurl, uuid);
						endpoint_description_pt endpoint = hashMap_get(fwServices, fwurl);
						discovery_removeService(discovery, endpoint);
						hashMap_remove(fwServices, fwurl);
					}
				}
				hashMapIterator_destroy(shmServicesItr);
			}
			hashMapIterator_destroy(regDiscoveryInstancesItr);

			/* check for obsolete frameworks*/
			hash_map_iterator_pt lclFwItr = hashMapIterator_create(discovery->shmServices);

			// iterate over frameworks
			while (hashMapIterator_hasNext(lclFwItr) == true)
			{
				hash_map_entry_pt lclFwEntry = hashMapIterator_nextEntry(lclFwItr);
				char *fwUUID = hashMapEntry_getKey(lclFwEntry);

				// whole framework not available any more
				if (hashMap_get(regDiscoveryInstances, fwUUID) == NULL)
				{
					hash_map_pt lclFwServices = NULL;

					if ((lclFwServices = (hash_map_pt) hashMapEntry_getValue(lclFwEntry)) == NULL)
					{
						printf("UUID %s does not have any services, but a structure\n", fwUUID);
					}
					else
					{
						// remove them all
						hash_map_iterator_pt lclFwServicesItr = hashMapIterator_create(lclFwServices);

						while (hashMapIterator_hasNext(lclFwServicesItr) == true)
						{
							hash_map_entry_pt lclFwSrvcEntry = hashMapIterator_nextEntry(lclFwServicesItr);

							discovery_removeService(discovery, (endpoint_description_pt) hashMapEntry_getValue(lclFwSrvcEntry));
							hashMapIterator_remove(lclFwServicesItr);
						}

						hashMapIterator_destroy(lclFwServicesItr);
					}
				}
			}

			hashMapIterator_destroy(lclFwItr);

		}
		hashMap_destroy(regDiscoveryInstances, false, false);
		discovery_unlock(shmData->semId, 0);
	}

	return status;
}

static void *APR_THREAD_FUNC discovery_pollSHMServices(apr_thread_t *thd, void *data)
{
	discovery_pt discovery = data;
	celix_status_t status = CELIX_SUCCESS;

	if ((discovery->shmId < 0) || (discovery->shmBaseAdress == NULL))
	{
		printf( "DISCOVERY: shared memory not initialized.");
		status = CELIX_BUNDLE_EXCEPTION;
	}
	else
	{
		ipc_shmData_pt shmData = (ipc_shmData_pt) discovery->shmBaseAdress;

		while(discovery->running == true)
		{
			if(((status = discovery_lock(shmData->semId, 1)) != CELIX_SUCCESS) && (discovery->running == true))
			{
				printf( "DISCOVERY: cannot acquire semaphore. Breaking main poll cycle.");
				break;
			}
			else
			{
				discovery_updateLocalSHMServices(discovery);
			}
		}
	}

	apr_thread_exit(thd, (status == CELIX_SUCCESS) ? APR_SUCCESS : -1);

	return NULL;
}
