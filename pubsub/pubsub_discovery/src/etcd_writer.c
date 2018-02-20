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
#include "etcd_writer.h"

#include "pubsub_discovery.h"
#include "pubsub_discovery_impl.h"

#define MAX_ROOTNODE_LENGTH		128

#define CFG_ETCD_ROOT_PATH		"PUBSUB_DISCOVERY_ETCD_ROOT_PATH"
#define DEFAULT_ETCD_ROOTPATH	"pubsub/discovery"

#define CFG_ETCD_SERVER_IP		"PUBSUB_DISCOVERY_ETCD_SERVER_IP"
#define DEFAULT_ETCD_SERVER_IP	"127.0.0.1"

#define CFG_ETCD_SERVER_PORT	"PUBSUB_DISCOVERY_ETCD_SERVER_PORT"
#define DEFAULT_ETCD_SERVER_PORT 2379

// be careful - this should be higher than the curl timeout
#define CFG_ETCD_TTL   "DISCOVERY_ETCD_TTL"
#define DEFAULT_ETCD_TTL 30

struct etcd_writer {
	pubsub_discovery_pt pubsub_discovery;
	celix_thread_mutex_t localPubsLock;
	array_list_pt localPubs;
	volatile bool running;
	celix_thread_t writerThread;
};


static const char* etcdWriter_getRootPath(bundle_context_pt context);
static void* etcdWriter_run(void* data);


etcd_writer_pt etcdWriter_create(pubsub_discovery_pt disc) {
	etcd_writer_pt writer = calloc(1, sizeof(*writer));
	if(writer) {
		celixThreadMutex_create(&writer->localPubsLock, NULL);
		arrayList_create(&writer->localPubs);
		writer->pubsub_discovery = disc;
		writer->running = true;
		celixThread_create(&writer->writerThread, NULL, etcdWriter_run, writer);
	}
	return writer;
}

void etcdWriter_destroy(etcd_writer_pt writer) {
	char dir[MAX_ROOTNODE_LENGTH];
	const char *rootPath = etcdWriter_getRootPath(writer->pubsub_discovery->context);

	writer->running = false;
	celixThread_join(writer->writerThread, NULL);

	celixThreadMutex_lock(&writer->localPubsLock);
	for(int i = 0; i < arrayList_size(writer->localPubs); i++) {
		pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(writer->localPubs,i);
		memset(dir,0,MAX_ROOTNODE_LENGTH);
		snprintf(dir,MAX_ROOTNODE_LENGTH,"%s/%s/%s/%s",
				 rootPath,
				 properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),
				 properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME),
				 properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID));

		etcd_del(dir);
		pubsubEndpoint_destroy(pubEP);
	}
	arrayList_destroy(writer->localPubs);

	celixThreadMutex_unlock(&writer->localPubsLock);
	celixThreadMutex_destroy(&(writer->localPubsLock));

	free(writer);
}

celix_status_t etcdWriter_addPublisherEndpoint(etcd_writer_pt writer, pubsub_endpoint_pt pubEP, bool storeEP){
	celix_status_t status = CELIX_BUNDLE_EXCEPTION;

	if(storeEP){
		const char *fwUUID = NULL;
		bundleContext_getProperty(writer->pubsub_discovery->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &fwUUID);
		if(fwUUID && strcmp(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID), fwUUID) == 0) {
			celixThreadMutex_lock(&writer->localPubsLock);
			pubsub_endpoint_pt p = NULL;
			pubsubEndpoint_clone(pubEP, &p);
			arrayList_add(writer->localPubs,p);
			celixThreadMutex_unlock(&writer->localPubsLock);
		}
	}

	char *key;

	const char* ttlStr = NULL;
	int ttl = 0;

	// determine ttl
	if ((bundleContext_getProperty(writer->pubsub_discovery->context, CFG_ETCD_TTL, &ttlStr) != CELIX_SUCCESS) || !ttlStr) {
		ttl = DEFAULT_ETCD_TTL;
	} else {
		char* endptr = NULL;
		errno = 0;
		ttl = strtol(ttlStr, &endptr, 10);
		if (*endptr || errno != 0) {
			ttl = DEFAULT_ETCD_TTL;
		}
	}

	const char *rootPath = etcdWriter_getRootPath(writer->pubsub_discovery->context);

	asprintf(&key,"%s/%s/%s/%s/%s",
			 rootPath,
			 properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),
			 properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME),
			 properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID),
			 properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_UUID));


	json_t *jsEndpoint = json_object();
	const char* propKey = NULL;
	PROPERTIES_FOR_EACH(pubEP->endpoint_props, propKey) {
		const char* val = properties_get(pubEP->endpoint_props, propKey);
		json_t* jsVal = json_string(val);
		json_object_set(jsEndpoint, propKey, jsVal);
	}
	char* jsonEndpointStr = json_dumps(jsEndpoint, JSON_COMPACT);

	if (!etcd_set(key,jsonEndpointStr,ttl,false)) {
		status = CELIX_ILLEGAL_ARGUMENT;
	}
	FREE_MEM(jsonEndpointStr);
	json_decref(jsEndpoint);

	return status;
}

celix_status_t etcdWriter_deletePublisherEndpoint(etcd_writer_pt writer, pubsub_endpoint_pt pubEP) {
	celix_status_t status = CELIX_SUCCESS;
	char *key = NULL;

	const char *rootPath = etcdWriter_getRootPath(writer->pubsub_discovery->context);

	asprintf(&key, "%s/%s/%s/%s/%s",
			 rootPath,
			 properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),
			 properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME),
			 properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID),
			 properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_UUID));

	celixThreadMutex_lock(&writer->localPubsLock);
	for (unsigned int i = 0; i < arrayList_size(writer->localPubs); i++) {
		pubsub_endpoint_pt ep = arrayList_get(writer->localPubs, i);
		if (pubsubEndpoint_equals(ep, pubEP)) {
			arrayList_remove(writer->localPubs, i);
			pubsubEndpoint_destroy(ep);
			break;
		}
	}
	celixThreadMutex_unlock(&writer->localPubsLock);

	if (etcd_del(key)) {
		printf("Failed to remove key %s from ETCD\n",key);
		status = CELIX_ILLEGAL_ARGUMENT;
	}
	FREE_MEM(key);
	return status;
}

static void* etcdWriter_run(void* data) {
	etcd_writer_pt writer = (etcd_writer_pt)data;
	while(writer->running) {
		celixThreadMutex_lock(&writer->localPubsLock);
		for(int i=0; i < arrayList_size(writer->localPubs); i++) {
			etcdWriter_addPublisherEndpoint(writer,(pubsub_endpoint_pt)arrayList_get(writer->localPubs,i),false);
		}
		celixThreadMutex_unlock(&writer->localPubsLock);
		sleep(DEFAULT_ETCD_TTL / 2);
	}

	return NULL;
}

static const char* etcdWriter_getRootPath(bundle_context_pt context) {
	const char* rootPath = NULL;
	bundleContext_getProperty(context, CFG_ETCD_ROOT_PATH, &rootPath);
	if(rootPath == NULL) {
		rootPath = DEFAULT_ETCD_ROOTPATH;
	}
	return rootPath;
}

