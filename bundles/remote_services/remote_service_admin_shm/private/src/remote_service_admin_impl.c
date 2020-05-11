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
 * remote_service_admin_impl.c
 *
 *  \date       Sep 14, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/sem.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <unistd.h>
#include <uuid/uuid.h>

#include "remote_service_admin_shm.h"
#include "remote_service_admin_shm_impl.h"

#include "export_registration_impl.h"
#include "import_registration_impl.h"
#include "remote_constants.h"
#include "celix_constants.h"
#include "utils.h"
#include "bundle_context.h"
#include "bundle.h"
#include "service_reference.h"
#include "service_registration.h"

static celix_status_t remoteServiceAdmin_lock(int semId, int semNr);
static celix_status_t remoteServiceAdmin_unlock(int semId, int semNr);
static int remoteServiceAdmin_getCount(int semId, int semNr);

celix_status_t remoteServiceAdmin_installEndpoint(remote_service_admin_t *admin, export_registration_t *registration, service_reference_pt reference, char *interface);
celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_t *admin, service_reference_pt reference, celix_properties_t *endpointProperties, char *interface, endpoint_description_t **description);

celix_status_t remoteServiceAdmin_createOrAttachShm(hash_map_pt ipcSegment, remote_service_admin_t *admin, endpoint_description_t *endpointDescription, bool createIfNotFound);
celix_status_t remoteServiceAdmin_getIpcSegment(remote_service_admin_t *admin, endpoint_description_t *endpointDescription, ipc_segment_pt* ipc);
celix_status_t remoteServiceAdmin_detachIpcSegment(ipc_segment_pt ipc);
celix_status_t remoteServiceAdmin_deleteIpcSegment(ipc_segment_pt ipc);

celix_status_t remoteServiceAdmin_getSharedIdentifierFile(remote_service_admin_t *admin, char *fwUuid, char* servicename, char* outFile);
celix_status_t remoteServiceAdmin_removeSharedIdentityFile(remote_service_admin_t *admin, char *fwUuid, char* servicename);
celix_status_t remoteServiceAdmin_removeSharedIdentityFiles(remote_service_admin_t *admin);

celix_status_t remoteServiceAdmin_create(celix_bundle_context_t *context, remote_service_admin_t **admin) {
	celix_status_t status = CELIX_SUCCESS;

	*admin = calloc(1, sizeof(**admin));

	if (!*admin) {
		status = CELIX_ENOMEM;
	} else {
		(*admin)->context = context;
		(*admin)->exportedServices = hashMap_create(NULL, NULL, NULL, NULL);
		(*admin)->importedServices = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		(*admin)->exportedIpcSegment = hashMap_create(NULL, NULL, NULL, NULL);
		(*admin)->importedIpcSegment = hashMap_create(NULL, NULL, NULL, NULL);
		(*admin)->pollThread = hashMap_create(NULL, NULL, NULL, NULL);
		(*admin)->pollThreadRunning = hashMap_create(NULL, NULL, NULL, NULL);

		if (logHelper_create(context, &(*admin)->loghelper) == CELIX_SUCCESS) {
		}
	}

	return status;
}

celix_status_t remoteServiceAdmin_destroy(remote_service_admin_t **admin) {
	celix_status_t status = CELIX_SUCCESS;

	hashMap_destroy((*admin)->exportedServices, false, false);
	hashMap_destroy((*admin)->importedServices, false, false);
	hashMap_destroy((*admin)->exportedIpcSegment, false, false);
	hashMap_destroy((*admin)->importedIpcSegment, false, false);
	hashMap_destroy((*admin)->pollThread, false, false);
	hashMap_destroy((*admin)->pollThreadRunning, false, false);

	free(*admin);

	*admin = NULL;

	return status;
}

celix_status_t remoteServiceAdmin_stop(remote_service_admin_t *admin) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&admin->exportedServicesLock);

	hash_map_iterator_pt iter = hashMapIterator_create(admin->exportedServices);
	while (hashMapIterator_hasNext(iter)) {
		array_list_pt exports = hashMapIterator_nextValue(iter);
		int i;
		for (i = 0; i < arrayList_size(exports); i++) {
			export_registration_t *export = arrayList_get(exports, i);
			exportRegistration_stopTracking(export);
		}
	}
	hashMapIterator_destroy(iter);
	celixThreadMutex_unlock(&admin->exportedServicesLock);

	celixThreadMutex_lock(&admin->importedServicesLock);

	iter = hashMapIterator_create(admin->importedServices);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);

		char* service = hashMapEntry_getKey(entry);
		import_registration_factory_t *importFactory = hashMapEntry_getValue(entry);

		if (importFactory != NULL) {

			int i;
			for (i = 0; i < arrayList_size(importFactory->registrations); i++) {
				import_registration_t *importRegistration = arrayList_get(importFactory->registrations, i);

				if (importFactory->trackedFactory != NULL) {
					importFactory->trackedFactory->unregisterProxyService(importFactory->trackedFactory->factory, importRegistration->endpointDescription);
				}
			}
			serviceTracker_close(importFactory->proxyFactoryTracker);
			importRegistrationFactory_close(importFactory);

			hashMap_remove(admin->importedServices, service);
			importRegistrationFactory_destroy(&importFactory);
		}
	}
	hashMapIterator_destroy(iter);
	celixThreadMutex_unlock(&admin->importedServicesLock);

	// set stop-thread-variable
	iter = hashMapIterator_create(admin->pollThreadRunning);
	while (hashMapIterator_hasNext(iter)) {
		bool *pollThreadRunning = hashMapIterator_nextValue(iter);
		*pollThreadRunning = false;
	}
	hashMapIterator_destroy(iter);

	// release lock
	iter = hashMapIterator_create(admin->exportedIpcSegment);
	while (hashMapIterator_hasNext(iter)) {
		ipc_segment_pt ipc = hashMapIterator_nextValue(iter);
		remoteServiceAdmin_unlock(ipc->semId, 1);
	}
	hashMapIterator_destroy(iter);

	// wait till threads has stopped
	iter = hashMapIterator_create(admin->pollThread);
	while (hashMapIterator_hasNext(iter)) {
		celix_thread_t *pollThread = hashMapIterator_nextValue(iter);

		if (pollThread != NULL) {
			status = celixThread_join(*pollThread, NULL);
		}
	}
	hashMapIterator_destroy(iter);

	iter = hashMapIterator_create(admin->importedIpcSegment);
	while (hashMapIterator_hasNext(iter)) {
		ipc_segment_pt ipc = hashMapIterator_nextValue(iter);
		remoteServiceAdmin_detachIpcSegment(ipc);
		free(ipc);
	}
	hashMapIterator_destroy(iter);

	iter = hashMapIterator_create(admin->exportedIpcSegment);
	while (hashMapIterator_hasNext(iter)) {
		ipc_segment_pt ipc = hashMapIterator_nextValue(iter);
		remoteServiceAdmin_deleteIpcSegment(ipc);
		free(ipc);
	}
	hashMapIterator_destroy(iter);

	remoteServiceAdmin_removeSharedIdentityFiles(admin);

	celix_logHelper_destroy(&admin->loghelper);
	return status;
}

static int remoteServiceAdmin_getCount(int semId, int semNr) {
	int token = -1;
	unsigned short semVals[3];
	union semun semArg;

	semArg.array = semVals;

	if (semctl(semId, 0, GETALL, semArg) == 0) {
		token = semArg.array[semNr];
	}

	return token;
}

static celix_status_t remoteServiceAdmin_lock(int semId, int semNr) {
	celix_status_t status = CELIX_SUCCESS;
	int semOpStatus = 0;
	struct sembuf semOperation;

	semOperation.sem_num = semNr;
	semOperation.sem_op = -1;
	semOperation.sem_flg = 0;

	do {
		status = CELIX_SUCCESS;

		if ((semOpStatus = semop(semId, &semOperation, 1)) != 0) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	} while (semOpStatus == -1 && errno == EINTR);

	return status;
}

static celix_status_t remoteServiceAdmin_unlock(int semId, int semNr) {
	celix_status_t status = CELIX_SUCCESS;
	int semOpStatus = 0;
	struct sembuf semOperation;

	semOperation.sem_num = semNr;
	semOperation.sem_op = 1;
	semOperation.sem_flg = 0;

	do {
		status = CELIX_SUCCESS;

		if ((semOpStatus = semop(semId, &semOperation, 1)) != 0) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
	} while (semOpStatus == -1 && errno == EINTR);

	return status;
}

celix_status_t remoteServiceAdmin_send(remote_service_admin_t *admin, endpoint_description_t *recpEndpoint, char *request, char **reply, int *replyStatus) {
	celix_status_t status = CELIX_SUCCESS;
	ipc_segment_pt ipc = NULL;

	if ((ipc = hashMap_get(admin->importedIpcSegment, recpEndpoint->service)) != NULL) {
		int semid = ipc->semId;

		/* lock critical area */
		remoteServiceAdmin_lock(semid, 0);

		/* write method and data */
		strcpy(ipc->shmBaseAddress, request);

		/* Check the status of the send-receive semaphore and reset them if not correct */
		if (remoteServiceAdmin_getCount(ipc->semId, 1) > 0) {
			semctl(semid, 1, SETVAL, (int) 0);
		}
		if (remoteServiceAdmin_getCount(ipc->semId, 2) > 0) {
			semctl(semid, 2, SETVAL, (int) 0);
		}

		/* Inform receiver we are invoking the remote service */
		remoteServiceAdmin_unlock(semid, 1);

		/* Wait until the receiver finished his operations */
		remoteServiceAdmin_lock(semid, 2);

		/* read reply */
		*reply = strdup(ipc->shmBaseAddress);

		/* TODO: fix replyStatus */
		*replyStatus = 0;

		/* release critical area */
		remoteServiceAdmin_unlock(semid, 0);

	} else {
		status = CELIX_ILLEGAL_STATE; /* could not find ipc segment */
	}

	return status;
}

static void * remoteServiceAdmin_receiveFromSharedMemory(void *data) {
	recv_shm_thread_pt thread_data = data;

	remote_service_admin_t *admin = thread_data->admin;
	endpoint_description_t *exportedEndpointDesc = thread_data->endpointDescription;

	ipc_segment_pt ipc;

	if ((ipc = hashMap_get(admin->exportedIpcSegment, exportedEndpointDesc->service)) != NULL) {
		bool *pollThreadRunning = hashMap_get(admin->pollThreadRunning, exportedEndpointDesc);

		while (*pollThreadRunning == true) {
			if ((remoteServiceAdmin_lock(ipc->semId, 1) == CELIX_SUCCESS) && (*pollThreadRunning == true)) {

				// TODO: align data size
				char *data = calloc(1024, sizeof(*data));
				strcpy(data, ipc->shmBaseAddress);

				hash_map_iterator_pt iter = hashMapIterator_create(admin->exportedServices);

				while (hashMapIterator_hasNext(iter)) {
					hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
					array_list_pt exports = hashMapEntry_getValue(entry);
					int expIt = 0;

					for (expIt = 0; expIt < arrayList_size(exports); expIt++) {
						export_registration_t *export = arrayList_get(exports, expIt);

						if ((strcmp(exportedEndpointDesc->service, export->endpointDescription->service) == 0) && (export->endpoint != NULL)) {
							char *reply = NULL;

							/* TODO: fix handling of handleRequest return value*/
									export->endpoint->handleRequest(export->endpoint->endpoint, data, &reply);

									if (reply != NULL) {
										if ((strlen(reply) * sizeof(char)) >= RSA_SHM_MEMSIZE) {
											logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "receiveFromSharedMemory : size of message bigger than shared memory message. NOT SENDING.");
										} else {
											strcpy(ipc->shmBaseAddress, reply);
										}
										free(reply);
									}
						} else {
							logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "receiveFromSharedMemory : No endpoint set for %s.", export->endpointDescription->service);
						}
					}
				}
				hashMapIterator_destroy(iter);
				free(data);

				remoteServiceAdmin_unlock(ipc->semId, 2);

			}
		}
	}

	free(data);

	return NULL;
}

celix_status_t remoteServiceAdmin_getSharedIdentifierFile(remote_service_admin_t *admin, char *fwUuid, char* servicename, char* outFile) {
	celix_status_t status = CELIX_SUCCESS;
	snprintf(outFile, RSA_FILEPATH_LENGTH, "%s/%s/%s", P_tmpdir, fwUuid, servicename);

	if (access(outFile, F_OK) != 0) {
		char tmpDir[RSA_FILEPATH_LENGTH];

		snprintf(tmpDir, RSA_FILEPATH_LENGTH, "%s/%s", P_tmpdir, fwUuid);

		if(mkdir(tmpDir, 0755) == -1 ){
			logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "error while creating directory %s (%s)", tmpDir, strerror(errno));
		}
		FILE *shid_file = fopen(outFile, "wb");
		if (shid_file == NULL) {
			logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "error while creating shared identifier file %s (%s)", outFile, strerror(errno));
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_DEBUG, "create shared identifier file %s", outFile);
			fclose(shid_file);
		}
	} else {
		logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_DEBUG, "shared identifier file %s already exists", outFile);
	}

	return status;
}

celix_status_t remoteServiceAdmin_removeSharedIdentityFile(remote_service_admin_t *admin, char *fwUuid, char* servicename) {
	celix_status_t status = CELIX_SUCCESS;
	char tmpPath[RSA_FILEPATH_LENGTH];
	int retVal = 0;

	snprintf(tmpPath, RSA_FILEPATH_LENGTH, "%s/%s/%s", P_tmpdir, fwUuid, servicename);

	retVal = unlink(tmpPath);

	if (retVal == 0) {
		logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_DEBUG, "Removed shared identifier file %s", tmpPath);
	} else {
		logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "Cannot remove shared identifier file %s", tmpPath);
		status = CELIX_FILE_IO_EXCEPTION;
	}

	return status;
}

celix_status_t remoteServiceAdmin_removeSharedIdentityFiles(remote_service_admin_t *admin) {
	char tmpDir[RSA_FILEPATH_LENGTH];
	const char* fwUuid = NULL;
	bundleContext_getProperty(admin->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &fwUuid);

	snprintf(tmpDir, RSA_FILEPATH_LENGTH, "%s/%s", P_tmpdir, fwUuid);

	DIR *d = opendir(tmpDir);
	size_t path_len = strlen(tmpDir);
	int retVal = 0;

	if (d) {
		struct dirent *p;

		while (!retVal && (p = readdir(d))) {
			char* f_name;
			size_t len;

			if (!strcmp(p->d_name, ".") || !strcmp(p->d_name, "..")) {
				continue;
			}

			len = path_len + strlen(p->d_name) + 2;
			f_name = (char*) calloc(len, 1);

			if (f_name) {
				snprintf(f_name, len, "%s/%s", tmpDir, p->d_name);

				retVal = unlink(f_name);

				if(retVal==0){
					logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_WARNING, "Removed shared identifier file %s (unproper clean-up?)", f_name);
				}
				else{
					logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_WARNING, "Unable to remove shared identifier file %s ", f_name);
				}

			}
			free(f_name);
		}
	}

	closedir(d);

	if (!retVal) {
		rmdir(tmpDir);
	}

	return retVal;
}

celix_status_t remoteServiceAdmin_exportService(remote_service_admin_t *admin, char *serviceId, celix_properties_t *properties, array_list_pt *registrations) {
	celix_status_t status = CELIX_SUCCESS;
	arrayList_create(registrations);

	array_list_pt references = NULL;
	service_reference_pt reference = NULL;
	char filter[256];
	const char *exportsProp = NULL;
	const char *providedProp = NULL;

	snprintf(filter, 256, "(%s=%s)", (char *) OSGI_FRAMEWORK_SERVICE_ID, serviceId);

	bundleContext_getServiceReferences(admin->context, NULL, filter, &references);

	if (arrayList_size(references) >= 1) {
		reference = arrayList_get(references, 0);
	}

	arrayList_destroy(references);

	serviceReference_getProperty(reference, (char *) OSGI_RSA_SERVICE_EXPORTED_INTERFACES, &exportsProp);
	serviceReference_getProperty(reference, (char *) OSGI_FRAMEWORK_OBJECTCLASS, &providedProp);

	char* exports = strndup(exportsProp, 1024*10);
	char* provided = strndup(providedProp, 1024*10);

	if (reference == NULL) {
		logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "expected a reference for service id %s.", serviceId);
		status = CELIX_ILLEGAL_STATE;
	}
	else if (exports == NULL || provided == NULL) {
		logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_WARNING, "No Services to export.");
	} else {
		logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_INFO, "Export services (%s)", exports);
		array_list_pt interfaces = NULL;
		arrayList_create(&interfaces);
		if (strcmp(utils_stringTrim(exports), "*") == 0) {
			char *provided_save_ptr = NULL;
			char *interface = strtok_r(provided, ",", &provided_save_ptr);
			while (interface != NULL) {
				arrayList_add(interfaces, utils_stringTrim(interface));
				interface = strtok_r(NULL, ",", &provided_save_ptr);
			}
		} else {
			char *provided_save_ptr = NULL;
			char *pinterface = strtok_r(provided, ",", &provided_save_ptr);
			while (pinterface != NULL) {
				char *exports_save_ptr = NULL;
				char *einterface = strtok_r(exports, ",", &exports_save_ptr);
				while (einterface != NULL) {
					if (strcmp(einterface, pinterface) == 0) {
						arrayList_add(interfaces, einterface);
					}
					einterface = strtok_r(NULL, ",", &exports_save_ptr);
				}
				pinterface = strtok_r(NULL, ",", &provided_save_ptr);
			}
		}

		if (arrayList_size(interfaces) != 0) {
			int iter = 0;
			for (iter = 0; iter < arrayList_size(interfaces); iter++) {
				char *interface = arrayList_get(interfaces, iter);
				export_registration_t *registration = NULL;

				exportRegistration_create(admin->loghelper, reference, NULL, admin, admin->context, &registration);
				arrayList_add(*registrations, registration);

				remoteServiceAdmin_installEndpoint(admin, registration, reference, interface);
				exportRegistration_open(registration);
				exportRegistration_startTracking(registration);

				if (remoteServiceAdmin_createOrAttachShm(admin->exportedIpcSegment, admin, registration->endpointDescription, true) == CELIX_SUCCESS) {
					recv_shm_thread_pt recvThreadData = NULL;

					if ((recvThreadData = calloc(1, sizeof(*recvThreadData))) == NULL) {
						status = CELIX_ENOMEM;
					} else {
						recvThreadData->admin = admin;
						recvThreadData->endpointDescription = registration->endpointDescription;

						celix_thread_t* pollThread = calloc(1, sizeof(*pollThread));
						bool *pollThreadRunningPtr = calloc(1, sizeof(*pollThreadRunningPtr));
						*pollThreadRunningPtr = true;

						hashMap_put(admin->pollThreadRunning, registration->endpointDescription, pollThreadRunningPtr);

						// start receiving thread
						status = celixThread_create(pollThread, NULL, remoteServiceAdmin_receiveFromSharedMemory, recvThreadData);

						hashMap_put(admin->pollThread, registration->endpointDescription, pollThread);
					}
				}
			}

			celixThreadMutex_lock(&admin->exportedServicesLock);
			hashMap_put(admin->exportedServices, reference, *registrations);
			celixThreadMutex_unlock(&admin->exportedServicesLock);

		}
		arrayList_destroy(interfaces);
	}

	free(exports);
	free(provided);

	return status;
}

celix_status_t remoteServiceAdmin_removeExportedService(remote_service_admin_t *admin, export_registration_t *registration) {
	celix_status_t status;
	ipc_segment_pt ipc = NULL;

	export_reference_t *ref = NULL;
	status = exportRegistration_getExportReference(registration, &ref);

	if (status == CELIX_SUCCESS) {
		bool *pollThreadRunning = NULL;

		service_reference_pt servRef;
		celixThreadMutex_lock(&admin->exportedServicesLock);
		exportReference_getExportedService(ref, &servRef);

		array_list_pt exports = (array_list_pt)hashMap_remove(admin->exportedServices, servRef);
		if(exports!=NULL){
			arrayList_destroy(exports);
		}

		exportRegistration_close(registration);

		if ((pollThreadRunning = hashMap_get(admin->pollThreadRunning, registration->endpointDescription)) != NULL) {
			*pollThreadRunning = false;

			if ((ipc = hashMap_get(admin->exportedIpcSegment, registration->endpointDescription->service)) != NULL) {
				celix_thread_t* pollThread;

				remoteServiceAdmin_unlock(ipc->semId, 1);

				if ((pollThread = hashMap_get(admin->pollThread, registration->endpointDescription)) != NULL) {
					status = celixThread_join(*pollThread, NULL);

					if (status == CELIX_SUCCESS) {
						semctl(ipc->semId, 1 , IPC_RMID);
						shmctl(ipc->shmId, IPC_RMID, 0);

						remoteServiceAdmin_removeSharedIdentityFile(admin, registration->endpointDescription->frameworkUUID, registration->endpointDescription->service);

						hashMap_remove(admin->pollThreadRunning, registration->endpointDescription);
						hashMap_remove(admin->exportedIpcSegment, registration->endpointDescription->service);
						hashMap_remove(admin->pollThread, registration->endpointDescription);

						free(pollThreadRunning);
						free(pollThread);
						free(ipc);
					}
				}
			}
		}
		exportRegistration_destroy(&registration);
	}

	if(ref!=NULL){
		free(ref);
	}

	celixThreadMutex_unlock(&admin->exportedServicesLock);

	return status;
}

celix_status_t remoteServiceAdmin_getIpcSegment(remote_service_admin_t *admin, endpoint_description_t *endpointDescription, ipc_segment_pt* ipc) {
	(*ipc) = (hashMap_containsKey(admin->importedIpcSegment, endpointDescription) == true) ? hashMap_get(admin->importedIpcSegment, endpointDescription) : NULL;

	return (*ipc != NULL) ? CELIX_SUCCESS : CELIX_ILLEGAL_ARGUMENT;
}

celix_status_t remoteServiceAdmin_detachIpcSegment(ipc_segment_pt ipc) {
	return (shmdt(ipc->shmBaseAddress) != -1) ? CELIX_SUCCESS : CELIX_BUNDLE_EXCEPTION;
}

celix_status_t remoteServiceAdmin_deleteIpcSegment(ipc_segment_pt ipc) {
	return ((semctl(ipc->semId, 1 /*ignored*/, IPC_RMID) != -1) && (shmctl(ipc->shmId, IPC_RMID, 0) != -1)) ? CELIX_SUCCESS : CELIX_BUNDLE_EXCEPTION;
}

celix_status_t remoteServiceAdmin_createOrAttachShm(hash_map_pt ipcSegment, remote_service_admin_t *admin, endpoint_description_t *endpointDescription, bool createIfNotFound) {
	celix_status_t status = CELIX_SUCCESS;

	/* setup ipc sehment */
	ipc_segment_pt ipc = NULL;

	celix_properties_t *endpointProperties = endpointDescription->properties;

	char *shmPath = NULL;
	char *shmFtokId = NULL;

	char *semPath = NULL;
	char *semFtokId = NULL;

	if ((shmPath = (char*)properties_get(endpointProperties, (char *) RSA_SHM_PATH_PROPERTYNAME)) == NULL) {
		logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_DEBUG, "No value found for key %s in endpointProperties.", RSA_SHM_PATH_PROPERTYNAME);
		status = CELIX_BUNDLE_EXCEPTION;
	} else if ((shmFtokId = (char*)properties_get(endpointProperties, (char *) RSA_SHM_FTOK_ID_PROPERTYNAME)) == NULL) {
		logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_DEBUG, "No value found for key %s in endpointProperties.", RSA_SHM_FTOK_ID_PROPERTYNAME);
		status = CELIX_BUNDLE_EXCEPTION;
	} else if ((semPath = (char*)properties_get(endpointProperties, (char *) RSA_SEM_PATH_PROPERTYNAME)) == NULL) {
		logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_DEBUG, "No value found for key %s in endpointProperties.", RSA_SEM_PATH_PROPERTYNAME);
		status = CELIX_BUNDLE_EXCEPTION;
	} else if ((semFtokId = (char*)properties_get(endpointProperties, (char *) RSA_SEM_FTOK_ID_PROPERTYNAME)) == NULL) {
		logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_DEBUG, "No value found for key %s in endpointProperties.", RSA_SEM_FTOK_ID_PROPERTYNAME);
		status = CELIX_BUNDLE_EXCEPTION;
	} else {
		key_t shmKey = ftok(shmPath, atoi(shmFtokId));
		ipc = calloc(1, sizeof(*ipc));
		if(ipc == NULL){
			return CELIX_ENOMEM;
		}

		if ((ipc->shmId = shmget(shmKey, RSA_SHM_MEMSIZE, 0666)) < 0) {
			logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_WARNING, "Could not attach to shared memory");

			if (createIfNotFound == true) {
				if ((ipc->shmId = shmget(shmKey, RSA_SHM_MEMSIZE, IPC_CREAT | 0666)) < 0) {
					logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "Creation of shared memory segment failed.");
					status = CELIX_BUNDLE_EXCEPTION;
				} else if ((ipc->shmBaseAddress = shmat(ipc->shmId, 0, 0)) == (char *) -1) {
					logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "Attaching of shared memory segment failed.");
					status = CELIX_BUNDLE_EXCEPTION;
				} else {
					logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_INFO, "shared memory segment successfully created at %p.", ipc->shmBaseAddress);
				}
			}
		} else if ((ipc->shmBaseAddress = shmat(ipc->shmId, 0, 0)) == (char *) -1) {
			logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "Attaching to shared memory segment failed.");
			status = CELIX_BUNDLE_EXCEPTION;
		} else {
			logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_INFO, "successfully attached to shared memory at %p.", ipc->shmBaseAddress);
		}
	}

	if(ipc != NULL && status == CELIX_SUCCESS){

		key_t semkey = ftok(semPath, atoi(semFtokId));
		int semflg = (createIfNotFound == true) ? (0666 | IPC_CREAT) : (0666);
		int semid = semget(semkey, 3, semflg);

		if (semid != -1) {
			// only reset semaphores if a create was supposed
			if ((createIfNotFound == true) && ((semctl(semid, 0, SETVAL, (int) 1) == -1) || (semctl(semid, 1, SETVAL, (int) 0) == -1) || (semctl(semid, 2, SETVAL, (int) 0) == -1))) {
				logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "error while initialize semaphores.");
			}

			logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_DEBUG, "semaphores w/ key %s and id %i added.", endpointDescription->service, semid);
			ipc->semId = semid;

			hashMap_put(ipcSegment, endpointDescription->service, ipc);
		} else {
			logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "error getting semaphores.");
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	if(ipc != NULL && status != CELIX_SUCCESS){
		free(ipc);
	}

	return status;
}

celix_status_t remoteServiceAdmin_installEndpoint(remote_service_admin_t *admin, export_registration_t *registration, service_reference_pt reference, char *interface) {
	celix_status_t status = CELIX_SUCCESS;
	celix_properties_t *endpointProperties = celix_properties_create();

	unsigned int size = 0;
	char **keys;

	serviceReference_getPropertyKeys(reference, &keys, &size);
	for (int i = 0; i < size; i++) {
		char *key = keys[i];
		const char *value = NULL;

		if (serviceReference_getProperty(reference, key, &value) == CELIX_SUCCESS && strcmp(key, (char*) OSGI_RSA_SERVICE_EXPORTED_INTERFACES) != 0 && strcmp(key, (char*) OSGI_FRAMEWORK_OBJECTCLASS) != 0) {
			celix_properties_set(endpointProperties, key, value);
		}
	}

	hash_map_entry_pt entry = hashMap_getEntry(endpointProperties, (void *) OSGI_FRAMEWORK_SERVICE_ID);

	char* key = hashMapEntry_getKey(entry);
	char *serviceId = (char *) hashMap_remove(endpointProperties, (void *) OSGI_FRAMEWORK_SERVICE_ID);
	const char *uuid = NULL;

	uuid_t endpoint_uid;
	uuid_generate(endpoint_uid);
	char endpoint_uuid[37];
	uuid_unparse_lower(endpoint_uid, endpoint_uuid);

	bundleContext_getProperty(admin->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
	celix_properties_set(endpointProperties, (char*) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, uuid);
	celix_properties_set(endpointProperties, (char*) OSGI_FRAMEWORK_OBJECTCLASS, interface);
	celix_properties_set(endpointProperties, (char*) OSGI_RSA_ENDPOINT_SERVICE_ID, serviceId);
	celix_properties_set(endpointProperties, (char*) OSGI_RSA_ENDPOINT_ID, endpoint_uuid);
	celix_properties_set(endpointProperties, (char*) OSGI_RSA_SERVICE_IMPORTED, "true");
//    celix_properties_set(endpointProperties, (char*) OSGI_RSA_SERVICE_IMPORTED_CONFIGS, (char*) CONFIGURATION_TYPE);

	if (celix_properties_get(endpointProperties, (char *) RSA_SHM_PATH_PROPERTYNAME, NULL) == NULL) {
		char sharedIdentifierFile[RSA_FILEPATH_LENGTH];

		if (remoteServiceAdmin_getSharedIdentifierFile(admin, (char*)uuid, interface, sharedIdentifierFile) == CELIX_SUCCESS) {
			celix_properties_set(endpointProperties, RSA_SHM_PATH_PROPERTYNAME, sharedIdentifierFile);
		} else {
			celix_properties_set(endpointProperties, (char *) RSA_SHM_PATH_PROPERTYNAME, (char *) RSA_SHM_DEFAULTPATH);
		}
	}
	if (celix_properties_get(endpointProperties, (char *) RSA_SHM_FTOK_ID_PROPERTYNAME, NULL) == NULL) {
		celix_properties_set(endpointProperties, (char *) RSA_SHM_FTOK_ID_PROPERTYNAME, (char *) RSA_SHM_DEFAULT_FTOK_ID);
	}
	if (celix_properties_get(endpointProperties, (char *) RSA_SEM_PATH_PROPERTYNAME, NULL) == NULL) {
		char sharedIdentifierFile[RSA_FILEPATH_LENGTH];

		if (remoteServiceAdmin_getSharedIdentifierFile(admin, (char*)uuid, interface, sharedIdentifierFile) == CELIX_SUCCESS) {
			celix_properties_set(endpointProperties, (char *) RSA_SEM_PATH_PROPERTYNAME, sharedIdentifierFile);
		} else {
			celix_properties_set(endpointProperties, (char *) RSA_SEM_PATH_PROPERTYNAME, (char *) RSA_SEM_DEFAULTPATH);
		}
	}
	if (celix_properties_get(endpointProperties, (char *) RSA_SEM_FTOK_ID_PROPERTYNAME, NULL) == NULL) {
		celix_properties_set(endpointProperties, (char *) RSA_SEM_FTOK_ID_PROPERTYNAME, (char *) RSA_SEM_DEFAULT_FTOK_ID);
	}

	endpoint_description_t *endpointDescription = NULL;
	remoteServiceAdmin_createEndpointDescription(admin, reference, endpointProperties, interface, &endpointDescription);
	exportRegistration_setEndpointDescription(registration, endpointDescription);

	free(key);
	free(serviceId);
	free(keys);

	return status;
}

celix_status_t remoteServiceAdmin_createEndpointDescription(remote_service_admin_t *admin, service_reference_pt reference, celix_properties_t *endpointProperties, char *interface, endpoint_description_t **description) {
	celix_status_t status = CELIX_SUCCESS;

	*description = calloc(1, sizeof(**description));
	if (!*description) {
		status = CELIX_ENOMEM;
	} else {
		if (status == CELIX_SUCCESS) {
			(*description)->properties = endpointProperties;
			(*description)->frameworkUUID = (char*)celix_properties_get(endpointProperties, (char*) OSGI_RSA_ENDPOINT_FRAMEWORK_UUID, NULL);
			const char *serviceId = NULL;
			serviceReference_getProperty(reference, (char*)OSGI_FRAMEWORK_SERVICE_ID, &serviceId);
			(*description)->serviceId = strtoull(serviceId, NULL, 0);
			(*description)->id = (char*)celix_properties_get(endpointProperties, (char*) OSGI_RSA_ENDPOINT_ID, NULL);
			(*description)->service = strndup(interface, 1024*10);
		}
	}

	return status;
}

celix_status_t remoteServiceAdmin_destroyEndpointDescription(endpoint_description_t **description) {
	celix_status_t status = CELIX_SUCCESS;

	celix_properties_destroy((*description)->properties);
	free((*description)->service);
	free(*description);

	return status;
}

celix_status_t remoteServiceAdmin_getExportedServices(remote_service_admin_t *admin, array_list_pt *services) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t remoteServiceAdmin_getImportedEndpoints(remote_service_admin_t *admin, array_list_pt *services) {
	celix_status_t status = CELIX_SUCCESS;
	return status;
}

celix_status_t remoteServiceAdmin_importService(remote_service_admin_t *admin, endpoint_description_t *endpointDescription, import_registration_t **registration) {
	celix_status_t status = CELIX_SUCCESS;

	logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_INFO, "RSA: Import service %s", endpointDescription->service);

	celixThreadMutex_lock(&admin->importedServicesLock);

	import_registration_factory_t *registration_factory = (import_registration_factory_t *) hashMap_get(admin->importedServices, endpointDescription->service);

	// check whether we already have a registration_factory registered in the hashmap
	if (registration_factory == NULL) {
		status = importRegistrationFactory_install(admin->loghelper, endpointDescription->service, admin->context, &registration_factory);
		if (status == CELIX_SUCCESS) {
			hashMap_put(admin->importedServices, endpointDescription->service, registration_factory);
		}
	}

	// factory available
	if (status != CELIX_SUCCESS || (registration_factory->trackedFactory == NULL)) {
		logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_WARNING, "RSA: no proxyFactory available.");
		if (status == CELIX_SUCCESS) {
			status = CELIX_SERVICE_EXCEPTION;
		}
	} else {
		// we create an importRegistration per imported service
		importRegistration_create(endpointDescription, admin, (sendToHandle) &remoteServiceAdmin_send, admin->context, registration);
		registration_factory->trackedFactory->registerProxyService(registration_factory->trackedFactory->factory, endpointDescription, admin, (sendToHandle) &remoteServiceAdmin_send);

		arrayList_add(registration_factory->registrations, *registration);
		remoteServiceAdmin_createOrAttachShm(admin->importedIpcSegment, admin, endpointDescription, false);
	}

	celixThreadMutex_unlock(&admin->importedServicesLock);

	return status;
}

celix_status_t remoteServiceAdmin_removeImportedService(remote_service_admin_t *admin, import_registration_t *registration) {
	celix_status_t status = CELIX_SUCCESS;

	if (registration != NULL) {

		celixThreadMutex_lock(&admin->importedServicesLock);

		ipc_segment_pt ipc = NULL;
		endpoint_description_t *endpointDescription = (endpoint_description_t *) registration->endpointDescription;
		import_registration_factory_t *registration_factory = (import_registration_factory_t *) hashMap_get(admin->importedServices, endpointDescription->service);

		// detach from IPC
		if (remoteServiceAdmin_getIpcSegment(admin, endpointDescription, &ipc) != CELIX_SUCCESS) {
			logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "Error while retrieving IPC segment for imported service %s.", endpointDescription->service);
		} else if (remoteServiceAdmin_detachIpcSegment(ipc) != CELIX_SUCCESS) {
			logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "Error while detaching IPC segment for imported service %s.", endpointDescription->service);
		}

		ipc = hashMap_remove(admin->importedIpcSegment,endpointDescription);
		if(ipc!=NULL){
			free(ipc);
		}

		// factory available
		if ((registration_factory == NULL) || (registration_factory->trackedFactory == NULL)) {
			logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_ERROR, "Error while retrieving registration factory for imported service %s.", endpointDescription->service);
		} else {
			registration_factory->trackedFactory->unregisterProxyService(registration_factory->trackedFactory->factory, endpointDescription);
			arrayList_removeElement(registration_factory->registrations, registration);
			importRegistration_destroy(registration);

			if (arrayList_isEmpty(registration_factory->registrations) == true) {
				logHelper_log(admin->loghelper, CELIX_LOG_LEVEL_INFO, "closing proxy");

				serviceTracker_close(registration_factory->proxyFactoryTracker);
				importRegistrationFactory_close(registration_factory);

				hashMap_remove(admin->importedServices, endpointDescription->service);

				importRegistrationFactory_destroy(&registration_factory);
			}
		}

		celixThreadMutex_unlock(&admin->importedServicesLock);
	}

	return status;
}
