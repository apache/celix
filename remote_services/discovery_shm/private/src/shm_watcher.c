/**
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
/*
 * shm_watcher.c
 *
 * \date       16 Sep 2014
 * \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 * \copyright  Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>

#include "celix_log.h"
#include "constants.h"
#include "discovery_impl.h"

#include "shm.h"
#include "shm_watcher.h"

#include "endpoint_discovery_poller.h"

struct shm_watcher {
    endpoint_discovery_poller_pt poller;
    bundle_context_pt context;

    shmData_pt shmData;
    celix_thread_t watcherThread;
    celix_thread_mutex_t watcherLock;

    volatile bool running;
};

// note that the rootNode shouldn't have a leading slash
static celix_status_t shmWatcher_getRootPath(char* rootNode) {
    celix_status_t status = CELIX_SUCCESS;

    strcpy(rootNode, "discovery");

    return status;
}

static celix_status_t shmWatcher_getLocalNodePath(bundle_context_pt context, char* localNodePath) {
    celix_status_t status = CELIX_SUCCESS;
    char rootPath[MAX_ROOTNODE_LENGTH];
    char* uuid = NULL;

    if (shmWatcher_getRootPath(&rootPath[0]) != CELIX_SUCCESS) {
        status = CELIX_ILLEGAL_STATE;
    } else if (((bundleContext_getProperty(context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid)) != CELIX_SUCCESS) || (!uuid)) {
        status = CELIX_ILLEGAL_STATE;
    } else if (rootPath[strlen(&rootPath[0]) - 1] == '/') {
        snprintf(localNodePath, MAX_LOCALNODE_LENGTH, "%s%s", &rootPath[0], uuid);
    } else {
        snprintf(localNodePath, MAX_LOCALNODE_LENGTH, "%s/%s", &rootPath[0], uuid);
    }

    return status;
}

/* retrieves all endpoints from shm and syncs them with the ones already available */
static celix_status_t shmWatcher_syncEndpoints(shm_watcher_pt watcher) {
    celix_status_t status = CELIX_SUCCESS;
    char** shmKeyArr = calloc(SHM_DATA_MAX_ENTRIES, sizeof(*shmKeyArr));
    array_list_pt registeredKeyArr = NULL; //calloc(SHM_DATA_MAX_ENTRIES, sizeof(*registeredKeyArr));

    int i, j, shmSize;

    for (i = 0; i < SHM_DATA_MAX_ENTRIES; i++) {
        shmKeyArr[i] = calloc(SHM_ENTRY_MAX_KEY_LENGTH, sizeof(*shmKeyArr[i]));
    }

    arrayList_create(&registeredKeyArr);

    // get all urls available in shm
    discovery_shmGetKeys(watcher->shmData, shmKeyArr, &shmSize);

    // get all locally registered endpoints
    endpointDiscoveryPoller_getDiscoveryEndpoints(watcher->poller, registeredKeyArr);

    // add discovery points which are in shm, but not local yet
    for (i = 0; i < shmSize; i++) {
        char url[SHM_ENTRY_MAX_VALUE_LENGTH];
        bool elementFound = false;

        if (discovery_shmGet(watcher->shmData, shmKeyArr[i], &url[0]) == CELIX_SUCCESS) {
            for (j = 0; j < arrayList_size(registeredKeyArr) && elementFound == false; j++) {

                if (strcmp(url, (char*) arrayList_get(registeredKeyArr, j)) == 0) {
                    free(arrayList_remove(registeredKeyArr, j));
                    elementFound = true;
                }
            }

            if (elementFound == false) {
                endpointDiscoveryPoller_addDiscoveryEndpoint(watcher->poller, strdup(url));
            }
        }
    }

    // remove those which are not in shm
    for (i = 0; i < arrayList_size(registeredKeyArr); i++) {
        char* regUrl = arrayList_get(registeredKeyArr, i);

        if (regUrl != NULL) {
            endpointDiscoveryPoller_removeDiscoveryEndpoint(watcher->poller, regUrl);
        }
    }

    for (i = 0; i < SHM_DATA_MAX_ENTRIES; i++) {
        free(shmKeyArr[i]);
    }

    free(shmKeyArr);

    for (j = 0; j < arrayList_size(registeredKeyArr); j++) {
        free(arrayList_get(registeredKeyArr, j));
    }

    arrayList_destroy(registeredKeyArr);

    return status;
}

static void* shmWatcher_run(void* data) {
    shm_watcher_pt watcher = (shm_watcher_pt) data;
    char localNodePath[MAX_LOCALNODE_LENGTH];
    char* endpoints = NULL;

    if (shmWatcher_getLocalNodePath(watcher->context, &localNodePath[0]) != CELIX_SUCCESS) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Cannot register local discovery");
    }

    if ((bundleContext_getProperty(watcher->context, DISCOVERY_POLL_ENDPOINTS, &endpoints) != CELIX_SUCCESS) || !endpoints) {
        endpoints = DEFAULT_POLL_ENDPOINTS;
    }

    while (watcher->running) {
        // register own framework
        if (discovery_shmSet(watcher->shmData, localNodePath, endpoints) != CELIX_SUCCESS) {
            fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Cannot register local discovery");
        }

        shmWatcher_syncEndpoints(watcher);
        sleep(5);
    }

    return NULL;
}

celix_status_t shmWatcher_create(endpoint_discovery_poller_pt poller, bundle_context_pt context, shm_watcher_pt *watcher) {
    celix_status_t status = CELIX_SUCCESS;

    if (poller == NULL) {
        return CELIX_BUNDLE_EXCEPTION;
    }

    (*watcher) = calloc(1, sizeof(struct shm_watcher));
    if (!watcher) {
        return CELIX_ENOMEM;
    } else {
        (*watcher)->poller = poller;
        (*watcher)->context = context;
        if (discovery_shmAttach(&((*watcher)->shmData)) != CELIX_SUCCESS)
            discovery_shmCreate(&((*watcher)->shmData));

    }

    if ((status = celixThreadMutex_create(&(*watcher)->watcherLock, NULL)) != CELIX_SUCCESS) {
        return status;
    }

    if ((status = celixThreadMutex_lock(&(*watcher)->watcherLock)) != CELIX_SUCCESS) {
        return status;
    }

    if ((status = celixThread_create(&(*watcher)->watcherThread, NULL, shmWatcher_run, *watcher)) != CELIX_SUCCESS) {
        return status;
    }

    (*watcher)->running = true;

    if ((status = celixThreadMutex_unlock(&(*watcher)->watcherLock)) != CELIX_SUCCESS) {
        return status;
    }

    return status;
}

celix_status_t shmWatcher_destroy(shm_watcher_pt watcher) {
    celix_status_t status = CELIX_SUCCESS;
    char localNodePath[MAX_LOCALNODE_LENGTH];

    watcher->running = false;

    celixThread_join(watcher->watcherThread, NULL);

    // register own framework
    if ((status = shmWatcher_getLocalNodePath(watcher->context, &localNodePath[0])) != CELIX_SUCCESS) {
        return status;
    }

    if (discovery_shmRemove(watcher->shmData, localNodePath) != CELIX_SUCCESS) {
        fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Cannot remove local discovery registration.");
    }

    discovery_shmDetach(watcher->shmData);
    free(watcher);

    return status;
}

