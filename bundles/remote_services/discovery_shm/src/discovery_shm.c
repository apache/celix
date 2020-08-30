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
 * discovery_shm.c
 *
 *  \date       26 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <stdio.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include <celix_errno.h>
#include <celix_threads.h>

#include "discovery_shm.h"

#define DISCOVERY_SHM_MEMSIZE 262144
#define DISCOVERY_SHM_FILENAME "/dev/null"
#define DISCOVERY_SHM_FTOK_ID 50
#define DISCOVERY_SEM_FILENAME "/dev/null"
#define DISCOVERY_SEM_FTOK_ID 54

struct shmEntry {
    char key[SHM_ENTRY_MAX_KEY_LENGTH];
    char value[SHM_ENTRY_MAX_VALUE_LENGTH];

    time_t expires;
};

typedef struct shmEntry shmEntry;

struct shmData {
    shmEntry entries[SHM_DATA_MAX_ENTRIES];
    int numOfEntries;
    int shmId;

    celix_thread_mutex_t globalLock;
};

void* shmAddress;

static celix_status_t discoveryShm_removeWithIndex(shmData_t *data, int index);

/* returns the ftok key to identify shared memory*/
static key_t discoveryShm_getKey() {
    return ftok(DISCOVERY_SHM_FILENAME, DISCOVERY_SHM_FTOK_ID);
}

/* creates a new shared memory block */
celix_status_t discoveryShm_create(shmData_t **data) {
    celix_status_t status;

    shmData_t *shmData = calloc(1, sizeof(*shmData));
    key_t shmKey = discoveryShm_getKey();

    if (!shmData) {
        status = CELIX_ENOMEM;
    } else if ((shmData->shmId = shmget(shmKey, DISCOVERY_SHM_MEMSIZE, IPC_CREAT | 0666)) < 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else if ((shmAddress = shmat(shmData->shmId, 0, 0)) == (char*) -1) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        celix_thread_mutexattr_t threadAttr;

        shmData->numOfEntries = 0;

        status = celixThreadMutexAttr_create(&threadAttr);

#ifdef LINUX
        if (status == CELIX_SUCCESS) {
            // This is Linux specific
            status = pthread_mutexattr_setrobust(&threadAttr, PTHREAD_MUTEX_ROBUST);
        }
#endif

        if (status == CELIX_SUCCESS) {
            status = celixThreadMutex_create(&shmData->globalLock, &threadAttr);
        }

        if (status == CELIX_SUCCESS) {
            memcpy(shmAddress, shmData, sizeof(struct shmData));
            (*data) = shmAddress;
        }
    }

    free(shmData);

    return status;
}

celix_status_t discoveryShm_attach(shmData_t **data) {
    celix_status_t status = CELIX_SUCCESS;
    key_t shmKey = ftok(DISCOVERY_SHM_FILENAME, DISCOVERY_SHM_FTOK_ID);
    int shmId = -1;

    if ((shmId = shmget(shmKey, DISCOVERY_SHM_MEMSIZE, 0666)) < 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    }

    /* shmat has a curious return value of (void*)-1 in case of error */
    void *mem=shmat(shmId, 0, 0);
    if(mem==((void*)-1)){
	status = CELIX_BUNDLE_EXCEPTION;
    }
    else{
	(*data)=mem;
    }

    return status;
}

static celix_status_t discoveryShm_getwithIndex(shmData_t *data, char* key, char* value, int* index) {
    celix_status_t status = CELIX_BUNDLE_EXCEPTION;
    time_t currentTime = time(NULL);
    unsigned int i;

    for (i = 0; i < data->numOfEntries && status != CELIX_SUCCESS; i++) {
        shmEntry entry = data->entries[i];
        // check if entry is still valid
        if (data->entries[i].expires < currentTime) {
            discoveryShm_removeWithIndex(data, i);
        } else if (strcmp(entry.key, key) == 0) {
            if (value) {
                strcpy(value, entry.value);
            }
            if (index) {
                (*index) = i;
            }
            status = CELIX_SUCCESS;
        }
    }

    return status;
}

celix_status_t discoveryShm_getKeys(shmData_t *data, char** keys, int* size) {
    celix_status_t status;

    status = celixThreadMutex_lock(&data->globalLock);

    if (status == CELIX_SUCCESS) {
    	unsigned int i = 0;
        for (i = 0; i < data->numOfEntries; i++) {
            shmEntry entry = data->entries[i];

            if (strlen(entry.key)>0) {
                snprintf(keys[i], SHM_ENTRY_MAX_KEY_LENGTH, "%s", entry.key);
            }
        }

        (*size) = i;

        celixThreadMutex_unlock(&data->globalLock);
    }

    return status;
}

celix_status_t discoveryShm_set(shmData_t *data, char *key, char* value) {
    celix_status_t status;
    int index = -1;

    if (data->numOfEntries >= SHM_DATA_MAX_ENTRIES) {
        status = CELIX_ILLEGAL_STATE;
    } else {
        status = celixThreadMutex_lock(&data->globalLock);

        if (status == CELIX_SUCCESS) {
            // check if key already there
            status = discoveryShm_getwithIndex(data, key, NULL, &index);
            if (status != CELIX_SUCCESS) {
                index = data->numOfEntries;

                snprintf(data->entries[index].key, SHM_ENTRY_MAX_KEY_LENGTH, "%s", key);
                data->numOfEntries++;

                 status = CELIX_SUCCESS;
            }

            snprintf(data->entries[index].value, SHM_ENTRY_MAX_VALUE_LENGTH, "%s", value);
            data->entries[index].expires = (time(NULL) + SHM_ENTRY_DEFAULT_TTL);

            celixThreadMutex_unlock(&data->globalLock);
        }
    }

    return status;
}

celix_status_t discoveryShm_get(shmData_t *data, char* key, char* value) {
    celix_status_t status;

    status = celixThreadMutex_lock(&data->globalLock);

    if (status == CELIX_SUCCESS) {
        status = discoveryShm_getwithIndex(data, key, value, NULL);

        celixThreadMutex_unlock(&data->globalLock);
    }

    return status;
}

static celix_status_t discoveryShm_removeWithIndex(shmData_t *data, int index) {
    celix_status_t status = CELIX_SUCCESS;

    data->numOfEntries--;
    if (index < data->numOfEntries) {
        memcpy((void*) &data->entries[index], (void*) &data->entries[index + 1], ((data->numOfEntries - index) * sizeof(struct shmEntry)));
    }

    return status;
}

celix_status_t discoveryShm_remove(shmData_t *data, char* key) {
    celix_status_t status;
    int index = -1;

    status = celixThreadMutex_lock(&data->globalLock);

    if (status == CELIX_SUCCESS) {
        status = discoveryShm_getwithIndex(data, key, NULL, &index);

        if (status == CELIX_SUCCESS) {
            status = discoveryShm_removeWithIndex(data, index);
        }

        celixThreadMutex_unlock(&data->globalLock);
    }

    return status;
}

celix_status_t discoveryShm_detach(shmData_t *data) {
    celix_status_t status = CELIX_BUNDLE_EXCEPTION;

    if (data->numOfEntries == 0) {
        status = discoveryShm_destroy(data);
    }
    else if (shmdt(shmAddress) == 0) {
        status = CELIX_SUCCESS;
    }

    return status;
}

celix_status_t discoveryShm_destroy(shmData_t *data) {
    celix_status_t status = CELIX_BUNDLE_EXCEPTION;

    if (shmctl(data->shmId, IPC_RMID, 0) == 0) {
        status = CELIX_SUCCESS;
    }

    return status;

}
