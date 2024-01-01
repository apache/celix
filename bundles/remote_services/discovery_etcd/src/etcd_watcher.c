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

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include "celix_api.h"
#include "celix_log_helper.h"
#include "utils.h"
#include "discovery.h"
#include "discovery_impl.h"

#include "etcd.h"
#include "etcd_watcher.h"

#include "endpoint_discovery_poller.h"

struct etcd_watcher {
    etcdlib_t *etcdlib;
    int ttl;

    discovery_t *discovery;
    celix_log_helper_t **loghelper;
    hash_map_pt entries;

    celix_thread_mutex_t watcherLock;
    celix_thread_t watcherThread;

    volatile bool running;
};


#define MAX_ROOTNODE_LENGTH			128
#define MAX_LOCALNODE_LENGTH		4096
#define MAX_VALUE_LENGTH			256

#define CFG_ETCD_ROOT_PATH			"DISCOVERY_ETCD_ROOT_PATH"
#define DEFAULT_ETCD_ROOTPATH		"discovery"

#define CFG_ETCD_SERVER_IP			"DISCOVERY_ETCD_SERVER_IP"
#define DEFAULT_ETCD_SERVER_IP		"127.0.0.1"

#define CFG_ETCD_SERVER_PORT		"DISCOVERY_ETCD_SERVER_PORT"
#define DEFAULT_ETCD_SERVER_PORT 	2379

// be careful - this should be higher than the curl timeout
#define CFG_ETCD_TTL   				"DISCOVERY_ETCD_TTL"
#define DEFAULT_ETCD_TTL 			30


// note that the rootNode shouldn't have a leading slash
static celix_status_t etcdWatcher_getRootPath(celix_bundle_context_t *context, char* rootNode) {
	celix_status_t status = CELIX_SUCCESS;
	const char* rootPath = NULL;

	if (((bundleContext_getProperty(context, CFG_ETCD_ROOT_PATH, &rootPath)) != CELIX_SUCCESS) || (!rootPath)) {
		strncpy(rootNode, DEFAULT_ETCD_ROOTPATH, MAX_ROOTNODE_LENGTH);
	}
	else {
		strncpy(rootNode, rootPath, MAX_ROOTNODE_LENGTH);
	}

	return status;
}

static celix_status_t etcdWatcher_getLocalNodePath(celix_bundle_context_t *context, char* localNodePath) {
	celix_status_t status = CELIX_SUCCESS;
	char rootPath[MAX_ROOTNODE_LENGTH];
    const char* uuid = NULL;

    if ((etcdWatcher_getRootPath(context, rootPath) != CELIX_SUCCESS)) {
		status = CELIX_ILLEGAL_STATE;
    }
	else if (((bundleContext_getProperty(context, CELIX_FRAMEWORK_UUID, &uuid)) != CELIX_SUCCESS) || (!uuid)) {
		status = CELIX_ILLEGAL_STATE;
	}
	else if (rootPath[strlen(rootPath) - 1] == '/') {
    	snprintf(localNodePath, MAX_LOCALNODE_LENGTH, "%s%s", rootPath, uuid);
    }
    else {
    	snprintf(localNodePath, MAX_LOCALNODE_LENGTH, "%s/%s", rootPath, uuid);
    }

    return status;
}

static void add_node(const char *key, const char *value, void* arg) {
	discovery_t *discovery = (discovery_t *) arg;
	endpointDiscoveryPoller_addDiscoveryEndpoint(discovery->poller, (char *) value);
}

/*
 * retrieves all already existing discovery endpoints
 * from etcd and adds them to the poller.
 *
 * returns the modifiedIndex of the last modified
 * discovery endpoint (see etcd documentation).
 */
static celix_status_t etcdWatcher_addAlreadyExistingWatchpoints(etcd_watcher_t *watcher, discovery_t *discovery, long long* highestModified) {
	celix_status_t status = CELIX_SUCCESS;

	char rootPath[MAX_ROOTNODE_LENGTH];
	status = etcdWatcher_getRootPath(discovery->context, rootPath);

	if (status == CELIX_SUCCESS) {
		if(etcdlib_get_directory(watcher->etcdlib, rootPath, add_node, discovery, highestModified)) {
			    status = CELIX_ILLEGAL_ARGUMENT;
		}
	}

	return status;
}


static celix_status_t etcdWatcher_addOwnFramework(etcd_watcher_t *watcher)
{
    char localNodePath[MAX_LOCALNODE_LENGTH];
    char *value = NULL;
 	char url[MAX_VALUE_LENGTH];
    int modIndex;
    char* endpoints = NULL;

	celix_bundle_context_t *context = watcher->discovery->context;
	endpoint_discovery_server_t *server = watcher->discovery->server;

    // register own framework
    celix_status_t status;
    if ((status = etcdWatcher_getLocalNodePath(context, localNodePath)) != CELIX_SUCCESS) {
        return status;
    }

	if (endpointDiscoveryServer_getUrl(server, url, MAX_VALUE_LENGTH) != CELIX_SUCCESS) {
		snprintf(url, MAX_VALUE_LENGTH, "http://%s:%s/%s", DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT, DEFAULT_SERVER_PATH);
	}

	endpoints = url;

	watcher->ttl = celix_bundleContext_getPropertyAsLong(context, CFG_ETCD_TTL, DEFAULT_ETCD_TTL);

	if (etcdlib_get(watcher->etcdlib, localNodePath, &value, &modIndex) != ETCDLIB_RC_OK) {
		etcdlib_set(watcher->etcdlib, localNodePath, endpoints, watcher->ttl, false);
	} else if (etcdlib_set(watcher->etcdlib, localNodePath, endpoints, watcher->ttl , true) != ETCDLIB_RC_OK)  {
		celix_logHelper_log(*watcher->loghelper, CELIX_LOG_LEVEL_WARNING, "Cannot register local discovery");
    }

	FREE_MEM(value);

    return CELIX_SUCCESS;
}




static celix_status_t etcdWatcher_addEntry(etcd_watcher_t *watcher, char* key, char* value) {
	celix_status_t status = CELIX_BUNDLE_EXCEPTION;
	endpoint_discovery_poller_t *poller = watcher->discovery->poller;

	if (!hashMap_containsKey(watcher->entries, key)) {
		status = endpointDiscoveryPoller_addDiscoveryEndpoint(poller, value);

		if (status == CELIX_SUCCESS) {
			hashMap_put(watcher->entries, strdup(key), strdup(value));
		}
	}

	return status;
}

static celix_status_t etcdWatcher_removeEntry(etcd_watcher_t *watcher, char* key, char* value) {
	celix_status_t status = CELIX_BUNDLE_EXCEPTION;
	endpoint_discovery_poller_t *poller = watcher->discovery->poller;

	hash_map_entry_pt entry = hashMap_getEntry(watcher->entries, key);

	if (entry != NULL) {
		void* origKey = hashMapEntry_getKey(entry);
		void* value = hashMap_remove(watcher->entries, key);

		free(origKey);

		// check if there is another entry with the same value
		hash_map_iterator_pt iter = hashMapIterator_create(watcher->entries);
		unsigned int valueFound = 0;

		while (hashMapIterator_hasNext(iter) && valueFound <= 1) {
			if (strcmp(value, hashMapIterator_nextValue(iter)) == 0)
				valueFound++;
		}

		hashMapIterator_destroy(iter);

		if (valueFound == 0)
			status = endpointDiscoveryPoller_removeDiscoveryEndpoint(poller, value);

		free(value);

	}

	return status;

}


/*
 * performs (blocking) etcd_watch calls to check for
 * changing discovery endpoint information within etcd.
 */
static void* etcdWatcher_run(void* data) {
	etcd_watcher_t *watcher = (etcd_watcher_t *) data;
	time_t timeBeforeWatch = time(NULL);
	char rootPath[MAX_ROOTNODE_LENGTH];
	long long highestModified = 0;

	celix_bundle_context_t *context = watcher->discovery->context;

	etcdWatcher_addAlreadyExistingWatchpoints(watcher, watcher->discovery, &highestModified);
	etcdWatcher_getRootPath(context, rootPath);

	while (watcher->running) {

		char *rkey = NULL;
		char *value = NULL;
		char *preValue = NULL;
		char *action = NULL;
		long long modIndex;

        if (etcdlib_watch(watcher->etcdlib, rootPath, highestModified + 1, &action, &preValue, &value, &rkey, &modIndex) == 0 && action != NULL) {
			if (strcmp(action, "set") == 0) {
				etcdWatcher_addEntry(watcher, rkey, value);
			} else if (strcmp(action, "delete") == 0) {
				etcdWatcher_removeEntry(watcher, rkey, value);
			} else if (strcmp(action, "expire") == 0) {
				etcdWatcher_removeEntry(watcher, rkey, value);
			} else if (strcmp(action, "update") == 0) {
				etcdWatcher_addEntry(watcher, rkey, value);
			} else {
				celix_logHelper_log(*watcher->loghelper, CELIX_LOG_LEVEL_INFO, "Unexpected action: %s", action);
			}

			highestModified = modIndex;
        } else if (time(NULL) - timeBeforeWatch <= (watcher->ttl / 4)) {
			sleep(watcher->ttl / 4);
        }

        FREE_MEM(action);
        FREE_MEM(value);
        FREE_MEM(preValue);
        FREE_MEM(rkey);

		// update own framework uuid
		if (time(NULL) - timeBeforeWatch > (watcher->ttl / 4)) {
			etcdWatcher_addOwnFramework(watcher);
			timeBeforeWatch = time(NULL);
		}
	}

	return NULL;
}

/*
 * the ectdWatcher needs to have access to the endpoint_discovery_poller and therefore is only
 * allowed to be created after the endpoint_discovery_poller
 */
celix_status_t etcdWatcher_create(discovery_t *discovery, celix_bundle_context_t *context, etcd_watcher_t **watcher)
{
	if (discovery == NULL) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	(*watcher) = calloc(1, sizeof(struct etcd_watcher));
	if (!*watcher) {
		return CELIX_ENOMEM;
	}
	else
	{
		(*watcher)->discovery = discovery;
		(*watcher)->loghelper = &discovery->loghelper;
		(*watcher)->entries = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
        (*watcher)->ttl = DEFAULT_ETCD_TTL;
	}

	const char* etcd_server = celix_bundleContext_getProperty(context, CFG_ETCD_SERVER_IP, DEFAULT_ETCD_SERVER_IP);
	long etcd_port = celix_bundleContext_getPropertyAsLong(context, CFG_ETCD_SERVER_PORT, DEFAULT_ETCD_SERVER_PORT);

	(*watcher)->etcdlib = etcdlib_create(etcd_server, (int)etcd_port, ETCDLIB_NO_CURL_INITIALIZATION);
	celix_status_t status = CELIX_SUCCESS;
	if ((*watcher)->etcdlib == NULL) {
		status = CELIX_BUNDLE_EXCEPTION;
	}
    if (status == CELIX_SUCCESS) {
        etcdWatcher_addOwnFramework(*watcher);
        status = celixThreadMutex_create(&(*watcher)->watcherLock, NULL);
    }

    if (status == CELIX_SUCCESS) {
        if (celixThreadMutex_lock(&(*watcher)->watcherLock) == CELIX_SUCCESS) {
            status = celixThread_create(&(*watcher)->watcherThread, NULL, etcdWatcher_run, *watcher);
            if (status == CELIX_SUCCESS) {
                (*watcher)->running = true;
            }
            celixThreadMutex_unlock(&(*watcher)->watcherLock);
        }
    }

    return status;
}


celix_status_t etcdWatcher_destroy(etcd_watcher_t *watcher) {
	celix_status_t status = CELIX_SUCCESS;
	char localNodePath[MAX_LOCALNODE_LENGTH];

	celixThreadMutex_lock(&watcher->watcherLock);
	watcher->running = false;
	celixThreadMutex_unlock(&watcher->watcherLock);

	celixThread_join(watcher->watcherThread, NULL);

	// register own framework
	status = etcdWatcher_getLocalNodePath(watcher->discovery->context, localNodePath);

	if (status != CELIX_SUCCESS || etcdlib_del(watcher->etcdlib, localNodePath) == false)
	{
		celix_logHelper_log(*watcher->loghelper, CELIX_LOG_LEVEL_WARNING, "Cannot remove local discovery registration.");
	}

	watcher->loghelper = NULL;

	hashMap_destroy(watcher->entries, true, true);

	etcdlib_destroy(watcher->etcdlib);

	free(watcher);

	return status;
}

