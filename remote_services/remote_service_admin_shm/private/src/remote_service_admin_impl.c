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
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>
#include <unistd.h>
#include <apr_uuid.h>
#include <apr_strings.h>

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

celix_status_t remoteServiceAdmin_installEndpoint(remote_service_admin_pt admin, export_registration_pt registration, service_reference_pt reference, char *interface);
celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_pt admin, properties_pt serviceProperties, properties_pt endpointProperties, char *interface, endpoint_description_pt *description);

celix_status_t remoteServiceAdmin_lock(int semId, int semNr);
celix_status_t remoteServiceAdmin_unlock(int semId, int semNr);
celix_status_t remoteServiceAdmin_wait(int semId, int semNr);


celix_status_t remoteServiceAdmin_createOrAttachShm(hash_map_pt ipcSegment, remote_service_admin_pt admin, endpoint_description_pt endpointDescription, bool createIfNotFound);
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
        (*admin)->importedServices = hashMap_create(NULL, NULL, NULL, NULL);
        (*admin)->exportedIpcSegment = hashMap_create(NULL, NULL, NULL, NULL);
        (*admin)->importedIpcSegment = hashMap_create(NULL, NULL, NULL, NULL);
        (*admin)->pollThread = hashMap_create(NULL, NULL, NULL, NULL);
        (*admin)->pollThreadRunning =  hashMap_create(NULL, NULL, NULL, NULL);
    }

    return status;
}




celix_status_t remoteServiceAdmin_stop(remote_service_admin_pt admin)
{
    celix_status_t status = CELIX_SUCCESS;

    hash_map_iterator_pt iter = hashMapIterator_create(admin->importedServices);
    while (hashMapIterator_hasNext(iter))
    {
        array_list_pt exports = hashMapIterator_nextValue(iter);
        int i;
        for (i = 0; i < arrayList_size(exports); i++)
        {
            import_registration_pt export = arrayList_get(exports, i);
            importRegistration_stopTracking(export);
        }
    }

    // set stop-thread-variable
    iter = hashMapIterator_create(admin->pollThreadRunning);
    while (hashMapIterator_hasNext(iter))
    {
        bool *pollThreadRunning = hashMapIterator_nextValue(iter);
        *pollThreadRunning = false;
    }

    // release lock
    iter = hashMapIterator_create(admin->exportedIpcSegment);
    while (hashMapIterator_hasNext(iter))
    {
        ipc_segment_pt ipc = hashMapIterator_nextValue(iter);
        remoteServiceAdmin_unlock(ipc->semId, 0);
    }

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

    iter = hashMapIterator_create(admin->importedIpcSegment);
    while (hashMapIterator_hasNext(iter))
    {
        ipc_segment_pt ipc = hashMapIterator_nextValue(iter);
        shmdt(ipc->shmBaseAdress);
    }


    iter = hashMapIterator_create(admin->exportedIpcSegment);
    while (hashMapIterator_hasNext(iter))
    {
        ipc_segment_pt ipc = hashMapIterator_nextValue(iter);

        semctl(ipc->semId, 1 /*ignored*/, IPC_RMID);
        shmctl(ipc->shmId, IPC_RMID, 0);
    }

    return status;
}


celix_status_t remoteServiceAdmin_lock(int semId, int semNr)
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


celix_status_t remoteServiceAdmin_unlock(int semId, int semNr)
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

celix_status_t remoteServiceAdmin_wait(int semId, int semNr)
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
    }
    while (semOpStatus == -1 && errno == EINTR);

    return status;
}


celix_status_t remoteServiceAdmin_send(remote_service_admin_pt admin, endpoint_description_pt recpEndpoint, char *method, char *data, char **reply, int *replyStatus)
{

    celix_status_t status = CELIX_SUCCESS;
    ipc_segment_pt ipc = NULL;

    if ((ipc = hashMap_get(admin->importedIpcSegment, recpEndpoint->service)) != NULL)
    {
        hash_map_pt fncCallProps = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
        char *encFncCallProps = NULL;
        int semid = ipc->semId;

        /* write method and data */
        hashMap_put(fncCallProps, RSA_FUNCTIONCALL_METHOD_PROPERTYNAME, method);
        hashMap_put(fncCallProps, RSA_FUNCTIONCALL_DATA_PROPERTYNAME, data);

        if ((status = netstring_encodeFromHashMap(admin->pool, fncCallProps, &encFncCallProps)) == CELIX_SUCCESS)
        {
            char *fncCallReply = NULL;
            char *fncCallReplyStatus = CELIX_SUCCESS;

            strcpy(ipc->shmBaseAdress, encFncCallProps);

            remoteServiceAdmin_unlock(semid, 0);//sem0: 0 -> 1
            remoteServiceAdmin_wait(semid, 2);//sem2: when 0 continue
            remoteServiceAdmin_lock(semid, 1);//sem1: 1 -> 0

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
            // wait on semaphore 3 //sem3: when 0 continue
            if ((remoteServiceAdmin_wait(ipc->semId, 3) != CELIX_SUCCESS))
            {
                printf("RSA : Error waiting on semaphore 3");
            }
            // acquire READ semaphore //sem0: 1 -> 0
            else if (((status = remoteServiceAdmin_lock(ipc->semId, 0)) == CELIX_SUCCESS) && (*pollThreadRunning == true))
            {
                hash_map_pt receivedMethodCall = hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);

                if ((status = netstring_decodeToHashMap(admin->pool, ipc->shmBaseAdress, receivedMethodCall)) != CELIX_SUCCESS)
                {
                    printf("DISCOVERY : receiveFromSharedMemory : decoding data to Properties\n");
                }
                else
                {
                    char *method = hashMap_get(receivedMethodCall, RSA_FUNCTIONCALL_METHOD_PROPERTYNAME);
                    char *data = hashMap_get(receivedMethodCall, RSA_FUNCTIONCALL_DATA_PROPERTYNAME);

                    if (method == NULL)
                    {
                        printf("RSA : receiveFromSharedMemory : no method found. \n");
                    }
                    else
                    {
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

                                    celix_status_t replyStatus = export->endpoint->handleRequest(export->endpoint->endpoint, method, data, &reply);
                                    hashMap_put(receivedMethodCall, RSA_FUNCTIONCALL_RETURNSTATUS_PROPERTYNAME, apr_itoa(admin->pool, replyStatus));

                                    if (reply != NULL)
                                    {
                                        hashMap_put(receivedMethodCall, RSA_FUNCTIONCALL_RETURN_PROPERTYNAME, reply);

                                        // write back
                                        if ((status = netstring_encodeFromHashMap(admin->pool, receivedMethodCall, &encReply)) == CELIX_SUCCESS)
                                        {

                                            if ((strlen(encReply) * sizeof(char)) >= RSA_SHM_MEMSIZE)
                                            {
                                                printf("RSA : receiveFromSharedMemory : size of message bigger than shared memory message. NOT SENDING.\n");
                                            }
                                            else
                                            {
                                                strcpy(ipc->shmBaseAdress, encReply);
                                            }
                                        }
                                        else
                                        {
                                            printf("RSA : receiveFromSharedMemory : encoding of reply failed\n");
                                        }
                                    }
                                }
                                else
                                {
                                    printf("RSA : receiveFromSharedMemory : No endpoint set for %s\n", export->endpointDescription->service );
                                }
                            }
                        }

                        remoteServiceAdmin_unlock(ipc->semId, 1); //sem1: 0 -> 1
                    }
                }
                hashMap_destroy(receivedMethodCall, false, false);
            }
        }
    }
    apr_thread_exit(thd, (status == CELIX_SUCCESS) ? APR_SUCCESS : -1);

    return NULL;
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
    }

    if (reference == NULL)
    {
        printf("ERROR: expected a reference for service id %s\n", serviceId);
        return CELIX_ILLEGAL_STATE;
    }

    serviceReference_getServiceRegistration(reference, &registration);
    properties_pt serviceProperties = NULL;
    serviceRegistration_getProperties(registration, &serviceProperties);
    char *exports = properties_get(serviceProperties, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES);
    char *provided = properties_get(serviceProperties, (char *) OSGI_FRAMEWORK_OBJECTCLASS);

    if (exports == NULL || provided == NULL)
    {
        printf("RSA: No Services to export.\n");
    }
    else
    {
        printf("RSA: Export services (%s)\n", exports);
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

                exportRegistration_create(admin->pool, reference, NULL, admin, admin->context, &registration);

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
        remoteServiceAdmin_unlock(ipc->semId, 0);

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

                hashMap_remove(admin->pollThreadRunning, registration->endpointDescription);
                hashMap_remove(admin->exportedIpcSegment, registration->endpointDescription->service);
                hashMap_remove(admin->pollThread, registration->endpointDescription);
            }
        }
    }


    return status;
}


celix_status_t remoteServiceAdmin_createOrAttachShm(hash_map_pt ipcSegment, remote_service_admin_pt admin, endpoint_description_pt endpointDescription, bool createIfNotFound)
{
    celix_status_t status = CELIX_SUCCESS;

    apr_pool_t *pool = admin->pool;
    apr_pool_t *shmPool = NULL;

    apr_status_t aprStatus = 0;

    /* setup ipc sehment */
    ipc_segment_pt ipc = NULL;

    properties_pt endpointProperties = endpointDescription->properties;

    char *shmPath = NULL;
    char *shmFtokId = NULL;

    char *semPath = NULL;
    char *semFtokId = NULL;


    if ((shmPath = properties_get(endpointProperties, (char *) RSA_SHM_PATH_PROPERTYNAME)) == NULL)
    {
        printf("RSA : No value found for key %s in endpointProperties\n", RSA_SHM_PATH_PROPERTYNAME);
        status = CELIX_BUNDLE_EXCEPTION;
    }
    else if ((shmFtokId = properties_get(endpointProperties, (char *) RSA_SHM_FTOK_ID_PROPERTYNAME)) == NULL)
    {
        printf("RSA : No value found for key %s in endpointProperties\n", RSA_SHM_FTOK_ID_PROPERTYNAME);
        status = CELIX_BUNDLE_EXCEPTION;
    }
    else if ((semPath = properties_get(endpointProperties, (char *) RSA_SEM_PATH_PROPERTYNAME)) == NULL)
    {
        printf("RSA : No value found for key %s in endpointProperties\n", RSA_SEM_PATH_PROPERTYNAME);
        status = CELIX_BUNDLE_EXCEPTION;
    }
    else if ((semFtokId = properties_get(endpointProperties, (char *) RSA_SEM_FTOK_ID_PROPERTYNAME)) == NULL)
    {
        printf("RSA : No value found for key %s in endpointProperties\n", RSA_SEM_FTOK_ID_PROPERTYNAME);
        status = CELIX_BUNDLE_EXCEPTION;
    }
    else
    {
        key_t shmKey = ftok(shmPath, atoi(shmFtokId));
        ipc = apr_palloc(admin->pool, sizeof(*ipc));

        if ((ipc->shmId = shmget(shmKey, RSA_SHM_MEMSIZE, 0666)) < 0)
        {
            printf("RSA : Could not attach to shared memory\n");

            if (createIfNotFound == true)
            {
                if ((ipc->shmId = shmget(shmKey, RSA_SHM_MEMSIZE, IPC_CREAT | 0666)) < 0)
                {
                    printf("RSA : Creation of shared memory segment failed\n");
                    status = CELIX_BUNDLE_EXCEPTION;
                }
                else if ((ipc->shmBaseAdress = shmat(ipc->shmId, 0, 0)) == (char *) - 1 )
                {
                    printf("RSA : Attaching of shared memory segment failed\n");
                    status = CELIX_BUNDLE_EXCEPTION;
                }
                else
                {
                    printf("RSA : shared memory segment sucessfully created at %p \n", ipc->shmBaseAdress);
                }
            }
        }
        else if ((ipc->shmBaseAdress = shmat(ipc->shmId, 0, 0)) == (char *) - 1 )
        {
            printf("RSA : Attaching of shared memory segment failed\n");
            status = CELIX_BUNDLE_EXCEPTION;
        }
        else
        {
            printf("RSA : sucessfully attached to shared memory at %p \n", ipc->shmBaseAdress);
        }
    }

    if ((status == CELIX_SUCCESS) && (ipc != NULL))
    {
        key_t semkey = ftok(semPath, atoi(semFtokId));
        int semflg = (createIfNotFound == true) ? (0666 | IPC_CREAT) : (0666);
        int semid = semget(semkey, 4, semflg);

        if (semid != -1)
        {
            // only reset semaphores if a create was supposed
            if ((createIfNotFound == true) && ((semctl (semid, 0, SETVAL, (int) 0) == -1) || (semctl (semid, 1, SETVAL, (int) 0) == -1) || (semctl (semid, 2, SETVAL, (int) 0) == -1) || (semctl (semid, 3, SETVAL, (int) 0) == -1)))
            {
                printf("RSA : error while initialize semaphores \n");
            }

            printf("RSA : semaphores w/ key %s and id %i added \n", endpointDescription->service, semid);
            ipc->semId = semid;

            hashMap_put(ipcSegment, endpointDescription->service, ipc);
        }
        else
        {
            printf("RSA : error getting semaphores.\n");
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
    char *serviceId = (char *) hashMap_remove(endpointProperties, (void *) OSGI_FRAMEWORK_SERVICE_ID);
    properties_set(endpointProperties, (char *) OSGI_FRAMEWORK_OBJECTCLASS, interface);
    properties_set(endpointProperties, (char *) OSGI_RSA_ENDPOINT_SERVICE_ID, serviceId);

    char *uuid = NULL;
    bundleContext_getProperty(admin->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
    properties_set(endpointProperties, (char *) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
    properties_set(endpointProperties, (char *) OSGI_RSA_SERVICE_LOCATION, apr_pstrdup(admin->pool, interface));
    if (properties_get(endpointProperties, (char *) RSA_SHM_PATH_PROPERTYNAME) == NULL)
    {
        properties_set(endpointProperties, (char *) RSA_SHM_PATH_PROPERTYNAME, apr_pstrdup(admin->pool, (char *) RSA_SHM_DEFAULTPATH));
    }
    if (properties_get(endpointProperties, (char *) RSA_SHM_FTOK_ID_PROPERTYNAME) == NULL)
    {
        properties_set(endpointProperties, (char *) RSA_SHM_FTOK_ID_PROPERTYNAME, apr_pstrdup(admin->pool, (char *) RSA_SHM_DEFAULT_FTOK_ID));
    }
    if (properties_get(endpointProperties, (char *) RSA_SEM_PATH_PROPERTYNAME) == NULL)
    {
        properties_set(endpointProperties, (char *) RSA_SEM_PATH_PROPERTYNAME, apr_pstrdup(admin->pool, RSA_SEM_DEFAULTPATH));
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
        char *uuid = NULL;
        status = bundleContext_getProperty(admin->context, OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, &uuid);
        if (status == CELIX_SUCCESS)
        {
            (*description)->properties = endpointProperties;
            (*description)->frameworkUUID = uuid;
            (*description)->serviceId = apr_atoi64(properties_get(serviceProperties, (char *) OSGI_FRAMEWORK_SERVICE_ID));
            (*description)->id = properties_get(endpointProperties, (char *) OSGI_RSA_SERVICE_LOCATION);
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

    printf("RSA: Import service %s\n", endpointDescription->service);

    importRegistration_create(admin->pool, endpointDescription, admin, admin->context, registration);

    array_list_pt importedRegs = hashMap_get(admin->importedServices, endpointDescription);
    if (importedRegs == NULL)
    {
        arrayList_create(&importedRegs);
        hashMap_put(admin->importedServices, endpointDescription, importedRegs);

    }
    arrayList_add(importedRegs, *registration);

    importRegistration_open(*registration);
    importRegistration_startTracking(*registration);
    remoteServiceAdmin_createOrAttachShm(admin->importedIpcSegment, admin, endpointDescription, false);

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

