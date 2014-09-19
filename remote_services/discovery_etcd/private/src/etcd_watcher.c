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
 * endpoint_discovery_watcher.c
 *
 * \date       16 Sep 2014
 * \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 * \copyright  Apache License, Version 2.0
 */

#include <stdbool.h>
#include <stdlib.h>

#include "celix_log.h"
#include "constants.h"
#include "discovery.h"
#include "discovery_impl.h"

#include "etcd.h"
#include "etcd_watcher.h"

#include "endpoint_discovery_poller.h"

struct etcd_watcher {
	endpoint_discovery_poller_pt poller;
	bundle_context_pt context;

	celix_thread_mutex_t watcherLock;
	celix_thread_t watcherThread;

	volatile bool running;
};

#define CFG_ETCD_SERVER_IP		"DISCOVERY_ETCD_SERVER_IP"
#define DEFAULT_ETCD_SERVER_IP	"172.17.8.201"

#define CFG_ETCD_SERVER_PORT	"DISCOVERY_ETCD_SERVER_PORT"
#define DEFAULT_ETCD_SERVER_PORT 4001


// note that the rootNode shouldn't have a leading slash
static celix_status_t etcdWatcher_getRootPath(char* rootNode) {
	celix_status_t status = CELIX_SUCCESS;

	strcpy(rootNode, "discovery");

	return status;
}


static celix_status_t etcdWatcher_getLocalNodePath(bundle_context_pt context, char* localNodePath) {
	celix_status_t status = CELIX_SUCCESS;
	char rootPath[MAX_ROOTNODE_LENGTH];
    char* uuid = NULL;

    if (((etcdWatcher_getRootPath(&rootPath[0]) != CELIX_SUCCESS)) || (!rootPath)) {
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
static celix_status_t etcdWatcher_addAlreadyExistingWatchpoints(endpoint_discovery_poller_pt poller, int* highestModified) {
	celix_status_t status = CELIX_SUCCESS;
	char** nodeArr = calloc(MAX_NODES, sizeof(*nodeArr));
	char rootPath[MAX_ROOTNODE_LENGTH];
	int i, size;

	*highestModified = -1;

	for (i = 0; i < MAX_NODES; i++) {
		nodeArr[i] = calloc(MAX_KEY_LENGTH, sizeof(*nodeArr[i]));
	}

	// we need to go though all nodes and get the highest modifiedIndex
	if (((status = etcdWatcher_getRootPath(&rootPath[0])) == CELIX_SUCCESS) &&
		 (etcd_getNodes(rootPath, nodeArr, &size) == true)) {
		for (i = 0; i < size; i++) {
			char* key = nodeArr[i];
			char value[MAX_VALUE_LENGTH];
			char action[MAX_VALUE_LENGTH];
			int modIndex;

			if (etcd_get(key, &value[0], &action[0], &modIndex) == true) {
				// check that this is not equals to the local endpoint
				endpointDiscoveryPoller_addDiscoveryEndpoint(poller, strdup(&value[0]));

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

/*
 * performs (blocking) etcd_watch calls to check for
 * changing discovery endpoint information within etcd.
 */
static void* etcdWatcher_run(void* data) {
	etcd_watcher_pt watcher = (etcd_watcher_pt) data;

	static char rootPath[MAX_ROOTNODE_LENGTH];
	int highestModified = 0;

	etcdWatcher_addAlreadyExistingWatchpoints(watcher->poller, &highestModified);
	etcdWatcher_getRootPath(&rootPath[0]);

	while (watcher->running) {
		char value[MAX_VALUE_LENGTH];
		char preValue[MAX_VALUE_LENGTH];
		char action[MAX_ACTION_LENGTH];

		if (etcd_watch(rootPath, highestModified + 1, &action[0], &preValue[0], &value[0]) == true) {

			if (strcmp(action, "set") == 0) {
				endpointDiscoveryPoller_removeDiscoveryEndpoint(watcher->poller, &preValue[0]);
				endpointDiscoveryPoller_addDiscoveryEndpoint(watcher->poller, &value[0]);
			} else if (strcmp(action, "delete") == 0) {
				endpointDiscoveryPoller_removeDiscoveryEndpoint(watcher->poller, &preValue[0]);
			} else {
				fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "Unexpected action: %s", action);
			}
		}
	}

	return NULL;
}

/*
 * the ectdWatcher needs to have access to the endpoint_discovery_poller and therefore is only
 * allowed to be created after the endpoint_discovery_poller
 */
celix_status_t etcdWatcher_create(endpoint_discovery_poller_pt poller, bundle_context_pt context,
		etcd_watcher_pt *watcher)
{
	celix_status_t status = CELIX_SUCCESS;
	char localNodePath[MAX_LOCALNODE_LENGTH];
	char* etcd_server = NULL;
	char* etcd_port_string = NULL;
	int etcd_port = NULL;
	char* endpoints = NULL;

	if (poller == NULL) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	(*watcher) = calloc(1, sizeof(struct etcd_watcher));
	if (!watcher) {
		return CELIX_ENOMEM;
	}
	else
	{
		(*watcher)->poller = poller;
		(*watcher)->context = context;
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

	if ((bundleContext_getProperty(context, DISCOVERY_POLL_ENDPOINTS, &endpoints) != CELIX_SUCCESS) || !endpoints) {
		endpoints = DEFAULT_POLL_ENDPOINTS;
	}


	if (etcd_init(etcd_server, etcd_port) == false)
	{
		return CELIX_BUNDLE_EXCEPTION;
	}

	// register own framework
	if ((status = etcdWatcher_getLocalNodePath(context, &localNodePath[0])) != CELIX_SUCCESS) {
		return status;
	}

	if (etcd_set(localNodePath, endpoints) == false)
	{
		fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Cannot register local discovery");
	}



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
	if ((status = etcdWatcher_getLocalNodePath(watcher->context, &localNodePath[0])) != CELIX_SUCCESS) {
		return status;
	}

	if (etcd_del(localNodePath) == false)
	{
		fw_log(logger, OSGI_FRAMEWORK_LOG_WARNING, "Cannot remove local discovery registration.");
	}

	free(watcher);

	return status;
}

