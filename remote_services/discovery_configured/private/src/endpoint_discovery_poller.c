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
 * endpoint_discovery_poller.c
 *
 *  \date       3 Jul 2014
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <curl/curl.h>

#include "endpoint_discovery_poller.h"
#include "hash_map.h"
#include "array_list.h"
#include "celix_threads.h"
#include "utils.h"
#include "endpoint_listener.h"

struct endpoint_discovery_poller {
    discovery_pt discovery;
    hash_map_pt entries;
    celix_thread_mutex_t pollerLock;
    celix_thread_t pollerThread;
};

static void *endpointDiscoveryPoller_poll(void *data);
static celix_status_t endpointDiscoveryPoller_getEndpoints(endpoint_discovery_poller_pt poller, char *url, array_list_pt *updatedEndpoints);
static size_t endpointDiscoveryPoller_writeMemory(void *contents, size_t size, size_t nmemb, void *memoryPtr);
static celix_status_t endpointDiscoveryPoller_endpointDescriptionEquals(void *endpointPtr, void *comparePtr, bool *equals);

celix_status_t endpointDiscoveryPoller_create(discovery_pt discovery, endpoint_discovery_poller_pt *poller) {
    celix_status_t status = CELIX_SUCCESS;
    *poller = malloc(sizeof(**poller));
    if (!poller) {
        status = CELIX_ENOMEM;
    } else {
        (*poller)->discovery = discovery;
        (*poller)->entries = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
        status = celixThreadMutex_create(&(*poller)->pollerLock, NULL);
        if (status != CELIX_SUCCESS) {
            status = CELIX_ILLEGAL_STATE;
        } else {
            celixThread_create(&(*poller)->pollerThread, NULL, endpointDiscoveryPoller_poll, *poller);
        }
    }
    return status;
}

celix_status_t endpointDiscoveryPoller_addDiscoveryEndpoint(endpoint_discovery_poller_pt poller, char *url) {
    celix_status_t status = CELIX_SUCCESS;

    status = celixThreadMutex_lock(&(poller)->pollerLock);
    if (status != 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        array_list_pt endpoints;
        status = arrayList_createWithEquals(endpointDiscoveryPoller_endpointDescriptionEquals, &endpoints);
        if (status == CELIX_SUCCESS) {
            hashMap_put(poller->entries, url, endpoints);
        }
        status = celixThreadMutex_unlock(&poller->pollerLock);
        if (status != 0) {
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    return status;
}

celix_status_t endpointDiscoveryPoller_removeDiscoveryEndpoint(endpoint_discovery_poller_pt poller, char *url) {
    celix_status_t status = CELIX_SUCCESS;

    status = celixThreadMutex_lock(&poller->pollerLock);
    if (status != 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        array_list_pt entries = hashMap_remove(poller->entries, url);
        int i;
        for (i = 0; i < arrayList_size(entries); i++) {
            endpoint_description_pt endpoint = arrayList_get(entries, i);
            // discovery_removeDiscoveredEndpoint(poller->discovery, endpoint);
        }
        arrayList_destroy(entries);

        status = celixThreadMutex_unlock(&poller->pollerLock);
        if (status != 0) {
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    return status;
}

static void *endpointDiscoveryPoller_poll(void *data) {
    endpoint_discovery_poller_pt poller = (endpoint_discovery_poller_pt) data;

    celix_status_t status = celixThreadMutex_lock(&poller->pollerLock);
    if (status != 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        hash_map_iterator_pt iterator = hashMapIterator_create(poller->entries);
        while (hashMapIterator_hasNext(iterator)) {
            array_list_pt currentEndpoints = hashMapIterator_nextValue(iterator);
            char *url = hashMapIterator_nextKey(iterator);

            array_list_pt updatedEndpoints = NULL;
            status = endpointDiscoveryPoller_getEndpoints(poller, url, &updatedEndpoints);
            if (status == CELIX_SUCCESS) {
                int i;
                for (i = 0; i < arrayList_size(currentEndpoints); i++) {
                    endpoint_description_pt endpoint = arrayList_get(currentEndpoints, i);
                    if (!arrayList_contains(updatedEndpoints, endpoint)) {
                        // status = discovery_removeDiscoveredEndpoint(poller->discovery, endpoint);
                    }
                }

                arrayList_clear(currentEndpoints);
                arrayList_addAll(currentEndpoints, updatedEndpoints);
                arrayList_destroy(updatedEndpoints);

                for (i = 0; i < arrayList_size(currentEndpoints); i++) {
                    endpoint_description_pt endpoint = arrayList_get(currentEndpoints, i);
                    // status = discovery_addDiscoveredEndpoint(poller->discovery, endpoint);
                }
            }
        }

        status = celixThreadMutex_unlock(&poller->pollerLock);
        if (status != 0) {
            status = CELIX_BUNDLE_EXCEPTION;
        }
    }

    return NULL;
}

struct MemoryStruct {
  char *memory;
  size_t size;
};

static celix_status_t endpointDiscoveryPoller_getEndpoints(endpoint_discovery_poller_pt poller, char *url, array_list_pt *updatedEndpoints) {
    celix_status_t status = CELIX_SUCCESS;

    CURL *curl;
    CURLcode res;

    struct MemoryStruct chunk;
    chunk.memory = malloc(1);
    chunk.size = 0;

    curl_global_init(CURL_GLOBAL_ALL);
    curl = curl_easy_init();
    if(!curl) {
        status = CELIX_ILLEGAL_STATE;
    } else {
        curl_easy_setopt(curl, CURLOPT_URL, url);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, endpointDiscoveryPoller_writeMemory);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)&chunk);
        res = curl_easy_perform(curl);
        curl_easy_cleanup(curl);
    }

    // process endpoints file

    // clean up endpoints file
    if(chunk.memory) {
        free(chunk.memory);
    }
    curl_global_cleanup();
    return status;
}

static size_t endpointDiscoveryPoller_writeMemory(void *contents, size_t size, size_t nmemb, void *memoryPtr) {
    size_t realsize = size * nmemb;
    struct MemoryStruct *mem = (struct MemoryStruct *)memoryPtr;

    mem->memory = realloc(mem->memory, mem->size + realsize + 1);
    if(mem->memory == NULL) {
        /* out of memory! */
        printf("not enough memory (realloc returned NULL)\n");
        return 0;
    }

    memcpy(&(mem->memory[mem->size]), contents, realsize);
    mem->size += realsize;
    mem->memory[mem->size] = 0;

    return realsize;
}

static celix_status_t endpointDiscoveryPoller_endpointDescriptionEquals(void *endpointPtr, void *comparePtr, bool *equals) {
    endpoint_description_pt endpoint = (endpoint_description_pt) endpointPtr;
    endpoint_description_pt compare = (endpoint_description_pt) comparePtr;

    if (strcmp(endpoint->id, compare->id) == 0) {
        *equals = true;
    } else {
        *equals = false;
    }

    return CELIX_SUCCESS;
}
