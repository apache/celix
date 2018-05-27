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

#include <stdbool.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>
#include <jansson.h>

#include "celix_log.h"
#include "constants.h"

#include "etcd.h"
#include "etcd_watcher.h"

#include "pubsub_discovery.h"
#include "pubsub_discovery_impl.h"



#define MAX_ROOTNODE_LENGTH             128
#define MAX_LOCALNODE_LENGTH            4096
#define MAX_FIELD_LENGTH                128

#define CFG_ETCD_ROOT_PATH              "PUBSUB_DISCOVERY_ETCD_ROOT_PATH"
#define DEFAULT_ETCD_ROOTPATH           "pubsub/discovery"

#define CFG_ETCD_SERVER_IP              "PUBSUB_DISCOVERY_ETCD_SERVER_IP"
#define DEFAULT_ETCD_SERVER_IP          "127.0.0.1"

#define CFG_ETCD_SERVER_PORT            "PUBSUB_DISCOVERY_ETCD_SERVER_PORT"
#define DEFAULT_ETCD_SERVER_PORT        2379

// be careful - this should be higher than the curl timeout
#define CFG_ETCD_TTL                    "DISCOVERY_ETCD_TTL"
#define DEFAULT_ETCD_TTL                30


struct etcd_watcher {
	pubsub_discovery_pt pubsub_discovery;

	celix_thread_mutex_t watcherLock;
	celix_thread_t watcherThread;

	char *scope;
	char *topic;
	volatile bool running;
};

struct etcd_writer {
	pubsub_discovery_pt pubsub_discovery;
	celix_thread_mutex_t localPubsLock;
	array_list_pt localPubs;
	volatile bool running;
	celix_thread_t writerThread;
};


// note that the rootNode shouldn't have a leading slash
static celix_status_t etcdWatcher_getTopicRootPath(bundle_context_pt context, const char *scope, const char *topic, char* rootNode, int rootNodeLen) {
	celix_status_t status = CELIX_SUCCESS;
	const char* rootPath = NULL;

	if (((bundleContext_getProperty(context, CFG_ETCD_ROOT_PATH, &rootPath)) != CELIX_SUCCESS) || (!rootPath)) {
		snprintf(rootNode, rootNodeLen, "%s/%s/%s", DEFAULT_ETCD_ROOTPATH, scope, topic);
	} else {
		snprintf(rootNode, rootNodeLen, "%s/%s/%s", rootPath, scope, topic);
	}

	return status;
}

static celix_status_t etcdWatcher_getRootPath(bundle_context_pt context, char* rootNode) {
	celix_status_t status = CELIX_SUCCESS;
	const char* rootPath = NULL;

	if (((bundleContext_getProperty(context, CFG_ETCD_ROOT_PATH, &rootPath)) != CELIX_SUCCESS) || (!rootPath)) {
		strncpy(rootNode, DEFAULT_ETCD_ROOTPATH, MAX_ROOTNODE_LENGTH);
	} else {
		strncpy(rootNode, rootPath, MAX_ROOTNODE_LENGTH);
	}

	return status;
}


static void add_node(const char *key, const char *value, void* arg) {
	pubsub_discovery_pt ps_discovery = (pubsub_discovery_pt) arg;
	pubsub_endpoint_pt pubEP = NULL;
	celix_status_t status = etcdWatcher_getPublisherEndpointFromKey(ps_discovery, key, value, &pubEP);
	if(status == CELIX_SUCCESS) {
		pubsub_discovery_addNode(ps_discovery, pubEP);
	}
}

static celix_status_t etcdWatcher_addAlreadyExistingPublishers(pubsub_discovery_pt ps_discovery, const char *rootPath, long long * highestModified) {
	celix_status_t status = CELIX_SUCCESS;
	if(etcd_get_directory(rootPath, add_node, ps_discovery, highestModified)) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}
	return status;
}

// gets everything from provided key
celix_status_t etcdWatcher_getPublisherEndpointFromKey(pubsub_discovery_pt pubsub_discovery, const char* etcdKey, const char* etcdValue, pubsub_endpoint_pt* pubEP) {

	celix_status_t status = CELIX_SUCCESS;

	char rootPath[MAX_ROOTNODE_LENGTH];
	char *expr = NULL;
	char scope[MAX_FIELD_LENGTH];
	char topic[MAX_FIELD_LENGTH];
	char fwUUID[MAX_FIELD_LENGTH];
	char pubsubUUID[MAX_FIELD_LENGTH];

	memset(rootPath,0,MAX_ROOTNODE_LENGTH);
	memset(topic,0,MAX_FIELD_LENGTH);
	memset(fwUUID,0,MAX_FIELD_LENGTH);
	memset(pubsubUUID,0,MAX_FIELD_LENGTH);

	etcdWatcher_getRootPath(pubsub_discovery->context, rootPath);

	asprintf(&expr, "/%s/%%[^/]/%%[^/]/%%[^/]/%%[^/].*", rootPath);
	if(expr) {
		int foundItems = sscanf(etcdKey, expr, scope, topic, fwUUID, pubsubUUID);
		free(expr);
		if (foundItems != 4) { // Could happen when a directory is removed, just don't process this.
			status = CELIX_ILLEGAL_STATE;
		}
		else{

			// etcdValue contains the json formatted string
			json_error_t error;
			json_t* jsonRoot = json_loads(etcdValue, JSON_DECODE_ANY, &error);

			properties_t *discovered_props = properties_create();

			if (json_is_object(jsonRoot)) {

                void *iter = json_object_iter(jsonRoot);

                const char *key;
                json_t *value;

                while (iter) {
                    key = json_object_iter_key(iter);
                    value = json_object_iter_value(iter);

                    properties_set(discovered_props, key, json_string_value(value));
                    iter = json_object_iter_next(jsonRoot, iter);
                }
            }


            status = pubsubEndpoint_createFromDiscoveredProperties(discovered_props, pubEP);
            if (status != CELIX_SUCCESS) {
                properties_destroy(discovered_props);
            }

			if (jsonRoot != NULL) {
				json_decref(jsonRoot);
			}
		}
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
	char rootPath[MAX_ROOTNODE_LENGTH];
	long long highestModified = 0;

	pubsub_discovery_pt ps_discovery = watcher->pubsub_discovery;
	bundle_context_pt context = ps_discovery->context;

	memset(rootPath, 0, MAX_ROOTNODE_LENGTH);

	//TODO: add topic to etcd key
	etcdWatcher_getTopicRootPath(context, watcher->scope, watcher->topic, rootPath, MAX_ROOTNODE_LENGTH);
	etcdWatcher_addAlreadyExistingPublishers(ps_discovery, rootPath, &highestModified);

	while ((celixThreadMutex_lock(&watcher->watcherLock) == CELIX_SUCCESS) && watcher->running) {

		char *rkey = NULL;
		char *value = NULL;
		char *preValue = NULL;
		char *action = NULL;
		long long modIndex;

		celixThreadMutex_unlock(&watcher->watcherLock);

		if (etcd_watch(rootPath, highestModified + 1, &action, &preValue, &value, &rkey, &modIndex) == 0 && action != NULL) {
			pubsub_endpoint_pt pubEP = NULL;
			if ((strcmp(action, "set") == 0) || (strcmp(action, "create") == 0)) {
				if (etcdWatcher_getPublisherEndpointFromKey(ps_discovery, rkey, value, &pubEP) == CELIX_SUCCESS) {
					pubsub_discovery_addNode(ps_discovery, pubEP);
				}
			} else if (strcmp(action, "delete") == 0) {
				if (etcdWatcher_getPublisherEndpointFromKey(ps_discovery, rkey, preValue, &pubEP) == CELIX_SUCCESS) {
					pubsub_discovery_removeNode(ps_discovery, pubEP);
				}
			} else if (strcmp(action, "expire") == 0) {
				if (etcdWatcher_getPublisherEndpointFromKey(ps_discovery, rkey, preValue, &pubEP) == CELIX_SUCCESS) {
					pubsub_discovery_removeNode(ps_discovery, pubEP);
				}
			} else if (strcmp(action, "update") == 0) {
				if (etcdWatcher_getPublisherEndpointFromKey(ps_discovery, rkey, value, &pubEP) == CELIX_SUCCESS) {
					pubsub_discovery_addNode(ps_discovery, pubEP);
				}
			} else {
				fw_log(logger, OSGI_FRAMEWORK_LOG_INFO, "Unexpected action: %s", action);
			}
			highestModified = modIndex;
		} else if (time(NULL) - timeBeforeWatch <= (DEFAULT_ETCD_TTL / 4)) {
			sleep(DEFAULT_ETCD_TTL / 4);
		}

		FREE_MEM(action);
		FREE_MEM(value);
		FREE_MEM(preValue);
		FREE_MEM(rkey);

		/* prevent busy waiting, in case etcd_watch returns false */


		if (time(NULL) - timeBeforeWatch > (DEFAULT_ETCD_TTL / 4)) {
			timeBeforeWatch = time(NULL);
		}

	}

	if (watcher->running == false) {
		celixThreadMutex_unlock(&watcher->watcherLock);
	}

	return NULL;
}

celix_status_t etcdWatcher_create(pubsub_discovery_pt pubsub_discovery, bundle_context_pt context, const char *scope, const char *topic, etcd_watcher_pt *watcher) {
	celix_status_t status = CELIX_SUCCESS;


	if (pubsub_discovery == NULL) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	(*watcher) = calloc(1, sizeof(struct etcd_watcher));

	if(*watcher == NULL){
		return CELIX_ENOMEM;
	}

	(*watcher)->pubsub_discovery = pubsub_discovery;
	(*watcher)->scope = strdup(scope);
	(*watcher)->topic = strdup(topic);


	celixThreadMutex_create(&(*watcher)->watcherLock, NULL);

	celixThreadMutex_lock(&(*watcher)->watcherLock);

	status = celixThread_create(&(*watcher)->watcherThread, NULL, etcdWatcher_run, *watcher);
	if (status == CELIX_SUCCESS) {
		(*watcher)->running = true;
	}

	celixThreadMutex_unlock(&(*watcher)->watcherLock);


	return status;
}

celix_status_t etcdWatcher_destroy(etcd_watcher_pt watcher) {

	celix_status_t status = CELIX_SUCCESS;

	char rootPath[MAX_ROOTNODE_LENGTH];
	etcdWatcher_getTopicRootPath(watcher->pubsub_discovery->context, watcher->scope, watcher->topic, rootPath, MAX_ROOTNODE_LENGTH);
	celixThreadMutex_destroy(&(watcher->watcherLock));

	free(watcher->scope);
	free(watcher->topic);
	free(watcher);

	return status;
}

celix_status_t etcdWatcher_stop(etcd_watcher_pt watcher){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&(watcher->watcherLock));
	watcher->running = false;
	celixThreadMutex_unlock(&(watcher->watcherLock));

	celixThread_join(watcher->watcherThread, NULL);

	return status;

}


