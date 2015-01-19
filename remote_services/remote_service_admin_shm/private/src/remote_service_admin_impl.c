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
 * remote_service_admin_impl.c
 *
 *  \date       Sep 14, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <apr_uuid.h>
#include <apr_strings.h>
#include <apr_thread_proc.h>

#include "remote_service_admin_shm.h"
#include "remote_service_admin_shm_impl.h"

#include "export_registration_impl.h"
#include "import_registration_impl.h"
#include "remote_constants.h"
#include "constants.h"
#include "utils.h"
#include "bundle_context.h"
#include "bundle.h"
#include "service_reference.h"
#include "service_registration.h"
#include "netstring.h"


static celix_status_t remoteServiceAdmin_lock(int semId, int semNr);
static celix_status_t remoteServiceAdmin_unlock(int semId, int semNr);
static int remoteServiceAdmin_getCount(int semId, int semNr);

celix_status_t remoteServiceAdmin_installEndpoint(remote_service_admin_pt admin, export_registration_pt registration, service_reference_pt reference, char *interface);
celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_pt admin, properties_pt serviceProperties, properties_pt endpointProperties, char *interface, endpoint_description_pt *description);

celix_status_t remoteServiceAdmin_createOrAttachShm(hash_map_pt ipcSegment, remote_service_admin_pt admin, endpoint_description_pt endpointDescription, bool createIfNotFound);
celix_status_t remoteServiceAdmin_getIpcSegment(remote_service_admin_pt admin, endpoint_description_pt endpointDescription, ipc_segment_pt* ipc);
celix_status_t remoteServiceAdmin_detachIpcSegment(ipc_segment_pt ipc);
celix_status_t remoteServiceAdmin_deleteIpcSegment(ipc_segment_pt ipc);

celix_status_t remoteServiceAdmin_getSharedIdentifierFile(char *fwUuid, char* servicename, char* outFile);
celix_status_t remoteServiceAdmin_removeSharedIdentityFile(char *fwUuid, char* servicename);
celix_status_t remoteServiceAdmin_removeSharedIdentityFiles(char* fwUuid);


celix_status_t remoteServiceAdmin_create(apr_pool_t *pool, bundle_context_pt context, remote_service_admin_pt *admin)
{
    celix_status_t status = CELIX_SUCCESS;

    *admin = apr_palloc(pool, sizeof(**admin));
    if (!*admin)
    {
        status = CELIX_ENOMEM;
    }
    else
    {
        (*admin)->pool = pool;
        (*admin)->context = context;
        (*admin)->exportedServices = hashMap_create(NULL, NULL, NULL, NULL);
        (*admin)->importedServices = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
        (*admin)->exportedIpcSegment = hashMap_create(NULL, NULL, NULL, NULL);
        (*admin)->importedIpcSegment = hashMap_create(NULL, NULL, NULL, NULL);
        (*admin)->pollThread = hashMap_create(NULL, NULL, NULL, NULL);
        (*admin)->pollThreadRunning =  hashMap_create(NULL, NULL, NULL, NULL);
    }

    return status;
}


celix_status_t remoteServiceAdmin_destroy(remote_service_admin_pt admin)
{
	celix_status_t status = CELIX_SUCCESS;

	hashMap_destroy(admin->exportedServices, false, false);
	hashMap_destroy(admin->importedServices, false, false);
	hashMap_destroy(admin->exportedIpcSegment, false, false);
	hashMap_destroy(admin->importedIpcSegment, false, false);
	hashMap_destroy(admin->pollThread, false, false);
	hashMap_destroy(admin->pollThreadRunning, false, false);

	return status;
}


celix_status_t remoteServiceAdmin_stop(remote_service_admin_pt admin)
{
    celix_status_t status = CELIX_SUCCESS;

    hash_map_iterator_pt iter = hashMapIterator_create(admin->importedServices);
    while (hashMapIterator_hasNext(iter))
    {
    	import_registration_factory_pt importFactory = hashMapIterator_nextValue(iter);
        int i;
        for (i = 0; i < arrayList_size(importFactory->registrations); i++)
        {
            import_registration_pt importRegistration = arrayList_get(importFactory->registrations, i);

			if (importFactory->trackedFactory != NULL)
			{
				importFactory->trackedFactory->unregisterProxyService(importFactory->trackedFactory->factory, importRegistration->endpointDescription);
			}
        }

        serviceTracker_close(importFactory->proxyFactoryTracker);
        importRegistrationFactory_close(importFactory);
    }
    hashMapIterator_destroy(iter);

    // set stop-thread-variable
    iter = hashMapIterator_create(admin->pollThreadRunning);
    while (hashMapIterator_hasNext(iter))
    {
        bool *pollThreadRunning = hashMapIterator_nextValue(iter);
        *pollThreadRunning = false;
    }
    hashMapIterator_destroy(iter);

    // release lock
    iter = hashMapIterator_create(admin->exportedIpcSegment);
    while (hashMapIterator_hasNext(iter))
    {
        ipc_segment_pt ipc = hashMapIterator_nextValue(iter);
        remoteServiceAdmin_unlock(ipc->semId, 1);
    }
    hashMapIterator_destroy(iter);

    // wait till threads has stopped
    iter = hashMapIterator_create(admin->pollThread);
    while (hashMapIterator_hasNext(iter))
    {
        apr_status_t tstat;
        apr_thread_t *pollThread = hashMapIterator_nextValue(iter);

        if (pollThread != NULL)
        {
            apr_status_t stat = apr_thread_join(&tstat, pollThread);

            if (stat != APR_SUCCESS && tstat != APR_SUCCESS)
            {
                status = CELIX_BUNDLE_EXCEPTION;
            }
        }
    }
    hashMapIterator_destroy(iter);

    iter = hashMapIterator_create(admin->importedIpcSegment);
    while (hashMapIterator_hasNext(iter))
    {
        ipc_segment_pt ipc = hashMapIterator_nextValue(iter);
        shmdt(ipc->shmBaseAdress);
    }
    hashMapIterator_destroy(iter);


    iter = hashMapIterator_create(admin->exportedIpcSegment);
    while (hashMapIterator_hasNext(iter))
    {
        ipc_segment_pt ipc = hashMapIterator_nextValue(iter);

        semctl(ipc->semId, 1 /*ignored*/, IPC_RMID);
        shmctl(ipc->shmId, IPC_RMID, 0);
    }
    hashMapIterator_destroy(iter);
    
    char *fwUuid = NULL;
    bundleContext_getProperty(admin->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &fwUuid);

    remoteServiceAdmin_removeSharedIdentityFiles(fwUuid);

    return status;
}


static int remoteServiceAdmin_getCount(int semId, int semNr)
{
	int token = -1;
	unsigned short semVals[3];
	union semun semArg;

	semArg.array = semVals;

	if(semctl(semId,0,GETALL,semArg) == 0)
	{
		token = semArg.array[semNr];
	}

	return token;
}


static celix_status_t remoteServiceAdmin_lock(int semId, int semNr)
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
    }
    while (semOpStatus == -1 && errno == EINTR);


    return status;
}


static celix_status_t remoteServiceAdmin_unlock(int semId, int semNr)
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
    }
    while (semOpStatus == -1 && errno == EINTR);

    return status;
}


celix_status_t remoteServiceAdmin_send(remote_service_admin_pt admin, endpoint_description_pt recpEndpoint, char *data, char **reply, int *replyStatus)
{

    celix_status_t status = CELIX_SUCCESS;
    ipc_segment_pt ipc = NULL;

    if ((ipc = hashMap_get(admin->importedIpcSegment, recpEndpoint->service)) != NULL)
    {
        hash_map_pt fncCallProps = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
        char *encFncCallProps = NULL;
        int semid = ipc->semId;

        /* write method and data */
        hashMap_put(fncCallProps, RSA_FUNCTIONCALL_DATA_PROPERTYNAME, data);

        if ((status = netstring_encodeFromHashMap(admin->pool, fncCallProps, &encFncCallProps)) == CELIX_SUCCESS)
        {
            char *fncCallReply = NULL;
            char *fncCallReplyStatus = CELIX_SUCCESS;

            /* lock critical area */
            remoteServiceAdmin_lock(semid, 0);

			strcpy(ipc->shmBaseAdress, encFncCallProps);

			/* Check the status of the send-receive semaphore and reset them if not correct */
			if (remoteServiceAdmin_getCount(ipc->semId,1) > 0)
			{
				semctl(semid, 1, SETVAL, (int) 0);
			}
			if (remoteServiceAdmin_getCount(ipc->semId,2) > 0)
			{
				semctl(semid, 2, SETVAL, (int) 0);
			}

			/* Inform receiver we are invoking the remote service */
			remoteServiceAdmin_unlock(semid, 1);

			/* Wait until the receiver finished his operations */
			remoteServiceAdmin_lock(semid, 2);

            if ((status = netstring_decodeToHashMap(admin->pool, ipc->shmBaseAdress, fncCallProps)) == CELIX_SUCCESS)
            {
                fncCallReply = hashMap_get(fncCallProps, RSA_FUNCTIONCALL_RETURN_PROPERTYNAME);
                fncCallReplyStatus = hashMap_get(fncCallProps, RSA_FUNCTIONCALL_RETURNSTATUS_PROPERTYNAME);
            }

            if (fncCallReply != NULL)
            {
            	*reply = strdup(fncCallReply);
            }

            if (fncCallReplyStatus != NULL)
            {
            	*replyStatus = apr_atoi64(fncCallReplyStatus);
            }

            /* release critical area */
            remoteServiceAdmin_unlock(semid, 0);
        }
        hashMap_destroy(fncCallProps, false, false);
    }
    else
    {
        status = CELIX_ILLEGAL_STATE;       /* could not find ipc segment */
    }

    return status;
}


static void *APR_THREAD_FUNC remoteServiceAdmin_receiveFromSharedMemory(apr_thread_t *thd, void *data)
{
    celix_status_t status = CELIX_SUCCESS;
    recv_shm_thread_pt thread_data = data;

    remote_service_admin_pt admin = thread_data->admin;
    endpoint_description_pt exportedEndpointDesc = thread_data->endpointDescription;

    ipc_segment_pt ipc;

    if ((ipc = hashMap_get(admin->exportedIpcSegment, exportedEndpointDesc->service)) != NULL)
    {
        celix_status_t status = CELIX_SUCCESS;
        bool *pollThreadRunning = hashMap_get(admin->pollThreadRunning, exportedEndpointDesc);

        while (*pollThreadRunning == true)
        {
            if (((status = remoteServiceAdmin_lock(ipc->semId, 1)) == CELIX_SUCCESS) && (*pollThreadRunning == true))
            	{
				apr_pool_t *pool = NULL;
				hash_map_pt receivedMethodCall = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);

				if (apr_pool_create(&pool, admin->pool) != APR_SUCCESS)
				{
					status = CELIX_BUNDLE_EXCEPTION;
				}
				else if ((status = netstring_decodeToHashMap(pool, ipc->shmBaseAdress, receivedMethodCall)) != CELIX_SUCCESS)
				{
					fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "RSA : receiveFromSharedMemory : decoding data to Properties");
				}
				else
                {
                    char *data = hashMap_get(receivedMethodCall, RSA_FUNCTIONCALL_DATA_PROPERTYNAME);

					hash_map_iterator_pt iter = hashMapIterator_create(admin->exportedServices);

					while (hashMapIterator_hasNext(iter))
					{
						hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
						array_list_pt exports = hashMapEntry_getValue(entry);
						int expIt = 0;

						for (expIt = 0; expIt < arrayList_size(exports); expIt++)
						{
							export_registration_pt export = arrayList_get(exports, expIt);

							if ( (strcmp(exportedEndpointDesc->service, export->endpointDescription->service) == 0) && (export->endpoint != NULL))
							{
								char *reply = NULL;
								char *encReply = NULL;

								celix_status_t replyStatus = export->endpoint->handleRequest(export->endpoint->endpoint, data, &reply);
								hashMap_put(receivedMethodCall, RSA_FUNCTIONCALL_RETURNSTATUS_PROPERTYNAME, apr_itoa(admin->pool, replyStatus));

								if (reply != NULL)
								{
									hashMap_put(receivedMethodCall, RSA_FUNCTIONCALL_RETURN_PROPERTYNAME, reply);

									// write back
									if ((status = netstring_encodeFromHashMap(admin->pool, receivedMethodCall, &encReply)) == CELIX_SUCCESS)
									{

										if ((strlen(encReply) * sizeof(char)) >= RSA_SHM_MEMSIZE)
										{
											fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA : receiveFromSharedMemory : size of message bigger than shared memory message. NOT SENDING.");
										}
										else
										{
											strcpy(ipc->shmBaseAdress, encReply);
										}
									}
									else
									{
										fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA : receiveFromSharedMemory : encoding of reply failed");
									}
								}
							}
							else
							{
								fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA : receiveFromSharedMemory : No endpoint set for %s.", export->endpointDescription->service );
							}
						}
					}
					hashMapIterator_destroy(iter);


                }
				hashMap_destroy(receivedMethodCall, false, false);
				remoteServiceAdmin_unlock(ipc->semId, 2);

				apr_pool_destroy(pool);
            }
        }
    }
    apr_thread_exit(thd, (status == CELIX_SUCCESS) ? APR_SUCCESS : -1);

    return NULL;
}



celix_status_t remoteServiceAdmin_getSharedIdentifierFile(char *fwUuid, char* servicename, char* outFile) {
	celix_status_t status = CELIX_SUCCESS;
	snprintf(outFile, RSA_FILEPATH_LENGTH, "%s/%s/%s", P_tmpdir, fwUuid, servicename);

	if (access(outFile, F_OK) != 0)
	{
		char tmpDir[RSA_FILEPATH_LENGTH];

		snprintf(tmpDir, sizeof(tmpDir), "%s/%s", P_tmpdir, fwUuid);

		// we call, even if it already exists (and just don't care about the return value)
		mkdir(tmpDir, 0755);

		if (fopen(outFile, "wb") == NULL)
		{
			fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA: error while creating shared identifier file %s (%s)", outFile, strerror(errno));
				status = CELIX_FILE_IO_EXCEPTION;
		}
		else
		{
			fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "RSA:create shared identifier file %s", outFile);
		}
	}
	else
	{
		fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "RSA: shared identifier file %s already exists", outFile);
	}

	return status;
}

celix_status_t remoteServiceAdmin_removeSharedIdentityFile(char *fwUuid, char* servicename) {
	celix_status_t status = CELIX_SUCCESS;
	char tmpPath[RSA_FILEPATH_LENGTH];

	snprintf(tmpPath, sizeof(tmpPath), "%s/%s/%s", P_tmpdir, fwUuid, servicename);

	if (access(tmpPath, F_OK) == 0)
	{
		fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "RSA: removing shared identifier file %s", tmpPath);
		unlink(tmpPath);
	}
	else
	{
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA: cannot remove shared identifier file %s", tmpPath);
	}

	return status;
}

celix_status_t remoteServiceAdmin_removeSharedIdentityFiles(char* fwUuid) {
	char tmpDir[RSA_FILEPATH_LENGTH];

	snprintf(tmpDir, sizeof(tmpDir), "%s/%s", P_tmpdir, fwUuid);

	DIR *d = opendir(tmpDir);
	size_t path_len = strlen(tmpDir);
	int retVal = 0;


	if (d)
	{
		struct dirent *p;

		while (!retVal && (p = readdir(d)))
		{
			char* f_name;
			size_t len;

			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, ".."))
			{
				continue;
			}

			len = path_len + strlen(p->d_name) + 2;
			f_name = (char*) calloc(len, 1);

			if (f_name)
			{
				struct stat statbuf;

				snprintf(f_name, len, "%s/%s", tmpDir, p->d_name);

				if (!stat(f_name, &statbuf))
				{
						fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "RSA: removing shared identifier file %s (unproper clean-up?)", f_name);
						retVal = unlink(f_name);
				}
			}
			free(f_name);
		}
	}

	closedir(d);

	if (!retVal)
	{
		rmdir(tmpDir);
	}

	return retVal;
}




celix_status_t remoteServiceAdmin_exportService(remote_service_admin_pt admin, char *serviceId, properties_pt properties, array_list_pt *registrations)
{
    celix_status_t status = CELIX_SUCCESS;
    arrayList_create(registrations);

    array_list_pt references = NULL;
    service_reference_pt reference = NULL;
    service_registration_pt registration = NULL;

    apr_pool_t *tmpPool = NULL;

    apr_pool_create(&tmpPool, admin->pool);
    if (tmpPool == NULL)
    {
        return CELIX_ENOMEM;
    }
    else
    {
        char *filter = apr_pstrcat(admin->pool, "(", (char *) OSGI_FRAMEWORK_SERVICE_ID, "=", serviceId, ")", NULL); /*FIXME memory leak*/
        bundleContext_getServiceReferences(admin->context, NULL, filter, &references);
        apr_pool_destroy(tmpPool);
        if (arrayList_size(references) >= 1)
        {
            reference = arrayList_get(references, 0);
        }
        arrayList_destroy(references);
    }

    if (reference == NULL)
    {
    	fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "ERROR: expected a reference for service id %s.", serviceId);
        return CELIX_ILLEGAL_STATE;
    }

    serviceReference_getServiceRegistration(reference, &registration);
    properties_pt serviceProperties = NULL;
    serviceRegistration_getProperties(registration, &serviceProperties);
    char *exports = properties_get(serviceProperties, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES);
    char *provided = properties_get(serviceProperties, (char *) OSGI_FRAMEWORK_OBJECTCLASS);

    if (exports == NULL || provided == NULL)
    {
    	fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "RSA: No Services to export.");
    }
    else
    {
    	fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "RSA: Export services (%s).", exports);
        array_list_pt interfaces = NULL;
        arrayList_create(&interfaces);
        if (strcmp(utils_stringTrim(exports), "*") == 0)
        {
            char *token;
           char *interface = apr_strtok(provided, ",", &token);

            while (interface != NULL)
            {
                arrayList_add(interfaces, utils_stringTrim(interface));
                interface = apr_strtok(NULL, ",", &token);
            }
        }
        else
        {
            char *exportToken;
            char *providedToken;

           char *pinterface = apr_strtok(provided, ",", &providedToken);
            while (pinterface != NULL)
            {
                char *einterface = apr_strtok(exports, ",", &exportToken);
                while (einterface != NULL)
                {
                    if (strcmp(einterface, pinterface) == 0)
                    {
                        arrayList_add(interfaces, einterface);
                    }
                    einterface = apr_strtok(NULL, ",", &exportToken);
                }
                pinterface = apr_strtok(NULL, ",", &providedToken);
            }
        }

        if (arrayList_size(interfaces) != 0)
        {
            int iter = 0;
            for (iter = 0; iter < arrayList_size(interfaces); iter++)
            {
                char *interface = arrayList_get(interfaces, iter);
                export_registration_pt registration = NULL;

                // TODO: add logHelper reference
                exportRegistration_create(admin->pool, NULL, reference, NULL, admin, admin->context, &registration);

                arrayList_add(*registrations, registration);

                remoteServiceAdmin_installEndpoint(admin, registration, reference, interface);
                exportRegistration_open(registration);
                exportRegistration_startTracking(registration);

                if ( remoteServiceAdmin_createOrAttachShm(admin->exportedIpcSegment, admin, registration->endpointDescription, true) == CELIX_SUCCESS)
                {
                    recv_shm_thread_pt recvThreadData = NULL;

                    if ((recvThreadData = apr_palloc(admin->pool, sizeof(*recvThreadData))) == NULL)
                    {
                        status = CELIX_ENOMEM;
                    }
                    else
                    {
                        recvThreadData->admin = admin;
                        recvThreadData->endpointDescription = registration->endpointDescription;

                        apr_thread_t *pollThread = NULL;
                        bool *pollThreadRunningPtr = apr_palloc(admin->pool, sizeof(*pollThreadRunningPtr));
                        *pollThreadRunningPtr  = true;

                        hashMap_put(admin->pollThreadRunning, registration->endpointDescription, pollThreadRunningPtr);

                        // start receiving thread
                        apr_thread_create(&pollThread, NULL, remoteServiceAdmin_receiveFromSharedMemory, recvThreadData, (admin)->pool);

                        hashMap_put(admin->pollThread, registration->endpointDescription, pollThread);
                    }
                }
            }

            hashMap_put(admin->exportedServices, reference, *registrations);
        }
        arrayList_destroy(interfaces);
    }

    return status;
}


celix_status_t remoteServiceAdmin_removeExportedService(export_registration_pt registration)
{
    celix_status_t status = CELIX_SUCCESS;
    remote_service_admin_pt admin = registration->rsa;
    bool *pollThreadRunning = NULL;
    ipc_segment_pt ipc = NULL;
    apr_thread_t *pollThread = NULL;

    hashMap_remove(admin->exportedServices, registration->reference);

    if ((pollThreadRunning = hashMap_get(admin->pollThreadRunning, registration->endpointDescription)) != NULL)
    {
        *pollThreadRunning = false;
    }

    if ((ipc = hashMap_get(admin->exportedIpcSegment, registration->endpointDescription->service)) != NULL)
    {
        remoteServiceAdmin_unlock(ipc->semId, 1);

        if ( (pollThread = hashMap_get(admin->pollThread, registration->endpointDescription)) != NULL)
        {
            apr_status_t tstat;
            apr_status_t stat = apr_thread_join(&tstat, pollThread);

            if (stat != APR_SUCCESS && tstat != APR_SUCCESS)
            {
                status = CELIX_BUNDLE_EXCEPTION;
            }
            else
            {
                semctl(ipc->semId, 1 /*ignored*/, IPC_RMID);
                shmctl(ipc->shmId, IPC_RMID, 0);

                remoteServiceAdmin_removeSharedIdentityFile(registration->endpointDescription->frameworkUUID, registration->endpointDescription->service);

                hashMap_remove(admin->pollThreadRunning, registration->endpointDescription);
                hashMap_remove(admin->exportedIpcSegment, registration->endpointDescription->service);
                hashMap_remove(admin->pollThread, registration->endpointDescription);
            }
        }
    }

    return status;
}



celix_status_t remoteServiceAdmin_getIpcSegment(remote_service_admin_pt admin, endpoint_description_pt endpointDescription, ipc_segment_pt* ipc)
{
        (*ipc) = (hashMap_containsKey(admin->importedIpcSegment, endpointDescription) == true) ? hashMap_get(admin->importedIpcSegment, endpointDescription) : NULL;

        return (*ipc != NULL) ? CELIX_SUCCESS : CELIX_ILLEGAL_ARGUMENT;
}

celix_status_t remoteServiceAdmin_detachIpcSegment(ipc_segment_pt ipc)
{
        return (shmdt(ipc->shmBaseAdress) != -1) ? CELIX_SUCCESS : CELIX_BUNDLE_EXCEPTION;
}

celix_status_t remoteServiceAdmin_deleteIpcSegment(ipc_segment_pt ipc)
{
        return ((semctl(ipc->semId, 1 /*ignored*/, IPC_RMID) != -1) && (shmctl(ipc->shmId, IPC_RMID, 0) != -1)) ? CELIX_SUCCESS : CELIX_BUNDLE_EXCEPTION;
}




celix_status_t remoteServiceAdmin_createOrAttachShm(hash_map_pt ipcSegment, remote_service_admin_pt admin, endpoint_description_pt endpointDescription, bool createIfNotFound)
{
    celix_status_t status = CELIX_SUCCESS;

    /* setup ipc sehment */
    ipc_segment_pt ipc = NULL;

    properties_pt endpointProperties = endpointDescription->properties;

    char *shmPath = NULL;
    char *shmFtokId = NULL;

    char *semPath = NULL;
    char *semFtokId = NULL;


    if ((shmPath = properties_get(endpointProperties, (char *) RSA_SHM_PATH_PROPERTYNAME)) == NULL)
    {
    	fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "RSA : No value found for key %s in endpointProperties.", RSA_SHM_PATH_PROPERTYNAME);
        status = CELIX_BUNDLE_EXCEPTION;
    }
    else if ((shmFtokId = properties_get(endpointProperties, (char *) RSA_SHM_FTOK_ID_PROPERTYNAME)) == NULL)
    {
    	fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "RSA : No value found for key %s in endpointProperties.", RSA_SHM_FTOK_ID_PROPERTYNAME);
        status = CELIX_BUNDLE_EXCEPTION;
    }
    else if ((semPath = properties_get(endpointProperties, (char *) RSA_SEM_PATH_PROPERTYNAME)) == NULL)
    {
    	fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "RSA : No value found for key %s in endpointProperties.", RSA_SEM_PATH_PROPERTYNAME);
        status = CELIX_BUNDLE_EXCEPTION;
    }
    else if ((semFtokId = properties_get(endpointProperties, (char *) RSA_SEM_FTOK_ID_PROPERTYNAME)) == NULL)
    {
    	fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "RSA : No value found for key %s in endpointProperties.", RSA_SEM_FTOK_ID_PROPERTYNAME);
        status = CELIX_BUNDLE_EXCEPTION;
    }
    else
    {
        key_t shmKey = ftok(shmPath, atoi(shmFtokId));
        ipc = apr_palloc(admin->pool, sizeof(*ipc));

        if ((ipc->shmId = shmget(shmKey, RSA_SHM_MEMSIZE, 0666)) < 0)
        {
        	fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "RSA : Could not attach to shared memory");

            if (createIfNotFound == true)
            {
                if ((ipc->shmId = shmget(shmKey, RSA_SHM_MEMSIZE, IPC_CREAT | 0666)) < 0)
                {
                	fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA : Creation of shared memory segment failed.");
                    status = CELIX_BUNDLE_EXCEPTION;
                }
                else if ((ipc->shmBaseAdress = shmat(ipc->shmId, 0, 0)) == (char *) - 1 )
                {
                	fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA : Attaching of shared memory segment failed.");
                    status = CELIX_BUNDLE_EXCEPTION;
                }
                else
                {
                	fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "RSA : shared memory segment sucessfully created at %p.", ipc->shmBaseAdress);
                }
            }
        }
        else if ((ipc->shmBaseAdress = shmat(ipc->shmId, 0, 0)) == (char *) - 1 )
        {
        	fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA : Attaching to shared memory segment failed.");
            status = CELIX_BUNDLE_EXCEPTION;
        }
        else
        {
        	fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "RSA : sucessfully attached to shared memory at %p.", ipc->shmBaseAdress);
        }
    }

    if ((status == CELIX_SUCCESS) && (ipc != NULL))
    {
        key_t semkey = ftok(semPath, atoi(semFtokId));
        int semflg = (createIfNotFound == true) ? (0666 | IPC_CREAT) : (0666);
        int semid = semget(semkey, 3, semflg);

        if (semid != -1)
        {
			// only reset semaphores if a create was supposed
			if ((createIfNotFound == true) && ((semctl(semid, 0, SETVAL, (int) 1) == -1) || (semctl(semid, 1, SETVAL, (int) 0) == -1) || (semctl(semid, 2, SETVAL, (int) 0) == -1))) {
				fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA : error while initialize semaphores.");
			}

			fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "RSA : semaphores w/ key %s and id %i added.", endpointDescription->service, semid);
            ipc->semId = semid;

            hashMap_put(ipcSegment, endpointDescription->service, ipc);
        }
        else
        {
        	fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA : error getting semaphores.");
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    return status;
}



celix_status_t remoteServiceAdmin_installEndpoint(remote_service_admin_pt admin, export_registration_pt registration, service_reference_pt reference, char *interface)
{
    celix_status_t status = CELIX_SUCCESS;
    properties_pt endpointProperties = properties_create();
    properties_pt serviceProperties = NULL;

    service_registration_pt sRegistration = NULL;
    serviceReference_getServiceRegistration(reference, &sRegistration);

    serviceRegistration_getProperties(sRegistration, &serviceProperties);

    hash_map_iterator_pt iter = hashMapIterator_create(serviceProperties);
    while (hashMapIterator_hasNext(iter))
    {
        hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
        char *key = (char *) hashMapEntry_getKey(entry);
        char *value = (char *) hashMapEntry_getValue(entry);

        properties_set(endpointProperties, key, value);
    }
	hashMapIterator_destroy(iter);

    char *serviceId = (char *) hashMap_remove(endpointProperties, (void *) OSGI_FRAMEWORK_SERVICE_ID);
    properties_set(endpointProperties, (char *) OSGI_FRAMEWORK_OBJECTCLASS, interface);
    properties_set(endpointProperties, (char *) OSGI_RSA_ENDPOINT_SERVICE_ID, serviceId);

    char *uuid = NULL;
    bundleContext_getProperty(admin->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
    properties_set(endpointProperties, (char *) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
    properties_set(endpointProperties, (char *) OSGI_RSA_SERVICE_LOCATION, apr_pstrdup(admin->pool, interface));
	if (properties_get(endpointProperties, (char *) RSA_SHM_PATH_PROPERTYNAME) == NULL)
	{
		char sharedIdentifierFile[RSA_FILEPATH_LENGTH];

		if (remoteServiceAdmin_getSharedIdentifierFile(uuid, interface, sharedIdentifierFile) == CELIX_SUCCESS)
		{
			properties_set(endpointProperties, (char *) RSA_SHM_PATH_PROPERTYNAME, sharedIdentifierFile);
		} else
		{
			properties_set(endpointProperties, (char *) RSA_SHM_PATH_PROPERTYNAME, apr_pstrdup(admin->pool, (char *) RSA_SHM_DEFAULTPATH));
		}
	}
    if (properties_get(endpointProperties, (char *) RSA_SHM_FTOK_ID_PROPERTYNAME) == NULL)
    {
        properties_set(endpointProperties, (char *) RSA_SHM_FTOK_ID_PROPERTYNAME, apr_pstrdup(admin->pool, (char *) RSA_SHM_DEFAULT_FTOK_ID));
    }
	if (properties_get(endpointProperties, (char *) RSA_SEM_PATH_PROPERTYNAME) == NULL)
	{
		char sharedIdentifierFile[RSA_FILEPATH_LENGTH];

		if (remoteServiceAdmin_getSharedIdentifierFile(uuid, interface, sharedIdentifierFile) == CELIX_SUCCESS)
		{
			properties_set(endpointProperties, (char *) RSA_SEM_PATH_PROPERTYNAME, sharedIdentifierFile);
		} else
		{
			properties_set(endpointProperties, (char *) RSA_SEM_PATH_PROPERTYNAME, apr_pstrdup(admin->pool, (char *) RSA_SEM_DEFAULTPATH));
		}
	}
    if (properties_get(endpointProperties, (char *) RSA_SEM_FTOK_ID_PROPERTYNAME) == NULL)
    {
        properties_set(endpointProperties, (char *) RSA_SEM_FTOK_ID_PROPERTYNAME, apr_pstrdup(admin->pool, (char *) RSA_SEM_DEFAULT_FTOK_ID));
    }

    endpoint_description_pt endpointDescription = NULL;
    remoteServiceAdmin_createEndpointDescription(admin, serviceProperties, endpointProperties, interface, &endpointDescription);
    exportRegistration_setEndpointDescription(registration, endpointDescription);

    return status;
}

celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_pt admin, properties_pt serviceProperties,
        properties_pt endpointProperties, char *interface, endpoint_description_pt *description)
{
    celix_status_t status = CELIX_SUCCESS;

    apr_pool_t *childPool = NULL;
    apr_pool_create(&childPool, admin->pool);

    *description = apr_palloc(childPool, sizeof(*description));

    if (!*description)
    {
        status = CELIX_ENOMEM;
    }
    else
    {
        if (status == CELIX_SUCCESS)
        {
            (*description)->properties = endpointProperties;
            (*description)->frameworkUUID = properties_get(endpointProperties, (char*) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID);
            (*description)->serviceId = apr_atoi64(properties_get(serviceProperties, (char *) OSGI_FRAMEWORK_SERVICE_ID));
            (*description)->id = properties_get(endpointProperties, (char*) OSGI_RSA_ENDPOINT_ID);
            (*description)->service = interface;
        }
    }

    return status;
}

celix_status_t remoteServiceAdmin_getExportedServices(remote_service_admin_pt admin, array_list_pt *services)
{
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t remoteServiceAdmin_getImportedEndpoints(remote_service_admin_pt admin, array_list_pt *services)
{
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t remoteServiceAdmin_importService(remote_service_admin_pt admin, endpoint_description_pt endpointDescription, import_registration_pt *registration)
{
    celix_status_t status = CELIX_SUCCESS;

    fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "RSA: Import service %s.", endpointDescription->service);

    import_registration_factory_pt registration_factory = (import_registration_factory_pt) hashMap_get(admin->importedServices, endpointDescription->service);

	// check whether we already have a registration_factory
	if (registration_factory == NULL)
	{
	    // TODO: Add logHelper
		importRegistrationFactory_install(admin->pool, NULL, endpointDescription->service, admin->context, &registration_factory);
		hashMap_put(admin->importedServices, endpointDescription->service, registration_factory);
	}

	 // factory available
	if (status != CELIX_SUCCESS || (registration_factory->trackedFactory == NULL))
	{
		fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "RSA: no proxyFactory available.");
	}
	else
	{
		// we create an importRegistration per imported service
		importRegistration_create(admin->pool, endpointDescription, admin, (sendToHandle) &remoteServiceAdmin_send, admin->context, registration);
		registration_factory->trackedFactory->registerProxyService(registration_factory->trackedFactory->factory,  endpointDescription, admin, (sendToHandle) &remoteServiceAdmin_send);

		arrayList_add(registration_factory->registrations, *registration);
		remoteServiceAdmin_createOrAttachShm(admin->importedIpcSegment, admin, endpointDescription, false);
	}

    return status;
}



celix_status_t remoteServiceAdmin_removeImportedService(remote_service_admin_pt admin, import_registration_pt registration)
{
    celix_status_t status = CELIX_BUNDLE_EXCEPTION;

    if (registration != NULL)
    {
		ipc_segment_pt ipc = NULL;
		endpoint_description_pt endpointDescription = (endpoint_description_pt) registration->endpointDescription;
		import_registration_factory_pt registration_factory = (import_registration_factory_pt) hashMap_get(admin->importedServices, endpointDescription->service);

        // detach from IPC
        if(remoteServiceAdmin_getIpcSegment(admin, endpointDescription, &ipc) != CELIX_SUCCESS)
        {
        	fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA: Error while retrieving IPC segment for imported service %s.", endpointDescription->service);
        }
        else if (remoteServiceAdmin_detachIpcSegment(ipc) != CELIX_SUCCESS)
        {
        	fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA: Error while detaching IPC segment for imported service %s.", endpointDescription->service);
        }

		// factory available
		if ((registration_factory == NULL) || (registration_factory->trackedFactory == NULL))
		{
			fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "RSA: Error while retrieving registration factory for imported service %s.", endpointDescription->service);
		}
		else
		{
			registration_factory->trackedFactory->unregisterProxyService(registration_factory->trackedFactory->factory, endpointDescription);
			arrayList_removeElement(registration_factory->registrations, registration);
			importRegistration_destroy(registration);

			if (arrayList_isEmpty(registration_factory->registrations) == true)
			{
				fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "RSA: closing proxy");

				serviceTracker_close(registration_factory->proxyFactoryTracker);
				importRegistrationFactory_close(registration_factory);

				hashMap_remove(admin->importedServices, endpointDescription->service);
				importRegistrationFactory_destroy(&registration_factory);
			}
		}
    }

	return status;
}



celix_status_t exportReference_getExportedEndpoint(export_reference_pt reference, endpoint_description_pt *endpoint)
{
    celix_status_t status = CELIX_SUCCESS;

    *endpoint = reference->endpoint;

    return status;
}

celix_status_t exportReference_getExportedService(export_reference_pt reference)
{
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t importReference_getImportedEndpoint(import_reference_pt reference)
{
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

celix_status_t importReference_getImportedService(import_reference_pt reference)
{
    celix_status_t status = CELIX_SUCCESS;
    return status;
}

