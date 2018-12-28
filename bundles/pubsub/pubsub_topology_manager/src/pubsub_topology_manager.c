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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <celix_api.h>
#include <pubsub_utils.h>
#include <assert.h>

#include "hash_map.h"
#include "celix_array_list.h"
#include "celix_bundle_context.h"
#include "constants.h"
#include "utils.h"
#include "log_service.h"
#include "log_helper.h"

#include "pubsub_listeners.h"
#include "pubsub_topology_manager.h"
#include "pubsub_admin.h"
#include "../../pubsub_admin_udp_mc/src/pubsub_udpmc_topic_sender.h"

#define PSTM_PSA_HANDLING_SLEEPTIME_IN_SECONDS       30L

static void* pstm_psaHandlingThread(void *data);

celix_status_t pubsub_topologyManager_create(bundle_context_pt context, log_helper_pt logHelper, pubsub_topology_manager_t **out) {
	celix_status_t status = CELIX_SUCCESS;

	pubsub_topology_manager_t *manager = calloc(1, sizeof(*manager));
	if (manager == NULL) {
		*out = NULL;
		return CELIX_ENOMEM;
	} else {
		*out = manager;
	}

	manager->context = context;

	celix_thread_mutexattr_t psaAttr;
	celixThreadMutexAttr_create(&psaAttr);
	celixThreadMutexAttr_settype(&psaAttr, CELIX_THREAD_MUTEX_RECURSIVE);
	status |= celixThreadMutex_create(&manager->pubsubadmins.mutex, &psaAttr);
	celixThreadMutexAttr_destroy(&psaAttr);

	status |= celixThreadMutex_create(&manager->discoveredEndpoints.mutex, NULL);
	status |= celixThreadMutex_create(&manager->announceEndpointListeners.mutex, NULL);
	status |= celixThreadMutex_create(&manager->topicReceivers.mutex, NULL);
	status |= celixThreadMutex_create(&manager->topicSenders.mutex, NULL);
	status |= celixThreadMutex_create(&manager->psaHandling.mutex, NULL);

	status |= celixThreadCondition_init(&manager->psaHandling.cond, NULL);

	manager->discoveredEndpoints.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	manager->announceEndpointListeners.list = celix_arrayList_create();
	manager->pubsubadmins.map = hashMap_create(NULL, NULL, NULL, NULL);
	manager->topicReceivers.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	manager->topicSenders.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

	manager->loghelper = logHelper;
	manager->verbose = celix_bundleContext_getPropertyAsBool(context, PUBSUB_TOPOLOGY_MANAGER_VERBOSE_KEY, PUBSUB_TOPOLOGY_MANAGER_DEFAULT_VERBOSE);

	manager->psaHandling.running = true;
	celixThread_create(&manager->psaHandling.thread, NULL, pstm_psaHandlingThread, manager);
	celixThread_setName(&manager->psaHandling.thread, "PubSub TopologyManager");

	return status;
}

celix_status_t pubsub_topologyManager_destroy(pubsub_topology_manager_t *manager) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&manager->psaHandling.mutex);
	manager->psaHandling.running = false;
	celixThreadCondition_broadcast(&manager->psaHandling.cond);
	celixThreadMutex_unlock(&manager->psaHandling.mutex);
	celixThread_join(manager->psaHandling.thread, NULL);

	celixThreadMutex_lock(&manager->pubsubadmins.mutex);
	hashMap_destroy(manager->pubsubadmins.map, false, false);
	celixThreadMutex_unlock(&manager->pubsubadmins.mutex);
	celixThreadMutex_destroy(&manager->pubsubadmins.mutex);

	celixThreadMutex_lock(&manager->discoveredEndpoints.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(manager->discoveredEndpoints.map);
    while (hashMapIterator_hasNext(&iter)) {
        pstm_discovered_endpoint_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry != NULL) {
            celix_properties_destroy(entry->endpoint);
            free(entry);
        }
    }
	hashMap_destroy(manager->discoveredEndpoints.map, false, false);
	celixThreadMutex_unlock(&manager->discoveredEndpoints.mutex);
	celixThreadMutex_destroy(&manager->discoveredEndpoints.mutex);

    celixThreadMutex_lock(&manager->topicReceivers.mutex);
    iter = hashMapIterator_construct(manager->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry != NULL) {
            free(entry->scopeAndTopicKey);
            free(entry->scope);
            free(entry->topic);
            if (entry->endpoint != NULL) {
                celix_properties_destroy(entry->endpoint);
            }
            celix_properties_destroy(entry->subscriberProperties);
            free(entry);
        }
    }
    hashMap_destroy(manager->topicReceivers.map, false, false);
    celixThreadMutex_unlock(&manager->topicReceivers.mutex);
    celixThreadMutex_destroy(&manager->topicReceivers.mutex);

    celixThreadMutex_lock(&manager->topicSenders.mutex);
    iter = hashMapIterator_construct(manager->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry != NULL) {
            free(entry->scopeAndTopicKey);
            free(entry->scope);
            free(entry->topic);
            if (entry->endpoint != NULL) {
                celix_properties_destroy(entry->endpoint);
            }
            celix_filter_destroy(entry->publisherFilter);
            free(entry);
        }
    }
    hashMap_destroy(manager->topicSenders.map, false, false);
    celixThreadMutex_unlock(&manager->topicSenders.mutex);
    celixThreadMutex_destroy(&manager->topicSenders.mutex);

    celixThreadMutex_destroy(&manager->announceEndpointListeners.mutex);
	celix_arrayList_destroy(manager->announceEndpointListeners.list);

	free(manager);

	return status;
}

void pubsub_topologyManager_psaAdded(void * handle, void *svc, const celix_properties_t *props __attribute__((unused))) {
	pubsub_topology_manager_t *manager = handle;
	pubsub_admin_service_t *psa = (pubsub_admin_service_t*) svc;


	long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
	logHelper_log(manager->loghelper, OSGI_LOGSERVICE_DEBUG, "PSTM: Added PSA");

	if (svcId >= 0) {
		celixThreadMutex_lock(&manager->pubsubadmins.mutex);
		hashMap_put(manager->pubsubadmins.map, (void*)svcId, psa);
		celixThreadMutex_unlock(&manager->pubsubadmins.mutex);
	}

	//NOTE new psa, so no endpoints announce yet

	/* NOTE for now it assumed PSA / PST and PSD are started before subscribers/publisher
	 * so no retroactively adding subscribers
	 *
	 * TODO future extension?
	 */
}

void pubsub_topologyManager_psaRemoved(void * handle, void *svc __attribute__((unused)), const celix_properties_t *props) {
	pubsub_topology_manager_t *manager = handle;
	//pubsub_admin_service_t *psa = (pubsub_admin_service_t*) svc;
	long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);

	//NOTE psa shutdown will teardown topic receivers / topic senders
	//de-setup all topic receivers/senders for the removed psa.
	//the next psaHandling run will try to find new psa.

    celixThreadMutex_lock(&manager->topicSenders.mutex);
	hash_map_iterator_t iter = hashMapIterator_construct(manager->topicSenders.map);
	while (hashMapIterator_hasNext(&iter)) {
	    pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);
	    if (entry->selectedPsaSvcId == svcId) {
			/* de-announce all senders */
			if (entry->endpoint != NULL) {
				celixThreadMutex_lock(&manager->announceEndpointListeners.mutex);
				for (int j = 0; j < celix_arrayList_size(manager->announceEndpointListeners.list); ++j) {
					pubsub_announce_endpoint_listener_t *listener;
					listener = celix_arrayList_get(manager->announceEndpointListeners.list, j);
					listener->removeEndpoint(listener->handle, entry->endpoint);
				}
				celixThreadMutex_unlock(&manager->announceEndpointListeners.mutex);
			}

			entry->needsMatch = true;
	        entry->selectedSerializerSvcId = -1L;
	        entry->selectedPsaSvcId = -1L;
	        if (entry->endpoint != NULL) {
	            celix_properties_destroy(entry->endpoint);
                entry->endpoint = NULL;
            }
	    }
	}
    celixThreadMutex_unlock(&manager->topicSenders.mutex);

    celixThreadMutex_lock(&manager->topicReceivers.mutex);
    iter = hashMapIterator_construct(manager->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry->selectedPsaSvcId == svcId) {
			/* de-announce all receivers */
			if (entry->endpoint != NULL) {
				celixThreadMutex_lock(&manager->announceEndpointListeners.mutex);
				for (int j = 0; j < celix_arrayList_size(manager->announceEndpointListeners.list); ++j) {
					pubsub_announce_endpoint_listener_t *listener;
					listener = celix_arrayList_get(manager->announceEndpointListeners.list, j);
					listener->removeEndpoint(listener->handle, entry->endpoint);
				}
				celixThreadMutex_unlock(&manager->announceEndpointListeners.mutex);
			}

            entry->needsMatch = true;
            entry->selectedSerializerSvcId = -1L;
            entry->selectedPsaSvcId = -1L;
            if (entry->endpoint != NULL) {
                celix_properties_destroy(entry->endpoint);
                entry->endpoint = NULL;
            }
        }
    }
    celixThreadMutex_unlock(&manager->topicReceivers.mutex);



    logHelper_log(manager->loghelper, OSGI_LOGSERVICE_DEBUG, "PSTM: Removed PSA");
}

void pubsub_topologyManager_subscriberAdded(void * handle, void *svc __attribute__((unused)), const celix_properties_t *props, const celix_bundle_t *bnd) {
	pubsub_topology_manager_t *manager = handle;

	//NOTE new local subscriber service register
	//1) First trying to see if a TopicReceiver already exists for this subscriber, if found
	//2) update the usage count. if not found
	//3) signal psaHandling thread to setup topic receiver

	const char *topic = celix_properties_get(props, PUBSUB_SUBSCRIBER_TOPIC, NULL);
	const char *scope = celix_properties_get(props, PUBSUB_SUBSCRIBER_SCOPE, "default");
	if (topic == NULL) {
		logHelper_log(manager->loghelper, OSGI_LOGSERVICE_WARNING,
					  "[PSTM] Warning found subscriber service without mandatory %s property.",
					  PUBSUB_SUBSCRIBER_TOPIC);
		return;
	}

	long bndId = celix_bundle_getId(bnd);
	char *scopeAndTopicKey = NULL;
	scopeAndTopicKey = pubsubEndpoint_createScopeTopicKey(scope, topic);

	celixThreadMutex_lock(&manager->topicReceivers.mutex);
	pstm_topic_receiver_or_sender_entry_t *entry = hashMap_get(manager->topicReceivers.map, scopeAndTopicKey);
	if (entry != NULL) {
		entry->usageCount += 1;
		free(scopeAndTopicKey);
	} else {
		entry = calloc(1, sizeof(*entry));
		entry->scopeAndTopicKey = scopeAndTopicKey; //note taking owner ship
		entry->scope = strndup(scope, 1024 * 1024);
		entry->topic = strndup(topic, 1024 * 1024);
		entry->usageCount = 1;
		entry->selectedPsaSvcId = -1L;
		entry->selectedSerializerSvcId = -1L;
		entry->needsMatch = true;
		entry->bndId = bndId;
		entry->subscriberProperties = celix_properties_copy(props);
		hashMap_put(manager->topicReceivers.map, entry->scopeAndTopicKey, entry);

		//signal psa handling thread
		celixThreadCondition_broadcast(&manager->psaHandling.cond);
	}
	celixThreadMutex_unlock(&manager->topicReceivers.mutex);
}

void pubsub_topologyManager_subscriberRemoved(void * handle, void *svc __attribute__((unused)), const celix_properties_t *props, const celix_bundle_t *bnd) {
	pubsub_topology_manager_t *manager = handle;

	//NOTE local subscriber service unregister
	//1) Find topic receiver and decrease count

	const char *topic = celix_properties_get(props, PUBSUB_SUBSCRIBER_TOPIC, NULL);
	const char *scope = celix_properties_get(props, PUBSUB_SUBSCRIBER_SCOPE, "default");

	if (topic == NULL) {
		return;
	}

	char *scopeAndTopicKey = pubsubEndpoint_createScopeTopicKey(scope, topic);
	celixThreadMutex_lock(&manager->topicReceivers.mutex);
	pstm_topic_receiver_or_sender_entry_t *entry = hashMap_get(manager->topicReceivers.map, scopeAndTopicKey);
	if (entry != NULL) {
		entry->usageCount -= 0;
	}
	celixThreadMutex_unlock(&manager->topicReceivers.mutex);
	free(scopeAndTopicKey);

	//NOTE not waking up psaHandling thread, topic receiver does not need to be removed immediately.
}

void pubsub_topologyManager_pubsubAnnounceEndpointListenerAdded(void* handle, void *svc, const celix_properties_t *props __attribute__((unused))) {
	pubsub_topology_manager_t *manager = (pubsub_topology_manager_t *)handle;
	pubsub_announce_endpoint_listener_t *listener = svc;

	//1) retroactively call announceEndpoint for already existing endpoints (manager->announcedEndpoints)
	//2) Add listener to manager->announceEndpointListeners

	celixThreadMutex_lock(&manager->topicSenders.mutex);
	hash_map_iterator_t iter = hashMapIterator_construct(manager->topicSenders.map);
	while (hashMapIterator_hasNext(&iter)) {
		pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);
		if (entry != NULL && entry->endpoint != NULL) {
			listener->announceEndpoint(listener->handle, entry->endpoint);
		}
	}
	celixThreadMutex_unlock(&manager->topicSenders.mutex);

	celixThreadMutex_lock(&manager->topicReceivers.mutex);
	iter = hashMapIterator_construct(manager->topicReceivers.map);
	while (hashMapIterator_hasNext(&iter)) {
		pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);
		if (entry != NULL && entry->endpoint != NULL) {
			listener->announceEndpoint(listener->handle, entry->endpoint);
		}
	}
	celixThreadMutex_unlock(&manager->topicReceivers.mutex);

	celixThreadMutex_lock(&manager->announceEndpointListeners.mutex);
	celix_arrayList_add(manager->announceEndpointListeners.list, listener);
	celixThreadMutex_unlock(&manager->announceEndpointListeners.mutex);
}


void pubsub_topologyManager_pubsubAnnounceEndpointListenerRemoved(void * handle, void *svc, const celix_properties_t *props __attribute__((unused))) {
	pubsub_topology_manager_t *manager = (pubsub_topology_manager_t *)handle;
	pubsub_announce_endpoint_listener_t *listener = svc;

	//1) Remove listener from manager->announceEndpointListeners
	//2) call removeEndpoint for already existing endpoints (manager->announcedEndpoints)

	celixThreadMutex_lock(&manager->announceEndpointListeners.mutex);
	celix_arrayList_remove(manager->announceEndpointListeners.list, listener);
	celixThreadMutex_unlock(&manager->announceEndpointListeners.mutex);
}

void pubsub_topologyManager_publisherTrackerAdded(void *handle, const celix_service_tracker_info_t *info) {
	pubsub_topology_manager_t *manager = handle;

	//NOTE new local subscriber service register
	//1) First trying to see if a TopicReceiver already exists for this subscriber, if found
	//2) update the usage count. if not found
	//3) signal psaHandling thread to find a psa and setup TopicSender


	char *topicFromFilter = NULL;
	char *scopeFromFilter = NULL;
	pubsub_getPubSubInfoFromFilter(info->filter->filterStr, &topicFromFilter, &scopeFromFilter);
	char *scope = scopeFromFilter == NULL ? strndup("default", 32) : scopeFromFilter;
	char *topic = topicFromFilter;

	char *scopeAndTopicKey = NULL;
	if (topic == NULL) {
		logHelper_log(manager->loghelper, OSGI_LOGSERVICE_WARNING,
					  "[PSTM] Warning found publisher service request without mandatory '%s' filter attribute.",
					  PUBSUB_SUBSCRIBER_TOPIC);
		return;
	}

	scopeAndTopicKey = pubsubEndpoint_createScopeTopicKey(scope, topic);
	celixThreadMutex_lock(&manager->topicSenders.mutex);
	pstm_topic_receiver_or_sender_entry_t *entry = hashMap_get(manager->topicSenders.map, scopeAndTopicKey);
	if (entry != NULL) {
		entry->usageCount += 1;
		free(scope);
		free(topic);
		free(scopeAndTopicKey);
	} else {
		entry = calloc(1, sizeof(*entry));
		entry->usageCount = 1;
		entry->selectedSerializerSvcId = -1L;
		entry->selectedPsaSvcId = -1L;
		entry->scope = scope; //taking ownership
		entry->topic = topic; //taking ownership
		entry->scopeAndTopicKey = scopeAndTopicKey; //taking ownership
		entry->needsMatch = true;
		entry->publisherFilter = celix_filter_create(info->filter->filterStr);
		entry->bndId = info->bundleId;
		hashMap_put(manager->topicSenders.map, entry->scopeAndTopicKey, entry);

		//new entry -> wakeup psaHandling thread
		celixThreadCondition_broadcast(&manager->psaHandling.cond);
	}
	celixThreadMutex_unlock(&manager->topicSenders.mutex);
}

void pubsub_topologyManager_publisherTrackerRemoved(void *handle, const celix_service_tracker_info_t *info) {
	pubsub_topology_manager_t *manager = handle;

	//NOTE local subscriber service unregister
	//1) Find topic sender and decrease count

	char *topic = NULL;
	char *scopeFromFilter = NULL;
	pubsub_getPubSubInfoFromFilter(info->filter->filterStr, &topic, &scopeFromFilter);
	const char *scope = scopeFromFilter == NULL ? "default" : scopeFromFilter;

	if (topic == NULL) {
		free(scopeFromFilter);
		return;
	}


	char *scopeAndTopicKey = pubsubEndpoint_createScopeTopicKey(scope, topic);
	celixThreadMutex_lock(&manager->topicSenders.mutex);
	pstm_topic_receiver_or_sender_entry_t *entry = hashMap_get(manager->topicSenders.map, scopeAndTopicKey);
	if (entry != NULL) {
		entry->usageCount -= 1;
	}
	celixThreadMutex_unlock(&manager->topicSenders.mutex);

	free(scopeAndTopicKey);
	free(topic);
	free(scopeFromFilter);
}

celix_status_t pubsub_topologyManager_addDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint){
	celix_status_t status = CELIX_SUCCESS;
    pubsub_topology_manager_t *manager = handle;

    const char *uuid = celix_properties_get(endpoint, PUBSUB_ENDPOINT_UUID, NULL);
    assert(uuid != NULL); //discovery should check if endpoint is valid -> pubsubEndoint_isValid.

    // 1) See if endpoint is already discovered, if so increase usage count.
    // 1) If not, find matching psa using the matchEndpoint
    // 2) if found call addEndpoint of the matching psa

	if (manager->verbose) {
		logHelper_log(manager->loghelper, OSGI_LOGSERVICE_DEBUG,
					  "PSTM: Discovered endpoint added for topic %s with scope %s [fwUUID=%s, epUUID=%s]\n",
					  celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL),
					  celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL),
					  celix_properties_get(endpoint, PUBSUB_ENDPOINT_FRAMEWORK_UUID, NULL),
					  uuid);
	}


	celixThreadMutex_lock(&manager->discoveredEndpoints.mutex);
	pstm_discovered_endpoint_entry_t *entry = hashMap_get(manager->discoveredEndpoints.map, uuid);
	if (entry != NULL) {
		//already existing endpoint -> increase usage
		entry->usageCount += 1;
	} else {
		//new endpoint -> new entry
		entry = calloc(1, sizeof(*entry));
		entry->usageCount = 1;
		entry->endpoint = celix_properties_copy(endpoint);
		entry->uuid = celix_properties_get(entry->endpoint, PUBSUB_ENDPOINT_UUID, NULL);
		entry->selectedPsaSvcId = -1L; //NOTE not selected a psa yet
		hashMap_put(manager->discoveredEndpoints.map, (void*)entry->uuid, entry);

		//waking up psa handling thread to select psa
		celixThreadCondition_broadcast(&manager->psaHandling.cond);

	}
	celixThreadMutex_unlock(&manager->discoveredEndpoints.mutex);

    return status;
}

static void pstm_removeEndpointCallback(void *handle, void *svc) {
	celix_properties_t *endpoint = handle;
	pubsub_admin_service_t *psa = svc;
	psa->removeEndpoint(psa->handle, endpoint);
}

celix_status_t pubsub_topologyManager_removeDiscoveredEndpoint(void *handle, const celix_properties_t *endpoint) {
    pubsub_topology_manager_t *manager = handle;

	// 1) See if endpoint is already discovered, if so decrease usage count.
	// 1) If usage count becomes 0, find matching psa using the matchEndpoint
	// 2) if found call disconnectEndpoint of the matching psa

	const char *uuid = celix_properties_get(endpoint, PUBSUB_ENDPOINT_UUID, NULL);
	assert(uuid != NULL); //discovery should check if endpoint is valid -> pubsubEndoint_isValid.

	if (manager->verbose) {
		logHelper_log(manager->loghelper, OSGI_LOGSERVICE_DEBUG,
					  "PSTM: Discovered endpoint removed for topic %s with scope %s [fwUUID=%s, epUUID=%s]\n",
					  celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, NULL),
					  celix_properties_get(endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL),
					  celix_properties_get(endpoint, PUBSUB_ENDPOINT_FRAMEWORK_UUID, NULL),
					  uuid);
	}

	celixThreadMutex_lock(&manager->discoveredEndpoints.mutex);
	pstm_discovered_endpoint_entry_t *entry = hashMap_get(manager->discoveredEndpoints.map, uuid);
	if (entry != NULL) {
		//already existing endpoint -> decrease usage
		entry->usageCount-= 1;
		if (entry->usageCount <= 0) {
			hashMap_remove(manager->discoveredEndpoints.map, entry->uuid);
		} else {
			entry = NULL; //still used (usage count > 0) -> do nothing
		}
	}
	celixThreadMutex_unlock(&manager->discoveredEndpoints.mutex);

	if (entry != NULL) {
		//note entry is removed from manager->discoveredEndpoints, also inform used psa
		if (entry->selectedPsaSvcId >= 0) {
			//note that it is possible that the psa is already gone, in that case the call is also not needed anymore.
			celix_bundleContext_useServiceWithId(manager->context, entry->selectedPsaSvcId, PUBSUB_ADMIN_SERVICE_NAME,
												 (void *) endpoint, pstm_removeEndpointCallback);
		} else {
			logHelper_log(manager->loghelper, OSGI_LOGSERVICE_DEBUG, "No selected psa for endpoint %s\n", entry->uuid);
		}
		celix_properties_destroy(entry->endpoint);
		free(entry);
	}


	return CELIX_SUCCESS;
}


static void pstm_teardownTopicSenderCallback(void *handle, void *svc) {
    pstm_topic_receiver_or_sender_entry_t *entry = handle;
    pubsub_admin_service_t *psa = svc;
    psa->teardownTopicSender(psa->handle, entry->scope, entry->topic);
}

static void pstm_teardownTopicSenders(pubsub_topology_manager_t *manager) {
    celixThreadMutex_lock(&manager->topicSenders.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(manager->topicSenders.map);
    while (hashMapIterator_hasNext(&iter)) {
        pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);

        if (entry != NULL && (entry->usageCount <= 0 || entry->needsMatch)) {
            if (manager->verbose && entry->endpoint != NULL) {
                logHelper_log(manager->loghelper, OSGI_LOGSERVICE_DEBUG,
                        "[PSTM] Tearing down TopicSender for scope/topic %s/%s\n", entry->scope, entry->topic);
            }

            if (entry->endpoint != NULL) {
				celixThreadMutex_lock(&manager->announceEndpointListeners.mutex);
				for (int i = 0; i < celix_arrayList_size(manager->announceEndpointListeners.list); ++i) {
					pubsub_announce_endpoint_listener_t *listener;
					listener = celix_arrayList_get(manager->announceEndpointListeners.list, i);
					listener->removeEndpoint(listener->handle, entry->endpoint);
				}
				celixThreadMutex_unlock(&manager->announceEndpointListeners.mutex);
				celix_bundleContext_useServiceWithId(manager->context, entry->selectedPsaSvcId,
													 PUBSUB_ADMIN_SERVICE_NAME,
													 entry, pstm_teardownTopicSenderCallback);
			}


            //cleanup entry
            if (entry->usageCount <= 0) {
            	//no usage -> remove
				hashMapIterator_remove(&iter);
				free(entry->scopeAndTopicKey);
				free(entry->scope);
				free(entry->topic);
				if (entry->publisherFilter != NULL) {
					celix_filter_destroy(entry->publisherFilter);
				}
				if (entry->endpoint != NULL) {
					celix_properties_destroy(entry->endpoint);
				}
				free(entry);
			} else {
            	//still usage, setup for new match
            	if (entry->endpoint != NULL) {
					celix_properties_destroy(entry->endpoint);
				}
            	entry->endpoint = NULL;
            	entry->selectedPsaSvcId = -1L;
            	entry->selectedSerializerSvcId = -1L;
            }
        }
    }
    celixThreadMutex_unlock(&manager->topicSenders.mutex);
}

static void pstm_teardownTopicReceiverCallback(void *handle, void *svc) {
    pstm_topic_receiver_or_sender_entry_t *entry = handle;
    pubsub_admin_service_t *psa = svc;
    psa->teardownTopicReceiver(psa->handle, entry->scope, entry->topic);
}

static void pstm_teardownTopicReceivers(pubsub_topology_manager_t *manager) {
    celixThreadMutex_lock(&manager->topicReceivers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(manager->topicReceivers.map);
    while (hashMapIterator_hasNext(&iter)) {
        pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry != NULL && (entry->usageCount <= 0 || entry->needsMatch)) {
            if (manager->verbose && entry->endpoint != NULL) {
                const char *adminType = celix_properties_get(entry->endpoint, PUBSUB_ENDPOINT_ADMIN_TYPE, "!Error!");
                const char *serType = celix_properties_get(entry->endpoint, PUBSUB_ENDPOINT_SERIALIZER, "!Error!");
                logHelper_log(manager->loghelper, OSGI_LOGSERVICE_DEBUG,
                              "[PSTM] Tearing down TopicReceiver for scope/topic %s/%s with psa admin type %s and serializer %s\n",
                              entry->scope, entry->topic, adminType, serType);
            }

            if (entry->endpoint != NULL) {
				celix_bundleContext_useServiceWithId(manager->context, entry->selectedPsaSvcId,
													 PUBSUB_ADMIN_SERVICE_NAME, entry,
													 pstm_teardownTopicReceiverCallback);
				celixThreadMutex_lock(&manager->announceEndpointListeners.mutex);
				for (int i = 0; i < celix_arrayList_size(manager->announceEndpointListeners.list); ++i) {
					pubsub_announce_endpoint_listener_t *listener = celix_arrayList_get(
							manager->announceEndpointListeners.list, i);
					listener->removeEndpoint(listener->handle, entry->endpoint);
				}
				celixThreadMutex_unlock(&manager->announceEndpointListeners.mutex);
			}

			if (entry->usageCount <= 0) {
				//no usage -> remove
				hashMapIterator_remove(&iter);
				//cleanup entry
				free(entry->scopeAndTopicKey);
				free(entry->scope);
				free(entry->topic);
				if (entry->subscriberProperties != NULL) {
					celix_properties_destroy(entry->subscriberProperties);
				}
				if (entry->endpoint != NULL) {
					celix_properties_destroy(entry->endpoint);
				}
				free(entry);
			} else {
				//still usage -> setup for rematch
				if (entry->endpoint != NULL) {
					celix_properties_destroy(entry->endpoint);
				}
				entry->endpoint = NULL;
				entry->selectedPsaSvcId = -1L;
				entry->selectedSerializerSvcId = -1L;
			}
        }
    }
    celixThreadMutex_unlock(&manager->topicReceivers.mutex);
}

static void pstm_addEndpointCallback(void *handle, void *svc) {
	celix_properties_t *endpoint = handle;
	pubsub_admin_service_t *psa = svc;
	psa->addEndpoint(psa->handle, endpoint);
}

static void pstm_findPsaForEndpoints(pubsub_topology_manager_t *manager) {
	celixThreadMutex_lock(&manager->discoveredEndpoints.mutex);
	hash_map_iterator_t iter = hashMapIterator_construct(manager->discoveredEndpoints.map);
	while (hashMapIterator_hasNext(&iter)) {
		pstm_discovered_endpoint_entry_t *entry = hashMapIterator_nextValue(&iter);
		if (entry != NULL && entry->selectedPsaSvcId < 0) {
			long psaSvcId = -1L;

			celixThreadMutex_lock(&manager->pubsubadmins.mutex);
			hash_map_iterator_t iter2 = hashMapIterator_construct(manager->pubsubadmins.map);
			while (hashMapIterator_hasNext(&iter2)) {
				hash_map_entry_t *mapEntry = hashMapIterator_nextEntry(&iter2);
				pubsub_admin_service_t *psa = hashMapEntry_getValue(mapEntry);
				long svcId = (long) hashMapEntry_getKey(mapEntry);
				bool match = false;
				psa->matchEndpoint(psa->handle, entry->endpoint, &match);
				if (match) {
					psaSvcId = svcId;
					break;
				}
			}
			celixThreadMutex_unlock(&manager->pubsubadmins.mutex);

			if (psaSvcId >= 0) {
				celix_bundleContext_useServiceWithId(manager->context, psaSvcId, PUBSUB_ADMIN_SERVICE_NAME,
													 (void *)entry->endpoint, pstm_addEndpointCallback);
			} else {
				logHelper_log(manager->loghelper, OSGI_LOGSERVICE_DEBUG, "Cannot find psa for endpoint %s\n", entry->uuid);
			}

			entry->selectedPsaSvcId = psaSvcId;
		}
	}
	celixThreadMutex_unlock(&manager->discoveredEndpoints.mutex);
}


static void pstm_setupTopicSenderCallback(void *handle, void *svc) {
	pstm_topic_receiver_or_sender_entry_t *entry = handle;
	pubsub_admin_service_t *psa = svc;
	psa->setupTopicSender(psa->handle, entry->scope, entry->topic, entry->selectedSerializerSvcId, &entry->endpoint);
}

static void pstm_setupTopicSenders(pubsub_topology_manager_t *manager) {
	celixThreadMutex_lock(&manager->topicSenders.mutex);
	hash_map_iterator_t iter = hashMapIterator_construct(manager->topicSenders.map);
	while (hashMapIterator_hasNext(&iter)) {
		pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);
		if (entry != NULL && entry->needsMatch) {
			//new topic sender needed, requesting match with current psa
			double highestScore = PUBSUB_ADMIN_NO_MATCH_SCORE;
			long serializerSvcId = -1L;
			long selectedPsaSvcId = -1L;

			celixThreadMutex_lock(&manager->pubsubadmins.mutex);
			hash_map_iterator_t iter2 = hashMapIterator_construct(manager->pubsubadmins.map);
			while (hashMapIterator_hasNext(&iter2)) {
				hash_map_entry_t *mapEntry = hashMapIterator_nextEntry(&iter2);
				long svcId = (long)hashMapEntry_getKey(mapEntry);
				pubsub_admin_service_t *psa = hashMapEntry_getValue(mapEntry);
				double score = PUBSUB_ADMIN_NO_MATCH_SCORE;
				long serSvcId = -1L;
				psa->matchPublisher(psa->handle, entry->bndId, entry->publisherFilter, &score, &serSvcId);
				if (score > highestScore) {
					highestScore = score;
					serializerSvcId = serSvcId;
                    selectedPsaSvcId = svcId;
				}
			}
			celixThreadMutex_unlock(&manager->pubsubadmins.mutex);

			if (highestScore > PUBSUB_ADMIN_NO_MATCH_SCORE) {
			    entry->selectedPsaSvcId = selectedPsaSvcId;
			    entry->selectedSerializerSvcId = serializerSvcId;
				bool called = celix_bundleContext_useServiceWithId(manager->context, selectedPsaSvcId, PUBSUB_ADMIN_SERVICE_NAME, entry, pstm_setupTopicSenderCallback);

				if (called && entry->endpoint != NULL) {
					entry->needsMatch = false;

					//announce new endpoint through the network
					celixThreadMutex_lock(&manager->announceEndpointListeners.mutex);
					for (int i = 0; i < celix_arrayList_size(manager->announceEndpointListeners.list); ++i) {
						pubsub_announce_endpoint_listener_t *listener = celix_arrayList_get(manager->announceEndpointListeners.list, i);
						listener->announceEndpoint(listener->handle, entry->endpoint);
					}
					celixThreadMutex_unlock(&manager->announceEndpointListeners.mutex);
				}
			}

			if (entry->needsMatch) {
				logHelper_log(manager->loghelper, OSGI_LOGSERVICE_WARNING, "Cannot setup TopicSender for %s/%s\n", entry->scope, entry->topic);
			}
		}
	}
	celixThreadMutex_unlock(&manager->topicSenders.mutex);
}

static void pstm_setupTopicReceiverCallback(void *handle, void *svc) {
    pstm_topic_receiver_or_sender_entry_t *entry = handle;
    pubsub_admin_service_t *psa = svc;
    psa->setupTopicReceiver(psa->handle, entry->scope, entry->topic, entry->selectedSerializerSvcId, &entry->endpoint);
}

static void pstm_setupTopicReceivers(pubsub_topology_manager_t *manager) {
	celixThreadMutex_lock(&manager->topicReceivers.mutex);
	hash_map_iterator_t iter = hashMapIterator_construct(manager->topicReceivers.map);
	while (hashMapIterator_hasNext(&iter)) {
		pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);
		if (entry->needsMatch) {

			double highestScore = PUBSUB_ADMIN_NO_MATCH_SCORE;
			long serializerSvcId = -1L;
			long selectedPsaSvcId = -1L;

			celixThreadMutex_lock(&manager->pubsubadmins.mutex);
			hash_map_iterator_t iter2 = hashMapIterator_construct(manager->pubsubadmins.map);
			while (hashMapIterator_hasNext(&iter2)) {
				hash_map_entry_t *mapEntry = hashMapIterator_nextEntry(&iter2);
				long svcId = (long) hashMapEntry_getKey(mapEntry);
				pubsub_admin_service_t *psa = hashMapEntry_getValue(mapEntry);
				double score = PUBSUB_ADMIN_NO_MATCH_SCORE;
				long serSvcId = -1L;

				psa->matchSubscriber(psa->handle, entry->bndId, entry->subscriberProperties, &score, &serSvcId);
				if (score > highestScore) {
					highestScore = score;
					serializerSvcId = serSvcId;
                    selectedPsaSvcId = svcId;
				}
			}
			celixThreadMutex_unlock(&manager->pubsubadmins.mutex);

			if (highestScore > PUBSUB_ADMIN_NO_MATCH_SCORE) {
				entry->selectedPsaSvcId = selectedPsaSvcId;
				entry->selectedSerializerSvcId = serializerSvcId;

				bool called = celix_bundleContext_useServiceWithId(manager->context, selectedPsaSvcId, PUBSUB_ADMIN_SERVICE_NAME,
													 entry,
													 pstm_setupTopicReceiverCallback);

				if (called && entry->endpoint != NULL) {
				    entry->needsMatch = false;
					//announce new endpoint through the network
					celixThreadMutex_lock(&manager->announceEndpointListeners.mutex);
					for (int i = 0; i < celix_arrayList_size(manager->announceEndpointListeners.list); ++i) {
						pubsub_announce_endpoint_listener_t *listener = celix_arrayList_get(
								manager->announceEndpointListeners.list, i);
						listener->announceEndpoint(listener->handle, entry->endpoint);
					}
					celixThreadMutex_unlock(&manager->announceEndpointListeners.mutex);
				}
			}


			if (entry->needsMatch) {
				logHelper_log(manager->loghelper, OSGI_LOGSERVICE_WARNING, "Cannot setup TopicReceiver for %s/%s\n", entry->scope, entry->topic);
			}
		}
	}
	celixThreadMutex_unlock(&manager->topicReceivers.mutex);
}

static void* pstm_psaHandlingThread(void *data) {
    pubsub_topology_manager_t *manager = data;

    celixThreadMutex_lock(&manager->psaHandling.mutex);
    bool running = manager->psaHandling.running;
    celixThreadMutex_unlock(&manager->psaHandling.mutex);

    while (running) {
    	//first teardown -> also if rematch is needed
        pstm_teardownTopicSenders(manager);
        pstm_teardownTopicReceivers(manager);

        //then see if any topic sender/receiver are needed
        pstm_setupTopicSenders(manager);
        pstm_setupTopicReceivers(manager);

		pstm_findPsaForEndpoints(manager); //trying to find psa and possible set for endpoints with no psa

        celixThreadMutex_lock(&manager->psaHandling.mutex);
        celixThreadCondition_timedwaitRelative(&manager->psaHandling.cond, &manager->psaHandling.mutex, PSTM_PSA_HANDLING_SLEEPTIME_IN_SECONDS, 0L);
        running = manager->psaHandling.running;
        celixThreadMutex_unlock(&manager->psaHandling.mutex);
    }
    return NULL;
}


celix_status_t pubsub_topologyManager_shellCommand(void *handle, char * commandLine __attribute__((unused)), FILE *os, FILE *errorStream __attribute__((unused))) {
	pubsub_topology_manager_t *manager = handle;
	//TODO add support for searching based on scope and topic

	fprintf(os, "\n");

	fprintf(os, "Discovered Endpoints: \n");
	celixThreadMutex_lock(&manager->discoveredEndpoints.mutex);
	hash_map_iterator_t iter = hashMapIterator_construct(manager->discoveredEndpoints.map);
	while (hashMapIterator_hasNext(&iter)) {
		pstm_discovered_endpoint_entry_t *discovered = hashMapIterator_nextValue(&iter);
        const char *cn = celix_properties_get(discovered->endpoint, "container_name", "!Error!");
        const char *fwuuid = celix_properties_get(discovered->endpoint, PUBSUB_ENDPOINT_FRAMEWORK_UUID, "!Error!");
        const char *type = celix_properties_get(discovered->endpoint, PUBSUB_ENDPOINT_TYPE, "!Error!");
		const char *scope = celix_properties_get(discovered->endpoint, PUBSUB_ENDPOINT_TOPIC_SCOPE, "!Error!");
		const char *topic = celix_properties_get(discovered->endpoint, PUBSUB_ENDPOINT_TOPIC_NAME, "!Error!");
		const char *adminType = celix_properties_get(discovered->endpoint, PUBSUB_ENDPOINT_ADMIN_TYPE, "!Error!");
		const char *serType = celix_properties_get(discovered->endpoint, PUBSUB_ENDPOINT_SERIALIZER, "!Error!");
		fprintf(os, "|- Discovered Endpoint %s:\n", discovered->uuid);
        fprintf(os, "   |- container name = %s\n", cn);
        fprintf(os, "   |- fw uuid        = %s\n", fwuuid);
        fprintf(os, "   |- type           = %s\n", type);
		fprintf(os, "   |- scope          = %s\n", scope);
		fprintf(os, "   |- topic          = %s\n", topic);
		fprintf(os, "   |- admin type     = %s\n", adminType);
		fprintf(os, "   |- serializer     = %s\n", serType);
		if (manager->verbose) {
			fprintf(os, "   |- psa svc id     = %li\n", discovered->selectedPsaSvcId);
			fprintf(os, "   |- usage count    = %i\n", discovered->usageCount);
		}
	}
	celixThreadMutex_unlock(&manager->discoveredEndpoints.mutex);
	fprintf(os,"\n");


	fprintf(os, "Active Topic Senders:\n");
	celixThreadMutex_lock(&manager->topicSenders.mutex);
	iter = hashMapIterator_construct(manager->topicSenders.map);
	while (hashMapIterator_hasNext(&iter)) {
		pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);
		if (entry->endpoint == NULL) {
			continue;
		}
		const char *uuid = celix_properties_get(entry->endpoint, PUBSUB_ENDPOINT_UUID, "!Error!");
		const char *adminType = celix_properties_get(entry->endpoint, PUBSUB_ENDPOINT_ADMIN_TYPE, "!Error!");
		const char *serType = celix_properties_get(entry->endpoint, PUBSUB_ENDPOINT_SERIALIZER, "!Error!");
		fprintf(os, "|- Topic Sender for endpoint %s:\n", uuid);
		fprintf(os, "   |- scope       = %s\n", entry->scope);
		fprintf(os, "   |- topic       = %s\n", entry->topic);
		fprintf(os, "   |- admin type  = %s\n", adminType);
		fprintf(os, "   |- serializer  = %s\n", serType);
		if (manager->verbose) {
			fprintf(os, "   |- psa svc id  = %li\n", entry->selectedPsaSvcId);
			fprintf(os, "   |- ser svc id  = %li\n", entry->selectedSerializerSvcId);
			fprintf(os, "   |- usage count = %i\n", entry->usageCount);
		}
	}
	celixThreadMutex_unlock(&manager->topicSenders.mutex);
	fprintf(os,"\n");

	fprintf(os, "Active Topic Receivers:\n");
	celixThreadMutex_lock(&manager->topicReceivers.mutex);
	iter = hashMapIterator_construct(manager->topicReceivers.map);
	while (hashMapIterator_hasNext(&iter)) {
		pstm_topic_receiver_or_sender_entry_t *entry = hashMapIterator_nextValue(&iter);
		if (entry->endpoint == NULL) {
			continue;
		}
		const char *uuid = celix_properties_get(entry->endpoint, PUBSUB_ENDPOINT_UUID, "!Error!");
		const char *adminType = celix_properties_get(entry->endpoint, PUBSUB_ENDPOINT_ADMIN_TYPE, "!Error!");
		const char *serType = celix_properties_get(entry->endpoint, PUBSUB_ENDPOINT_SERIALIZER, "!Error!");
		fprintf(os, "|- Topic Receiver for endpoint %s:\n", uuid);
		fprintf(os, "   |- scope       = %s\n", entry->scope);
		fprintf(os, "   |- topic       = %s\n", entry->topic);
		fprintf(os, "   |- admin type  = %s\n", adminType);
		fprintf(os, "   |- serializer  = %s\n", serType);
		if (manager->verbose) {
			fprintf(os, "    |- psa svc id  = %li\n", entry->selectedPsaSvcId);
			fprintf(os, "    |- ser svc id  = %li\n", entry->selectedSerializerSvcId);
			fprintf(os, "    |- usage count = %i\n", entry->usageCount);
		}
	}
	celixThreadMutex_unlock(&manager->topicReceivers.mutex);
	fprintf(os,"\n");

	fprintf(os, "TODO pending topic senders/receivers\n");

	return CELIX_SUCCESS;
}
