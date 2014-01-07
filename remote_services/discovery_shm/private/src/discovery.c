

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

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "bundle_context.h"
#include "array_list.h"
#include "utils.h"
#include "celix_errno.h"
#include "filter.h"
#include "service_reference.h"
#include "service_registration.h"
#include "netstring.h"

#include "discovery.h"

struct discovery {
	bundle_context_pt context;
	apr_pool_t *pool;

	hash_map_pt listenerReferences;

	bool running;

	int  shmId;
	void *shmBaseAdress;
	apr_thread_t *shmPollThread;

	hash_map_pt shmServices;

	array_list_pt handled;
	array_list_pt registered;
};

celix_status_t discovery_informListener(discovery_pt discovery, endpoint_listener_pt listener, endpoint_description_pt endpoint);
celix_status_t discovery_informListenerOfRemoval(discovery_pt discovery, endpoint_listener_pt listener, endpoint_description_pt endpoint);

celix_status_t discovery_addService(discovery_pt discovery, endpoint_description_pt endpoint);
celix_status_t discovery_removeService(discovery_pt discovery, endpoint_description_pt endpoint);

static void *APR_THREAD_FUNC discovery_pollSHMServices(apr_thread_t *thd, void *data);
celix_status_t discovery_lock(int semId, int semNr);
celix_status_t discovery_unlock(int semId, int semNr);
celix_status_t discovery_wait(int semId, int semNr);

celix_status_t discovery_updateLocalSHMServices(discovery_pt discovery);
celix_status_t discovery_updateSHMServices(discovery_pt discovery, char *serviceName, char *nsEncAttributes);

celix_status_t discovery_registerSHMService(discovery_pt discovery, char *url, char *attributes);
celix_status_t discovery_deregisterSHMService(discovery_pt discovery, char *serviceName);
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
		(*discovery)->handled = NULL;
		arrayList_create(&(*discovery)->handled);
		(*discovery)->registered = NULL;
		arrayList_create(&(*discovery)->registered);

		(*discovery)->shmId = -1;
		(*discovery)->shmBaseAdress = NULL;

		if ((status = discovery_createOrAttachShm(*discovery)) != CELIX_SUCCESS)
		{
			printf("DISCOVERY: Shared Memory initialization failed.\n");
		}
		else
		{
			apr_thread_create(&(*discovery)->shmPollThread, NULL, discovery_pollSHMServices, *discovery, (*discovery)->pool);
		}
	}

	return status;
}

celix_status_t discovery_stop(discovery_pt discovery) {
	celix_status_t status = CELIX_SUCCESS;

	apr_status_t tstat;
	discovery->running = false;

	ipc_shmData_pt shmData = (ipc_shmData_pt) discovery->shmBaseAdress;

	if (shmData != NULL)
	{
		discovery_unlock(shmData->semId, 1);
		discovery_wait(shmData->semId, 2);
		discovery_lock(shmData->semId, 1);

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
				char *serviceName = arrayList_get(discovery->registered, i);
				printf("DISCOVERY: deregistering service %s.\n", serviceName);
				status = discovery_deregisterSHMService(discovery, serviceName);
			}

			//discovery_lock(shmData->semId, 1);

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
		bundleContext_getService(discovery->context, reference, (void**)&listener);
		discovery_informListenerOfRemoval(discovery, listener, endpoint);
	}

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
		filter_pt filter = filter_create(scope, discovery->pool);
		bool matchResult = false;
		filter_match(filter, endpoint->properties, &matchResult);
		if (matchResult) {
			printf("DISCOVERY: Add service (%s)\n", endpoint->service);
			bundleContext_getService(discovery->context, reference, (void**)&listener);
			discovery_informListener(discovery, listener, endpoint);
		}
	}

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

	semOperation.sem_num=semNr;
	semOperation.sem_op=-1;
	semOperation.sem_flg=0;

	do
	{
		status = CELIX_SUCCESS;

		if ((semOpStatus = semop(semId, &semOperation, 1)) != 0)
		{
			status = CELIX_BUNDLE_EXCEPTION;
		}
	} while(semOpStatus == -1 && errno == EINTR);

	return status;
}


celix_status_t discovery_unlock(int semId, int semNr)
{
	celix_status_t status = CELIX_SUCCESS;
	int semOpStatus = 0;
	struct sembuf semOperation;

	semOperation.sem_num=semNr;
	semOperation.sem_op=1;
	semOperation.sem_flg=0;

	do
	{
		status = CELIX_SUCCESS;

		if ((semOpStatus = semop(semId, &semOperation, 1)) != 0)
		{
			status = CELIX_BUNDLE_EXCEPTION;
		}
	} while(semOpStatus == -1 && errno == EINTR);


	return status;
}


celix_status_t discovery_wait(int semId, int semNr)
{
    celix_status_t status = CELIX_SUCCESS;
    int semOpStatus = 0;
    struct sembuf semOperation;

    semOperation.sem_num = semNr;
    semOperation.sem_op = 0;
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



celix_status_t discovery_updateSHMServices(discovery_pt discovery, char *serviceName, char *nsEncAttributes)
{
	celix_status_t status = CELIX_SUCCESS;

	if ((discovery->shmId  < 0) || (discovery->shmBaseAdress == NULL))
	{
		printf("DISCOVERY : shared memory not initialized.\n");
		status = CELIX_BUNDLE_EXCEPTION;
	}
	else
	{
   		ipc_shmData_pt shmData = (ipc_shmData_pt) discovery->shmBaseAdress;

		if((status = discovery_lock(shmData->semId, 0)) == CELIX_SUCCESS)
		{
			hash_map_pt registeredShmServices = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);

			/* get already saved properties */
			if ((status = netstring_decodeToHashMap(discovery->pool, shmData->data, registeredShmServices)) != CELIX_SUCCESS)
			{
				printf("DISCOVERY : discovery_registerSHMService : decoding data to Properties failed\n");
			}
			else
			{
				char *encShmServices = NULL;

				if (nsEncAttributes != NULL)
				{
					hashMap_put(registeredShmServices, serviceName, nsEncAttributes);
				}
				else
				{
					hashMap_remove(registeredShmServices, serviceName);
				}

				// write back
				if ((status = netstring_encodeFromHashMap(discovery->pool, registeredShmServices, &encShmServices)) == CELIX_SUCCESS)
				{
					strcpy(shmData->data, encShmServices);
				}
				else
				{
					printf("DISCOVERY : discovery_registerSHMService : encoding data from HashMap failed\n");
				}
			}

			hashMap_destroy(registeredShmServices, false, false);
			discovery_unlock(shmData->semId, 0);

			/* unlock and afterwards lock to inform all listener */
			discovery_unlock(shmData->semId, 1);
			// wait till notify semaphore is 0 to ensure all threads have performed update routine
			discovery_wait(shmData->semId, 2);
			discovery_lock(shmData->semId, 1);
		}
	}
	return status;
}


celix_status_t discovery_registerSHMService(discovery_pt discovery, char *serviceName, char *nsEncAttributes)
{
	return discovery_updateSHMServices(discovery, serviceName, nsEncAttributes);
}



celix_status_t discovery_deregisterSHMService(discovery_pt discovery, char *serviceName)
{
	return discovery_updateSHMServices(discovery, serviceName, NULL);
}




celix_status_t discovery_endpointAdded(void *handle, endpoint_description_pt endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Endpoint for %s, with filter \"%s\" added\n", endpoint->service, machtedFilter);
	discovery_pt discovery = handle;

	if (status == CELIX_SUCCESS) {

		char *nsEncAttribute;

		netstring_encodeFromHashMap(discovery->pool, (hash_map_pt) endpoint->properties, &nsEncAttribute);

		if ((status = discovery_registerSHMService(discovery, endpoint->service, nsEncAttribute)) != CELIX_SUCCESS)
		{
			printf("DISCOVERY: Error registering service (%s) within shared memory \n", endpoint->service);
		}

		arrayList_add(discovery->registered, strdup(endpoint->service));
	}

	return status;
}


celix_status_t discovery_endpointRemoved(void *handle, endpoint_description_pt endpoint, char *machtedFilter) {
	celix_status_t status = CELIX_SUCCESS;
	printf("DISCOVERY: Endpoint for %s, with filter \"%s\" removed\n", endpoint->service, machtedFilter);

	discovery_pt discovery = handle;
	char *serviceUrl = NULL;

	if (status == CELIX_SUCCESS) {
		status = discovery_deregisterSHMService(discovery, endpoint->service);
		int i;
		for (i = 0; i < arrayList_size(discovery->registered); i++) {
			char *url = arrayList_get(discovery->registered, i);
			if (strcmp(url, endpoint->service) == 0) {
				arrayList_remove(discovery->registered, i);
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

	hash_map_iterator_pt iter = hashMapIterator_create(discovery->shmServices);

	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		char *key = hashMapEntry_getKey(entry);
		endpoint_description_pt value = hashMapEntry_getValue(entry);
		discovery_informListener(discovery, service, value);
	}

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

	key_t shmKey = ftok(DISCOVERY_SHM_FILENAME,  DISCOVERY_SHM_FTOK_ID);

	if ((discovery->shmId = shmget(shmKey, DISCOVERY_SHM_MEMSIZE, 0666)) < 0)
	{
	   	printf("DISCOVERY : Could not attach to shared memory. Trying to create shared memory segment. \n");

	   	// trying to create shared memory
	   	if ((discovery->shmId = shmget(shmKey, DISCOVERY_SHM_MEMSIZE, IPC_CREAT | 0666)) < 0)
	   	{
	   		printf("DISCOVERY : Creation of shared memory segment failed\n");
	   		status = CELIX_BUNDLE_EXCEPTION;
	   	}
	   	else if ((discovery->shmBaseAdress = shmat(discovery->shmId, 0, 0)) == (char*) -1 )
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
	   		shmData = apr_palloc(discovery->pool, sizeof(*shmData));
   			semKey = ftok(DISCOVERY_SEM_FILENAME, DISCOVERY_SEM_FTOK_ID);

			if ((semId = semget(semKey, 3, 0666 | IPC_CREAT)) == -1)
			{
				printf("DISCOVERY : Creation of semaphores failed %i\n", semId);
			}
			else
			{
				// set
				if ( semctl (semId, 0, SETVAL, (int) 1) < 0)
				{
					printf(" DISCOVERY : error while initializing semaphore 0 \n");
				}
				else
				{
					printf(" DISCOVERY : semaphore 0 initialized w/ %d\n", semctl(semId, 0, GETVAL, 0));
				}

				if ( semctl (semId, 1, SETVAL, (int) 0) < 0)
				{
					printf(" DISCOVERY : error while initializing semaphore 1\n");
				}
				else
				{
					printf(" DISCOVERY : semaphore 1 initialized w/ %d\n", semctl(semId, 1, GETVAL, 0));
				}

				if ( semctl (semId, 2, SETVAL, (int) 0) < 0)
				{
					printf(" DISCOVERY : error while initializing semaphore 2\n");
				}
				else
				{
					printf(" DISCOVERY : semaphore 2 initialized w/ %d\n", semctl(semId, 2, GETVAL, 0));
				}

				shmData->semId = semId;
				memcpy(discovery->shmBaseAdress, shmData, sizeof(*shmData));
			}
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

	if ((discovery->shmId  < 0) || (discovery->shmBaseAdress == NULL))
	{
		printf("DISCOVERY : discovery_addNewEntry : shared memory not initialized.\n");
		status = CELIX_BUNDLE_EXCEPTION;
	}
	else
	{
		int listener = 0;
   		ipc_shmData_pt shmData = (ipc_shmData_pt) discovery->shmBaseAdress;

		discovery_lock(shmData->semId, 0);
		listener = shmData->numListeners--;
		discovery_unlock(shmData->semId, 0);

		if (listener  > 0)
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

	if((status = discovery_lock(shmData->semId, 0)) != CELIX_SUCCESS)
	{
		printf("DISCOVERY : discovery_updateLocalSHMServices : cannot acquire semaphore\n");
	}
	else
	{
		hash_map_pt registeredShmServices = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);

		if ((status = netstring_decodeToHashMap(discovery->pool, &(shmData->data[0]), registeredShmServices)) != CELIX_SUCCESS)
		{
			printf("DISCOVERY : discovery_updateLocalSHMServices : decoding data to properties failed\n");
		}
		else
		{
			/* check for new services */
			hash_map_iterator_pt shmPrpItr = hashMapIterator_create(registeredShmServices);

			while(hashMapIterator_hasNext(shmPrpItr) == true)
			{
				hash_map_entry_pt shmPrpEntry = hashMapIterator_nextEntry(shmPrpItr);
				char *serviceName = hashMapEntry_getKey(shmPrpEntry);

				if(hashMap_get(discovery->shmServices, serviceName) != NULL)
				{
					printf("DISCOVERY : discovery_updateLocalSHMServices : service with url %s already registered\n", serviceName );
				}
				else
				{
					hash_map_pt props = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
					char *nsEncEndpointProp = hashMapEntry_getValue(shmPrpEntry);

					if ( (status = netstring_decodeToHashMap(discovery->pool, nsEncEndpointProp, props)) != CELIX_SUCCESS)
					{
						printf("DISCOVERY : discovery_updateLocalSHMServices : Decoding of endpointProperties failed\n");
					}
					else
					{
						endpoint_description_pt endpoint = apr_palloc(discovery->pool, sizeof(*endpoint));
						endpoint->id = apr_pstrdup(discovery->pool, serviceName);
						endpoint->serviceId = 42;
						endpoint->service = apr_pstrdup(discovery->pool, serviceName);
						endpoint->properties = (properties_pt) props;

						discovery_addService(discovery, endpoint);
						hashMap_put(discovery->shmServices, apr_pstrdup(discovery->pool, serviceName), endpoint);
					}
				}
			}

			hashMapIterator_destroy(shmPrpItr);

			/* check for obsolete services */
			hash_map_iterator_pt shmServicesItr = hashMapIterator_create(discovery->shmServices);

			while(hashMapIterator_hasNext(shmServicesItr) == true)
			{
				hash_map_entry_pt shmService = hashMapIterator_nextEntry(shmServicesItr);
				char *url = hashMapEntry_getKey(shmService);

				if(hashMap_get(registeredShmServices, url) == NULL)
				{
					printf("DISCOVERY : discovery_updateLocalSHMServices : service with url %s unregistered\n", url);
					endpoint_description_pt endpoint =  hashMap_get(discovery->shmServices, url);
					discovery_removeService(discovery, endpoint);
					hashMap_remove(discovery->shmServices, url);
				}
			}
			hashMapIterator_destroy(shmServicesItr);
		}

		discovery_unlock(shmData->semId, 0);
		hashMap_destroy(registeredShmServices, false, false);
	}
	return status;
}


static void *APR_THREAD_FUNC discovery_pollSHMServices(apr_thread_t *thd, void *data)
{
	discovery_pt discovery = data;
	celix_status_t status = CELIX_SUCCESS;

	if ((discovery->shmId  < 0) || (discovery->shmBaseAdress == NULL))
	{
		printf("DISCOVERY : discovery_pollSHMServices : shared memory not initialized.\n");
		status = CELIX_BUNDLE_EXCEPTION;
	}
	else
	{
   		ipc_shmData_pt shmData = (ipc_shmData_pt) discovery->shmBaseAdress;

		printf("DISCOVERY : discovery_pollSHMServices thread started\n");

		while(discovery->running == true)
		{
			if(((status = discovery_unlock(shmData->semId, 2)) != CELIX_SUCCESS) && (discovery->running == true))
			{
				printf("DISCOVERY : discovery_pollSHMServices : cannot acquire semaphore\n");
			}
			else if(((status = discovery_lock(shmData->semId, 1)) != CELIX_SUCCESS) && (discovery->running == true))
			{
				printf("DISCOVERY : discovery_pollSHMServices : cannot acquire semaphore\n");
			}
			else
			{
				discovery_updateLocalSHMServices(discovery);

				discovery_lock(shmData->semId, 2);
				discovery_unlock(shmData->semId, 1);
			}

			sleep(1);
		}
	}

	apr_thread_exit(thd, (status == CELIX_SUCCESS) ? APR_SUCCESS : -1);

	return NULL;
}

