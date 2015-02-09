#include <celix_errno.h>
#include <celix_log.h>

#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include <sys/types.h>
#include <sys/sem.h>
#include <sys/shm.h>

#include "shm.h"

// TODO: move somewhere else
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

    pthread_mutex_t globalLock;
};

void* shmAdress;

static celix_status_t discovery_shmRemoveWithIndex(shmData_pt data, int index);

/* returns the ftok key to identify shared memory*/
static key_t getShmKey() {
    return ftok(DISCOVERY_SHM_FILENAME, DISCOVERY_SHM_FTOK_ID);
}

/* creates a new shared memory block */
celix_status_t discovery_shmCreate(shmData_pt* data) {
    celix_status_t status = CELIX_SUCCESS;

    shmData_pt shmData = calloc(1, sizeof(struct shmData));

    key_t shmKey = getShmKey();

    if (!shmData) {
        status = CELIX_ENOMEM;
    } else if ((shmData->shmId = shmget(shmKey, DISCOVERY_SHM_MEMSIZE, IPC_CREAT | 0666)) < 0) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Creation of shared memory segment failed.");
        status = CELIX_BUNDLE_EXCEPTION;
    } else if ((shmAdress = shmat(shmData->shmId, 0, 0)) == (char*) -1) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Attaching of shared memory segment failed.");
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        shmData->numOfEntries = 0;

        pthread_mutexattr_t threadAttr;
        if (pthread_mutexattr_init(&threadAttr) != 0)
            printf("Error while initalizing lock attributes\n");
        else
            pthread_mutexattr_setrobust(&threadAttr, PTHREAD_MUTEX_ROBUST);

        if (pthread_mutex_init(&shmData->globalLock, &threadAttr) == 0)
            printf("Global lock sucessfully initialized\n");
        else {
            printf("Global lock init failed\n");
            exit(1);
        }
        memcpy(shmAdress, shmData, sizeof(struct shmData));

        (*data) = shmAdress;
    }
    free(shmData);

    return status;
}

celix_status_t discovery_shmAttach(shmData_pt* data) {
    celix_status_t status = CELIX_SUCCESS;
    key_t shmKey = ftok(DISCOVERY_SHM_FILENAME, DISCOVERY_SHM_FTOK_ID);
    int shmId = -1;

    if ((shmId = shmget(shmKey, DISCOVERY_SHM_MEMSIZE, 0666)) < 0) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "DISCOVERY : Attaching of shared memory segment failed.");
        status = CELIX_BUNDLE_EXCEPTION;
    }

    if (((*data) = shmat(shmId, 0, 0)) < 0) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "DISCOVERY : Attaching of shared memory segment failed.");
        status = CELIX_BUNDLE_EXCEPTION;
    }

    return status;
}

static celix_status_t discovery_shmGetwithIndex(shmData_pt data, char* key, char* value, int* index) {
    celix_status_t status = CELIX_BUNDLE_EXCEPTION;
    time_t currentTime = time(NULL);
    unsigned int i;

    for (i = 0; i < data->numOfEntries && status != CELIX_SUCCESS; i++) {
        shmEntry entry = data->entries[i];
        // check if entry is still valid
        if (data->entries[i].expires < currentTime) {
            discovery_shmRemoveWithIndex(data, i);
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

celix_status_t discovery_shmGetKeys(shmData_pt data, char** keys, int* size) {
    celix_status_t status = CELIX_SUCCESS;
    unsigned int i = 0;

    pthread_mutex_lock(&data->globalLock);

    for (i = 0; i < data->numOfEntries; i++) {
        shmEntry entry = data->entries[i];

        if (entry.key) {
            snprintf(keys[i], SHM_ENTRY_MAX_KEY_LENGTH, "%s", entry.key);
        }
    }

    (*size) = i;

    pthread_mutex_unlock(&data->globalLock);
    return status;
}

celix_status_t discovery_shmSet(shmData_pt data, char *key, char* value) {
    celix_status_t status = CELIX_SUCCESS;
    int index = -1;

    if (data->numOfEntries >= SHM_DATA_MAX_ENTRIES) {
        status = CELIX_ILLEGAL_STATE;
    } else {
        pthread_mutex_lock(&data->globalLock);

        // check if key already there
        if (discovery_shmGetwithIndex(data, key, NULL, &index) != CELIX_SUCCESS) {
            index = data->numOfEntries;

            snprintf(data->entries[index].key, SHM_ENTRY_MAX_KEY_LENGTH, "%s", key);
            data->numOfEntries++;
        }

        snprintf(data->entries[index].value, SHM_ENTRY_MAX_VALUE_LENGTH, "%s", value);
        data->entries[index].expires = (time(NULL) + SHM_ENTRY_DEFAULT_TTL);

        pthread_mutex_unlock(&data->globalLock);
    }

    return status;
}

celix_status_t discovery_shmGet(shmData_pt data, char* key, char* value) {
    celix_status_t status = CELIX_ILLEGAL_ARGUMENT;

    pthread_mutex_lock(&data->globalLock);

    status = discovery_shmGetwithIndex(data, key, value, NULL);

    pthread_mutex_unlock(&data->globalLock);

    return status;
}

static celix_status_t discovery_shmRemoveWithIndex(shmData_pt data, int index) {
    celix_status_t status = CELIX_SUCCESS;

    data->numOfEntries--;
    if (index < data->numOfEntries) {
        memcpy((void*) &data->entries[index], (void*) &data->entries[index + 1], ((data->numOfEntries - index) * sizeof(struct shmEntry)));
    }

    return status;
}

celix_status_t discovery_shmRemove(shmData_pt data, char* key) {
    celix_status_t status = CELIX_SUCCESS;
    int index = -1;

    pthread_mutex_lock(&data->globalLock);

    if ((status = discovery_shmGetwithIndex(data, key, NULL, &index)) == CELIX_SUCCESS) {

        discovery_shmRemoveWithIndex(data, index);
    }
    pthread_mutex_unlock(&data->globalLock);

    return status;

}

celix_status_t discovery_shmDetach(shmData_pt data) {
    celix_status_t status = CELIX_SUCCESS;

    if (data->numOfEntries == 0)
        discovery_shmDestroy(data);
    else {
        fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "DISCOVERY_SHM: Detaching from Shared memory\n");
        shmdt(shmAdress);
    }

    return status;
}

celix_status_t discovery_shmDestroy(shmData_pt data) {
    celix_status_t status = CELIX_SUCCESS;

    fw_log(logger, OSGI_FRAMEWORK_LOG_DEBUG, "DISCOVERY_SHM: Destroying Shared memory.");
    shmctl(data->shmId, IPC_RMID, 0);

    return status;

}
