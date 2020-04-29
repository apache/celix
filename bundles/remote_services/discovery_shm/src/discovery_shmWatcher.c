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
 * discovery_shmWatcher.c
 *
 * \date       16 Sep 2014
 * \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 * \copyright  Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <stdio.h>


#include "celix_log.h"
#include "celix_constants.h"
#include "discovery_impl.h"

#include "discovery_shm.h"
#include "discovery_shmWatcher.h"

#include "endpoint_discovery_poller.h"

#define DEFAULT_SERVER_IP   "127.0.0.1"
#define DEFAULT_SERVER_PORT "9999"
#define DEFAULT_SERVER_PATH "/org.apache.celix.discovery.shm"
#define DEFAULT_POLL_ENDPOINTS "http://localhost:9999/org.apache.celix.discovery.shm"

#define MAX_ROOTNODE_LENGTH		 64
#define MAX_LOCALNODE_LENGTH	256


struct shm_watcher {
    shmData_t *shmData;
    celix_thread_t watcherThread;
    celix_thread_mutex_t watcherLock;

    volatile bool running;
};

// note that the rootNode shouldn't have a leading slash
static celix_status_t discoveryShmWatcher_getRootPath(char* rootNode) {
    celix_status_t status = CELIX_SUCCESS;

    strcpy(rootNode, "discovery");

    return status;
}

static celix_status_t discoveryShmWatcher_getLocalNodePath(celix_bundle_context_t *context, char* localNodePath) {
    celix_status_t status;
    char rootPath[MAX_ROOTNODE_LENGTH];
    const char* uuid = NULL;

    status = discoveryShmWatcher_getRootPath(&rootPath[0]);

    if (status == CELIX_SUCCESS) {
        status = bundleContext_getProperty(context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid);
    }

    if (status == CELIX_SUCCESS) {
        if (rootPath[strlen(&rootPath[0]) - 1] == '/') {
            snprintf(localNodePath, MAX_LOCALNODE_LENGTH, "%s%s", &rootPath[0], uuid);
        } else {
            snprintf(localNodePath, MAX_LOCALNODE_LENGTH, "%s/%s", &rootPath[0], uuid);
        }
    }

    return status;
}

/* retrieves all endpoints from shm and syncs them with the ones already available */
static celix_status_t discoveryShmWatcher_syncEndpoints(discovery_t *discovery) {
    celix_status_t status = CELIX_SUCCESS;
    shm_watcher_t *watcher = discovery->pImpl->watcher;
    char** shmKeyArr = calloc(SHM_DATA_MAX_ENTRIES, sizeof(*shmKeyArr));
    array_list_pt registeredKeyArr = NULL;

    int i, j, shmSize;

    for (i = 0; i < SHM_DATA_MAX_ENTRIES; i++) {
        shmKeyArr[i] = calloc(SHM_ENTRY_MAX_KEY_LENGTH, sizeof(*shmKeyArr[i]));
    }

    arrayList_create(&registeredKeyArr);

    // get all urls available in shm
    discoveryShm_getKeys(watcher->shmData, shmKeyArr, &shmSize);

    // get all locally registered endpoints
    endpointDiscoveryPoller_getDiscoveryEndpoints(discovery->poller, registeredKeyArr);

    // add discovery points which are in shm, but not local yet
    for (i = 0; i < shmSize; i++) {
        char url[SHM_ENTRY_MAX_VALUE_LENGTH];

        if (discoveryShm_get(watcher->shmData, shmKeyArr[i], &url[0]) == CELIX_SUCCESS) {
            bool elementFound = false;

            for (j = 0; j < arrayList_size(registeredKeyArr) && elementFound == false; j++) {

                if (strcmp(url, (char*) arrayList_get(registeredKeyArr, j)) == 0) {
                    free(arrayList_remove(registeredKeyArr, j));
                    elementFound = true;
                }
            }

            if (elementFound == false) {
                endpointDiscoveryPoller_addDiscoveryEndpoint(discovery->poller, url);
            }
        }
    }

    // remove those which are not in shm
    for (i = 0; i < arrayList_size(registeredKeyArr); i++) {
        char* regUrl = arrayList_get(registeredKeyArr, i);

        if (regUrl != NULL) {
            endpointDiscoveryPoller_removeDiscoveryEndpoint(discovery->poller, regUrl);
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

static void* discoveryShmWatcher_run(void* data) {
    discovery_t *discovery = (discovery_t *) data;
    shm_watcher_t *watcher = discovery->pImpl->watcher;
    char localNodePath[MAX_LOCALNODE_LENGTH];
    char url[MAX_LOCALNODE_LENGTH];

    if (discoveryShmWatcher_getLocalNodePath(discovery->context, &localNodePath[0]) != CELIX_SUCCESS) {
        celix_logHelper_log(discovery->loghelper, CELIX_LOG_LEVEL_WARNING, "Cannot retrieve local discovery path.");
    }

    if (endpointDiscoveryServer_getUrl(discovery->server, &url[0]) != CELIX_SUCCESS) {
        snprintf(url, MAX_LOCALNODE_LENGTH, "http://%s:%s/%s", DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT, DEFAULT_SERVER_PATH);
    }

    while (watcher->running) {
        // register own framework
        if (discoveryShm_set(watcher->shmData, localNodePath, url) != CELIX_SUCCESS) {
            celix_logHelper_log(discovery->loghelper, CELIX_LOG_LEVEL_WARNING, "Cannot set local discovery registration.");
        }

        discoveryShmWatcher_syncEndpoints(discovery);
        sleep(5);
    }

    return NULL;
}

celix_status_t discoveryShmWatcher_create(discovery_t *discovery) {
    celix_status_t status = CELIX_SUCCESS;
    shm_watcher_t *watcher = NULL;

    watcher = calloc(1, sizeof(*watcher));

    if (!watcher) {
        status = CELIX_ENOMEM;
    } else {
        status = discoveryShm_attach(&(watcher->shmData));

        if (status != CELIX_SUCCESS) {
            celix_logHelper_log(discovery->loghelper, CELIX_LOG_LEVEL_DEBUG, "Attaching to Shared Memory Failed. Trying to create.");

            status = discoveryShm_create(&(watcher->shmData));

            if (status != CELIX_SUCCESS) {
                celix_logHelper_log(discovery->loghelper, CELIX_LOG_LEVEL_ERROR, "Failed to create Shared Memory Segment.");
            }
        }

        if (status == CELIX_SUCCESS) {
            discovery->pImpl->watcher = watcher;
        }
        else{
        	discovery->pImpl->watcher = NULL;
        	free(watcher);
        }

    }

    if (status == CELIX_SUCCESS) {
        status += celixThreadMutex_create(&watcher->watcherLock, NULL);
        status += celixThreadMutex_lock(&watcher->watcherLock);
        watcher->running = true;
        status += celixThread_create(&watcher->watcherThread, NULL, discoveryShmWatcher_run, discovery);
        status += celixThreadMutex_unlock(&watcher->watcherLock);
    }

    return status;
}

celix_status_t discoveryShmWatcher_destroy(discovery_t *discovery) {
    celix_status_t status;
    shm_watcher_t *watcher = discovery->pImpl->watcher;
    char localNodePath[MAX_LOCALNODE_LENGTH];

    celixThreadMutex_lock(&watcher->watcherLock);
    watcher->running = false;
    celixThreadMutex_unlock(&watcher->watcherLock);

    celixThread_join(watcher->watcherThread, NULL);

    // remove own framework
    status = discoveryShmWatcher_getLocalNodePath(discovery->context, &localNodePath[0]);

    if (status == CELIX_SUCCESS) {
        status = discoveryShm_remove(watcher->shmData, localNodePath);
    }

    if (status == CELIX_SUCCESS) {
        discoveryShm_detach(watcher->shmData);
        free(watcher);
    }
    else {
        celix_logHelper_log(discovery->loghelper, CELIX_LOG_LEVEL_WARNING, "Cannot remove local discovery registration.");
    }


    return status;
}

