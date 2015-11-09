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
 * etcd_watcher.c
 *
 * \date       16 Sep 2014
 * \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 * \copyright  Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include "log_helper.h"
#include "log_service.h"
#include "constants.h"
#include "utils.h"
#include "discovery.h"
#include "discovery_impl.h"

#include "etcd.h"
#include "etcd_watcher.h"

#include "endpoint_discovery_poller.h"

struct etcd_watcher {
    discovery_pt discovery;
    log_helper_pt* loghelper;
    hash_map_pt entries;

	celix_thread_mutex_t watcherLock;
	celix_thread_t watcherThread;

	volatile bool running;
};

#define CFG_ETCD_ROOT_PATH		"DISCOVERY_ETCD_ROOT_PATH"
#define DEFAULT_ETCD_ROOTPATH	"discovery"

#define CFG_ETCD_SERVER_IP		"DISCOVERY_ETCD_SERVER_IP"
#define DEFAULT_ETCD_SERVER_IP	"127.0.0.1"

#define CFG_ETCD_SERVER_PORT	"DISCOVERY_ETCD_SERVER_PORT"
#define DEFAULT_ETCD_SERVER_PORT 4001

// be careful - this should be higher than the curl timeout
#define CFG_ETCD_TTL   "DISCOVERY_ETCD_TTL"
#define DEFAULT_ETCD_TTL 30


// note that the rootNode shouldn't have a leading slash
static celix_status_t etcdWatcher_getRootPath(bundle_context_pt context, char* rootNode) {
	celix_status_t status = CELIX_SUCCESS;
	char* rootPath = NULL;

	if (((bundleContext_getProperty(context, CFG_ETCD_ROOT_PATH, &rootPath)) != CELIX_SUCCESS) || (!rootPath)) {
		strcpy(rootNode, DEFAULT_ETCD_ROOTPATH);
	}
	else {
		strcpy(rootNode, rootPath);
	}

	return status;
}


static celix_status_t etcdWatcher_getLocalNodePath(bundle_context_pt context, char* localNodePath) {
	celix_status_t status = CELIX_SUCCESS;
	char rootPath[MAX_ROOTNODE_LENGTH];
    char* uuid = NULL;

    if ((etcdWatcher_getRootPath(context, &rootPath[0]) != CELIX_SUCCESS)) {
		status = CELIX_ILLEGAL_STATE;
    }
	else if (((bundleContext_getProperty(context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &uuid)) != CELIX_SUCCESS) || (!uuid)) {
		status = CELIX_ILLEGAL_STATE;
	}
	else if (rootPath[strlen(&rootPath[0]) - 1] == '/') {
    	snprintf(localNodePath, MAX_LOCALNODE_LENGTH, "%s%s", &rootPath[0], uuid);
    }
    else {
    	snprintf(localNodePath, MAX_LOCALNODE_LENGTH, "%s/%s", &rootPath[0], uuid);
    }

    return status;
}
/*
 * retrieves all already existing discovery endpoints
 * from etcd and adds them to the poller.
 *
 * returns the modifiedIndex of the last modified
 * discovery endpoint (see etcd documentation).
 */
static celix_status_t etcdWatcher_addAlreadyExistingWatchpoints(discovery_pt discovery, int* highestModified) {
	celix_status_t status = CELIX_SUCCESS;
	char** nodeArr = calloc(MAX_NODES, sizeof(*nodeArr));
	char rootPath[MAX_ROOTNODE_LENGTH];
	int i, size;

	*highestModified = -1;

	for (i = 0; i < MAX_NODES; i++) {
		nodeArr[i] = calloc(MAX_KEY_LENGTH, sizeof(*nodeArr[i]));
	}

	// we need to go though all nodes and get the highest modifiedIndex
	if (((status = etcdWatcher_getRootPath(discovery->context, &rootPath[0])) == CELIX_SUCCESS) &&
		 (etcd_getNodes(rootPath, nodeArr, &size) == true)) {
		for (i = 0; i < size; i++) {
			char* key = nodeArr[i];
			char value[MAX_VALUE_LENGTH];
			char action[MAX_VALUE_LENGTH];
			int modIndex;

			if (etcd_get(key, &value[0], &action[0], &modIndex) == true) {
				// TODO: check that this is not equals to the local endpoint
				endpointDiscoveryPoller_addDiscoveryEndpoint(discovery->poller, &value[0]);

				if (modIndex > *highestModified) {
					*highestModified = modIndex;
				}
			}
		}
	}

	for (i = 0; i < MAX_NODES; i++) {
		free(nodeArr[i]);
	}

	free(nodeArr);

	return status;
}


static celix_status_t etcdWatcher_addOwnFramework(etcd_watcher_pt watcher)
{
    celix_status_t status = CELIX_BUNDLE_EXCEPTION;
    char localNodePath[MAX_LOCALNODE_LENGTH];
    char value[MAX_VALUE_LENGTH];
    char action[MAX_VALUE_LENGTH];
 	char url[MAX_VALUE_LENGTH];
    int modIndex;
    char* endpoints = NULL;
    char* ttlStr = NULL;
    int ttl;

	bundle_context_pt context = watcher->discovery->context;
	endpoint_discovery_server_pt server = watcher->discovery->server;

    // register own framework
    if ((status = etcdWatcher_getLocalNodePath(context, &localNodePath[0])) != CELIX_SUCCESS) {
        return status;
    }

	if (endpointDiscoveryServer_getUrl(server, &url[0]) != CELIX_SUCCESS) {
		snprintf(url, MAX_VALUE_LENGTH, "http://%s:%s/%s", DEFAULT_SERVER_IP, DEFAULT_SERVER_PORT, DEFAULT_SERVER_PATH);
	}

	endpoints = &url[0];

    if ((bundleContext_getProperty(context, CFG_ETCD_TTL, &ttlStr) != CELIX_SUCCESS) || !ttlStr) {
        ttl = DEFAULT_ETCD_TTL;
    }
    else
    {
        char* endptr = ttlStr;
        errno = 0;
        ttl =  strtol(ttlStr, &endptr, 10);
        if (*endptr || errno != 0) {
            ttl = DEFAULT_ETCD_TTL;
        }
    }

	if (etcd_get(localNodePath, &value[0], &action[0], &modIndex) != true) {
		etcd_set(localNodePath, endpoints, ttl, false);
	}
	else if (etcd_set(localNodePath, endpoints, ttl, true) == false)  {
		logHelper_log(*watcher->loghelper, OSGI_LOGSERVICE_WARNING, "Cannot register local discovery");
    }
    else {
        status = CELIX_SUCCESS;
    }

    return status;
}




static celix_status_t etcdWatcher_addEntry(etcd_watcher_pt watcher, char* key, char* value) {
	celix_status_t status = CELIX_BUNDLE_EXCEPTION;
	endpoint_discovery_poller_pt poller = watcher->discovery->poller;

	if (!hashMap_containsKey(watcher->entries, key)) {
		status = endpointDiscoveryPoller_addDiscoveryEndpoint(poller, value);

		if (status == CELIX_SUCCESS) {
			hashMap_put(watcher->entries, strdup(key), strdup(value));
		}
	}

	return status;
}

static celix_status_t etcdWatcher_removeEntry(etcd_watcher_pt watcher, char* key, char* value) {
	celix_status_t status = CELIX_BUNDLE_EXCEPTION;
	endpoint_discovery_poller_pt poller = watcher->discovery->poller;

	hash_map_entry_pt entry = hashMap_getEntry(watcher->entries, key);

	if (entry != NULL) {
		hashMap_remove(watcher->entries, key);

		free(hashMapEntry_getKey(entry));
		free(hashMapEntry_getValue(entry));
		free(entry);

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

	}

	return status;

}


/*
 * performs (blocking) etcd_watch calls to check for
 * changing discovery endpoint information within etcd.
 */
static void* etcdWatcher_run(void* data) {
	etcd_watcher_pt watcher = (etcd_watcher_pt) data;
	time_t timeBeforeWatch = time(NULL);
	static char rootPath[MAX_ROOTNODE_LENGTH];
	int highestModified = 0;

	bundle_context_pt context = watcher->discovery->context;

	etcdWatcher_addAlreadyExistingWatchpoints(watcher->discovery, &highestModified);
	etcdWatcher_getRootPath(context, &rootPath[0]);

	while (watcher->running) {

        char rkey[MAX_KEY_LENGTH];
		char value[MAX_VALUE_LENGTH];
		char preValue[MAX_VALUE_LENGTH];
		char action[MAX_ACTION_LENGTH];
        int modIndex;

		if (etcd_watch(rootPath, highestModified + 1, &action[0], &preValue[0], &value[0], &rkey[0], &modIndex) == true) {
			if (strcmp(action, "set") == 0) {
				etcdWatcher_addEntry(watcher, &rkey[0], &value[0]);
			} else if (strcmp(action, "delete") == 0) {
				etcdWatcher_removeEntry(watcher, &rkey[0], &value[0]);
			} else if (strcmp(action, "expire") == 0) {
				etcdWatcher_removeEntry(watcher, &rkey[0], &value[0]);
			} else if (strcmp(action, "update") == 0) {
				etcdWatcher_addEntry(watcher, &rkey[0], &value[0]);
			} else {
				logHelper_log(*watcher->loghelper, OSGI_LOGSERVICE_INFO, "Unexpected action: %s", action);
			}

			highestModified = modIndex;
		}

		// update own framework uuid
		if (time(NULL) - timeBeforeWatch > (DEFAULT_ETCD_TTL/2)) {
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
celix_status_t etcdWatcher_create(discovery_pt discovery, bundle_context_pt context,
		etcd_watcher_pt *watcher)
{
	celix_status_t status = CELIX_SUCCESS;

	char* etcd_server = NULL;
	char* etcd_port_string = NULL;
	int etcd_port = 0;

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
	}

	if ((bundleContext_getProperty(context, CFG_ETCD_SERVER_IP, &etcd_server) != CELIX_SUCCESS) || !etcd_server) {
		etcd_server = DEFAULT_ETCD_SERVER_IP;
	}

	if ((bundleContext_getProperty(context, CFG_ETCD_SERVER_PORT, &etcd_port_string) != CELIX_SUCCESS) || !etcd_port_string) {
		etcd_port = DEFAULT_ETCD_SERVER_PORT;
	}
	else
	{
		char* endptr = etcd_port_string;
		errno = 0;
		etcd_port =  strtol(etcd_port_string, &endptr, 10);
		if (*endptr || errno != 0) {
			etcd_port = DEFAULT_ETCD_SERVER_PORT;
		}
	}

	status = etcd_init(etcd_server, etcd_port);
	if (status != CELIX_SUCCESS)
	{
		return status;
	}

	etcdWatcher_addOwnFramework(*watcher);

	if ((status = celixThreadMutex_create(&(*watcher)->watcherLock, NULL)) != CELIX_SUCCESS) {
		return status;
	}

	if ((status = celixThreadMutex_lock(&(*watcher)->watcherLock)) != CELIX_SUCCESS) {
		return status;
	}

	if ((status = celixThread_create(&(*watcher)->watcherThread, NULL, etcdWatcher_run, *watcher)) != CELIX_SUCCESS) {
		return status;
	}

	(*watcher)->running = true;

	if ((status = celixThreadMutex_unlock(&(*watcher)->watcherLock)) != CELIX_SUCCESS) {
		return status;
	}

	return status;
}

celix_status_t etcdWatcher_destroy(etcd_watcher_pt watcher) {
	celix_status_t status = CELIX_SUCCESS;
	char localNodePath[MAX_LOCALNODE_LENGTH];

	watcher->running = false;

	celixThread_join(watcher->watcherThread, NULL);

	// register own framework
	status = etcdWatcher_getLocalNodePath(watcher->discovery->context, &localNodePath[0]);

	if (status != CELIX_SUCCESS || etcd_del(localNodePath) == false)
	{
		logHelper_log(*watcher->loghelper, OSGI_LOGSERVICE_WARNING, "Cannot remove local discovery registration.");
	}

	watcher->loghelper = NULL;

	hashMap_destroy(watcher->entries, true, true);

	free(watcher);

	return status;
}

