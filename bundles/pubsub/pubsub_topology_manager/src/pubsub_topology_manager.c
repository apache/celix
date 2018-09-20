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
 * pubsub_topology_manager.c
 *
 *  \date       Sep 29, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <celix_api.h>

#include "hash_map.h"
#include "array_list.h"
#include "bundle_context.h"
#include "constants.h"
#include "listener_hook_service.h"
#include "utils.h"
#include "log_service.h"
#include "log_helper.h"

#include "pubsub_listeners.h"
#include "pubsub_topology_manager.h"
#include "pubsub_admin.h"

static void print_endpoint_info(hash_map_pt endpoints, FILE *outStream) {
	for(hash_map_iterator_t iter = hashMapIterator_construct(endpoints); hashMapIterator_hasNext(&iter);) {
		const char* key = (const char*)hashMapIterator_nextKey(&iter);
		fprintf(outStream, "    Topic=%s\n", key);
		array_list_pt ep_list = hashMap_get(endpoints, key);
		for(unsigned int i = 0; i < arrayList_size(ep_list); ++i) {
			pubsub_endpoint_pt ep = arrayList_get(ep_list, i);
			fprintf(outStream, "        Endpoint %d\n", i);
			fprintf(outStream, "            Endpoint properties\n");
			const char *propKey;
			if(ep->properties) {
				PROPERTIES_FOR_EACH(ep->properties, propKey) {
					fprintf(outStream, "                %s => %s\n", propKey, celix_properties_get(ep->properties, propKey, NULL));
				}
			}
		}
	}

}

static celix_status_t shellCommand(void *handle, char * commandLine, FILE *outStream, FILE *errorStream) {
	pubsub_topology_manager_t *manager = (pubsub_topology_manager_t*) handle;
	if (manager->publications && !hashMap_isEmpty(manager->publications)) {
		fprintf(outStream, "Publications:\n");
		print_endpoint_info(manager->publications, outStream);
	}
	if (manager->subscriptions && !hashMap_isEmpty(manager->subscriptions)) {
		fprintf(outStream, "Subscriptions:\n");
		print_endpoint_info(manager->subscriptions, outStream);
	}
	return CELIX_SUCCESS;
}

celix_status_t pubsub_topologyManager_create(bundle_context_pt context, log_helper_pt logHelper, pubsub_topology_manager_t **manager) {
	celix_status_t status = CELIX_SUCCESS;

	*manager = calloc(1, sizeof(**manager));
	if (!*manager) {
		return CELIX_ENOMEM;
	}

	(*manager)->context = context;

	celix_thread_mutexattr_t psaAttr;
	celixThreadMutexAttr_create(&psaAttr);
	celixThreadMutexAttr_settype(&psaAttr, CELIX_THREAD_MUTEX_RECURSIVE);
	status |= celixThreadMutex_create(&(*manager)->psaListLock, &psaAttr);
	celixThreadMutexAttr_destroy(&psaAttr);

	status |= celixThreadMutex_create(&(*manager)->publicationsLock, NULL);
	status |= celixThreadMutex_create(&(*manager)->subscriptionsLock, NULL);
	status |= celixThreadMutex_create(&(*manager)->discoveryListLock, NULL);

	arrayList_create(&(*manager)->psaList);

	(*manager)->discoveryList = hashMap_create(NULL, NULL, NULL, NULL);
	(*manager)->publications = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	(*manager)->subscriptions = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

	(*manager)->loghelper = logHelper;
	(*manager)->shellCmdService.handle = *manager;
	(*manager)->shellCmdService.executeCommand = shellCommand;

	(*manager)->verbose = PUBSUB_TOPOLOGY_MANAGER_DEFAULT_VERBOSE;
	const char *verboseStr = NULL;
	bundleContext_getProperty(context, PUBSUB_TOPOLOGY_MANAGER_VERBOSE_KEY, &verboseStr);
	if (verboseStr != NULL) {
		(*manager)->verbose = strncasecmp("true", verboseStr, strlen("true")) == 0;
	}

	properties_pt shellProps = properties_create();
	properties_set(shellProps, OSGI_SHELL_COMMAND_NAME, "ps_info");
	properties_set(shellProps, OSGI_SHELL_COMMAND_USAGE, "ps_info");
	properties_set(shellProps, OSGI_SHELL_COMMAND_DESCRIPTION, "ps_info: Overview of PubSub");
	bundleContext_registerService(context, OSGI_SHELL_COMMAND_SERVICE_NAME, &((*manager)->shellCmdService), shellProps, &((*manager)->shellCmdReg));
	return status;
}

celix_status_t pubsub_topologyManager_destroy(pubsub_topology_manager_t *manager) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&manager->discoveryListLock);
	hashMap_destroy(manager->discoveryList, false, false);
	celixThreadMutex_unlock(&manager->discoveryListLock);
	celixThreadMutex_destroy(&manager->discoveryListLock);

	celixThreadMutex_lock(&manager->psaListLock);
	arrayList_destroy(manager->psaList);
	celixThreadMutex_unlock(&manager->psaListLock);
	celixThreadMutex_destroy(&manager->psaListLock);

	celixThreadMutex_lock(&manager->publicationsLock);
	hash_map_iterator_pt pubit = hashMapIterator_create(manager->publications);
	while(hashMapIterator_hasNext(pubit)){
		array_list_pt l = (array_list_pt)hashMapIterator_nextValue(pubit);
		unsigned int i;
		for(i=0;i<arrayList_size(l);i++){
			pubsubEndpoint_destroy((pubsub_endpoint_pt)arrayList_get(l,i));
		}
		arrayList_destroy(l);
	}
	hashMapIterator_destroy(pubit);
	hashMap_destroy(manager->publications, true, false);
	celixThreadMutex_unlock(&manager->publicationsLock);
	celixThreadMutex_destroy(&manager->publicationsLock);

	celixThreadMutex_lock(&manager->subscriptionsLock);
	hash_map_iterator_pt subit = hashMapIterator_create(manager->subscriptions);
	while(hashMapIterator_hasNext(subit)){
		array_list_pt l = (array_list_pt)hashMapIterator_nextValue(subit);
		unsigned int i;
		for(i=0;i<arrayList_size(l);i++){
			pubsubEndpoint_destroy((pubsub_endpoint_pt)arrayList_get(l,i));
		}
		arrayList_destroy(l);
	}
	hashMapIterator_destroy(subit);
	hashMap_destroy(manager->subscriptions, true, false);
	celixThreadMutex_unlock(&manager->subscriptionsLock);
	celixThreadMutex_destroy(&manager->subscriptionsLock);
	serviceRegistration_unregister(manager->shellCmdReg);
	free(manager);

	return status;
}

void pubsub_topologyManager_psaAdded(void * handle, void *svc, const celix_properties_t *props __attribute__((unused))) {
	pubsub_topology_manager_t *manager = handle;
	unsigned int i;

	pubsub_admin_service_pt psa = (pubsub_admin_service_pt) svc;
	logHelper_log(manager->loghelper, OSGI_LOGSERVICE_INFO, "PSTM: Added PSA");

	celixThreadMutex_lock(&manager->psaListLock);
	arrayList_add(manager->psaList, psa);
	celixThreadMutex_unlock(&manager->psaListLock);

	// Add already detected subscriptions to new PSA
	celixThreadMutex_lock(&manager->subscriptionsLock);
	hash_map_iterator_pt subscriptionsIterator = hashMapIterator_create(manager->subscriptions);

	//TODO FIXME no matching used, should only add unmatched subscribers ?
	//NOTE this is a bug which occurs when psa are started after bundles that uses the PSA
	while (hashMapIterator_hasNext(subscriptionsIterator)) {
		array_list_pt sub_ep_list = hashMapIterator_nextValue(subscriptionsIterator);
		for(i=0;i<arrayList_size(sub_ep_list);i++){
			psa->addSubscription(psa->admin, (pubsub_endpoint_pt)arrayList_get(sub_ep_list,i));
		}
	}

	hashMapIterator_destroy(subscriptionsIterator);

	celixThreadMutex_unlock(&manager->subscriptionsLock);

	// Add already detected publications to new PSA
	celixThreadMutex_lock(&manager->publicationsLock);
	hash_map_iterator_pt publicationsIterator = hashMapIterator_create(manager->publications);

	//TODO FIXME no matching used, should only add unmatched publications ?
	//NOTE this is a bug which occurs when psa are started after bundles that uses the PSA
	while (hashMapIterator_hasNext(publicationsIterator)) {
		array_list_pt pub_ep_list = hashMapIterator_nextValue(publicationsIterator);
		for(i=0;i<arrayList_size(pub_ep_list);i++){
			psa->addPublication(psa->admin, (pubsub_endpoint_pt)arrayList_get(pub_ep_list,i));
		}
	}

	hashMapIterator_destroy(publicationsIterator);

	celixThreadMutex_unlock(&manager->publicationsLock);
}

void pubsub_topologyManager_psaRemoved(void * handle, void *svc, const celix_properties_t *props  __attribute__((unused))) {
	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_t *manager = handle;

	pubsub_admin_service_pt psa = (pubsub_admin_service_pt) svc;

	/* Deactivate all publications */
	celixThreadMutex_lock(&manager->publicationsLock);

	hash_map_iterator_pt pubit = hashMapIterator_create(manager->publications);
	while(hashMapIterator_hasNext(pubit)){
		hash_map_entry_pt pub_entry = hashMapIterator_nextEntry(pubit);
		char* scope_topic_key = (char*)hashMapEntry_getKey(pub_entry);
		// Extract scope/topic name from key
		char scope[MAX_SCOPE_LEN];
		char topic[MAX_TOPIC_LEN];
		sscanf(scope_topic_key, "%[^:]:%s", scope, topic );
		array_list_pt pubEP_list = (array_list_pt)hashMapEntry_getValue(pub_entry);

		status = psa->closeAllPublications(psa->admin,scope,topic);

		if(status==CELIX_SUCCESS){
			celixThreadMutex_lock(&manager->discoveryListLock);
			hash_map_iterator_pt iter = hashMapIterator_create(manager->discoveryList);
			while(hashMapIterator_hasNext(iter)){
				service_reference_pt disc_sr = (service_reference_pt)hashMapIterator_nextKey(iter);
				pubsub_announce_endpoint_listener_t *disc = NULL;
				bundleContext_getService(manager->context, disc_sr, (void**) &disc);
				const char* fwUUID = NULL;
				bundleContext_getProperty(manager->context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);
				unsigned int i;
				for(i=0;i<arrayList_size(pubEP_list);i++){
					pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(pubEP_list,i);
					if(strcmp(properties_get(pubEP->properties, PUBSUB_ENDPOINT_FRAMEWORK_UUID),fwUUID)==0){
						disc->removeEndpoint(disc->handle,pubEP->properties);
					}
				}
				bundleContext_ungetService(manager->context, disc_sr, NULL);
			}
			hashMapIterator_destroy(iter);
			celixThreadMutex_unlock(&manager->discoveryListLock);
		}
	}
	hashMapIterator_destroy(pubit);

	celixThreadMutex_unlock(&manager->publicationsLock);

	/* Deactivate all subscriptions */
	celixThreadMutex_lock(&manager->subscriptionsLock);
	hash_map_iterator_pt subit = hashMapIterator_create(manager->subscriptions);
	while(hashMapIterator_hasNext(subit)){
		// TODO do some error checking
		char* scope_topic = (char*)hashMapIterator_nextKey(subit);
		char scope[MAX_TOPIC_LEN];
		char topic[MAX_TOPIC_LEN];
		memset(scope, 0 , MAX_TOPIC_LEN*sizeof(char));
		memset(topic, 0 , MAX_TOPIC_LEN*sizeof(char));
		sscanf(scope_topic, "%[^:]:%s", scope, topic );
		status += psa->closeAllSubscriptions(psa->admin,scope, topic);
	}
	hashMapIterator_destroy(subit);
	celixThreadMutex_unlock(&manager->subscriptionsLock);

	celixThreadMutex_lock(&manager->psaListLock);
	arrayList_removeElement(manager->psaList, psa);
	celixThreadMutex_unlock(&manager->psaListLock);

	logHelper_log(manager->loghelper, OSGI_LOGSERVICE_INFO, "PSTM: Removed PSA");
}

void pubsub_topologyManager_subscriberAdded(void * handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
	pubsub_topology_manager_t *manager = handle;
	//subscriber_service_pt subscriber = (subscriber_service_pt)service;

	pubsub_endpoint_pt sub = NULL;
	if(pubsubEndpoint_createFromSvc(manager->context, bnd, props,false, &sub) == CELIX_SUCCESS) {
		celixThreadMutex_lock(&manager->subscriptionsLock);
		char *sub_key = pubsubEndpoint_createScopeTopicKey(properties_get(sub->properties, PUBSUB_ENDPOINT_TOPIC_SCOPE), properties_get(sub->properties, PUBSUB_ENDPOINT_TOPIC_NAME));

		array_list_pt sub_list_by_topic = hashMap_get(manager->subscriptions,sub_key);
		if(sub_list_by_topic==NULL){
			arrayList_create(&sub_list_by_topic);
			hashMap_put(manager->subscriptions,strdup(sub_key),sub_list_by_topic);
		}
		free(sub_key);
		arrayList_add(sub_list_by_topic,sub);

		celixThreadMutex_unlock(&manager->subscriptionsLock);

		unsigned int j;
		double score = 0;
		double best_score = 0;
		pubsub_admin_service_pt best_psa = NULL;
		celixThreadMutex_lock(&manager->psaListLock);
		for(j=0;j<arrayList_size(manager->psaList);j++){
			pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,j);
			psa->matchEndpoint(psa->admin,sub,&score);
			if (score > best_score) { /* We have a new winner! */
				best_score = score;
				best_psa = psa;
			}
		}

		if (best_psa != NULL && best_score>0) {
			best_psa->addSubscription(best_psa->admin,sub);
		}

		// Inform discoveries for interest in the topic
		celixThreadMutex_lock(&manager->discoveryListLock);
		hash_map_iterator_pt iter = hashMapIterator_create(manager->discoveryList);
		while(hashMapIterator_hasNext(iter)){
			service_reference_pt disc_sr = (service_reference_pt)hashMapIterator_nextKey(iter);
			pubsub_announce_endpoint_listener_t *disc = NULL;
			bundleContext_getService(manager->context, disc_sr, (void**) &disc);
			disc->announceEndpoint(disc->handle, sub->properties);
			bundleContext_ungetService(manager->context, disc_sr, NULL);
		}
		hashMapIterator_destroy(iter);
		celixThreadMutex_unlock(&manager->discoveryListLock);

		celixThreadMutex_unlock(&manager->psaListLock);
	}
}


void pubsub_topologyManager_subscriberRemoved(void * handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
	pubsub_topology_manager_t *manager = handle;

	pubsub_endpoint_pt subcmp = NULL;
	if (pubsubEndpoint_createFromSvc(manager->context, bnd, props, false, &subcmp) == CELIX_SUCCESS){

		unsigned int j,k;

		// Inform discoveries that we not interested in the topic any more
		celixThreadMutex_lock(&manager->discoveryListLock);
		hash_map_iterator_pt iter = hashMapIterator_create(manager->discoveryList);
		while(hashMapIterator_hasNext(iter)){
			service_reference_pt disc_sr = (service_reference_pt)hashMapIterator_nextKey(iter);
			pubsub_announce_endpoint_listener_t *disc = NULL;
			bundleContext_getService(manager->context, disc_sr, (void**) &disc);
			disc->removeEndpoint(disc->handle, subcmp->properties);
			bundleContext_ungetService(manager->context, disc_sr, NULL);
		}
		hashMapIterator_destroy(iter);
		celixThreadMutex_unlock(&manager->discoveryListLock);

		celixThreadMutex_lock(&manager->subscriptionsLock);
		celixThreadMutex_lock(&manager->psaListLock);

		char *sub_key = pubsubEndpoint_createScopeTopicKey(properties_get(subcmp->properties, PUBSUB_ENDPOINT_TOPIC_SCOPE),properties_get(subcmp->properties, PUBSUB_ENDPOINT_TOPIC_NAME));
		array_list_pt sub_list_by_topic = hashMap_get(manager->subscriptions,sub_key);
		free(sub_key);
		if(sub_list_by_topic!=NULL){
			for(j=0;j<arrayList_size(sub_list_by_topic);j++){
				pubsub_endpoint_pt sub = arrayList_get(sub_list_by_topic,j);
				if(pubsubEndpoint_equals(sub,subcmp)){
					for(k=0;k<arrayList_size(manager->psaList);k++){
						/* No problem with invoking removal on all psa's, only the one that manage this topic will do something */
						pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,k);
						psa->removeSubscription(psa->admin,sub);
					}

				}
				arrayList_remove(sub_list_by_topic,j);

				/* If it was the last subscriber for this topic, tell PSA to close the ZMQ socket */
				if(arrayList_size(sub_list_by_topic)==0){
					for(k=0;k<arrayList_size(manager->psaList);k++){
						pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,k);
						psa->closeAllSubscriptions(psa->admin, (char*) properties_get(subcmp->properties, PUBSUB_ENDPOINT_TOPIC_SCOPE), (char*) properties_get(subcmp->properties, PUBSUB_ENDPOINT_TOPIC_NAME));
					}
				}

				pubsubEndpoint_destroy(sub);

			}
		}

		celixThreadMutex_unlock(&manager->psaListLock);
		celixThreadMutex_unlock(&manager->subscriptionsLock);

		pubsubEndpoint_destroy(subcmp);

	}
}

void pubsub_topologyManager_pubsubDiscoveryAdded(void* handle, void *svc, const celix_properties_t *props) {
	pubsub_topology_manager_t *manager = (pubsub_topology_manager_t *)handle;
	pubsub_announce_endpoint_listener_t *disc = svc;

	const char* fwUUID = NULL;

	bundleContext_getProperty(manager->context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);
	if(fwUUID==NULL){
		printf("PSD: ERRROR: Cannot retrieve fwUUID.\n");
		return;
	}

	celixThreadMutex_lock(&manager->publicationsLock);

	celixThreadMutex_lock(&manager->discoveryListLock);
	long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
	hashMap_put(manager->discoveryList, (void*)svcId, NULL);
	celixThreadMutex_unlock(&manager->discoveryListLock);

	hash_map_iterator_pt iter = hashMapIterator_create(manager->publications);
	while(hashMapIterator_hasNext(iter)){
		array_list_pt pubEP_list = (array_list_pt)hashMapIterator_nextValue(iter);
		for(unsigned int i = 0; i < arrayList_size(pubEP_list); i++) {
			pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(pubEP_list,i);
			if( (strcmp(properties_get(pubEP->properties, PUBSUB_ENDPOINT_FRAMEWORK_UUID),fwUUID)==0)) {
				disc->announceEndpoint(disc->handle,pubEP->properties);
			}
		}
	}
	hashMapIterator_destroy(iter);

	celixThreadMutex_unlock(&manager->publicationsLock);

	celixThreadMutex_lock(&manager->subscriptionsLock);
	iter = hashMapIterator_create(manager->subscriptions);

	while(hashMapIterator_hasNext(iter)) {
		array_list_pt l = (array_list_pt)hashMapIterator_nextValue(iter);
		unsigned int i;
		for(i=0;i<arrayList_size(l);i++){
			pubsub_endpoint_pt subEp = (pubsub_endpoint_pt)arrayList_get(l,i);

			disc->announceEndpoint(disc->handle, subEp->properties);
		}
	}
	hashMapIterator_destroy(iter);
	celixThreadMutex_unlock(&manager->subscriptionsLock);
}


void pubsub_topologyManager_pubsubDiscoveryRemoved(void * handle, void *svc, const celix_properties_t *props) {
	pubsub_topology_manager_t *manager = handle;

	celixThreadMutex_lock(&manager->discoveryListLock);


	long svcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
	if (hashMap_remove(manager->discoveryList, (void*)svcId)) {
		logHelper_log(manager->loghelper, OSGI_LOGSERVICE_INFO, "EndpointListener Removed");
	}

	celixThreadMutex_unlock(&manager->discoveryListLock);
}

static void tm_callAnnounce(void *handle, void *svc) {
    pubsub_endpoint_t *pub = handle;
    pubsub_announce_endpoint_listener_t *listener = svc;
    listener->announceEndpoint(listener->handle, pub->properties);
}

void pubsub_topologyManager_publisherTrackerAdded(void *handle, const celix_service_tracker_info_t *info) {
	pubsub_topology_manager_t *manager = handle;

	pubsub_endpoint_pt pub = NULL;
	celix_status_t status = pubsubEndpoint_createFromListenerHookInfo(manager->context, info, true, &pub);
	if (status == CELIX_SUCCESS) {

		celixThreadMutex_lock(&manager->publicationsLock);
		char *pub_key = pubsubEndpoint_createScopeTopicKey(properties_get(pub->properties, PUBSUB_ENDPOINT_TOPIC_SCOPE), properties_get(pub->properties, PUBSUB_ENDPOINT_TOPIC_NAME));
		array_list_pt pub_list_by_topic = hashMap_get(manager->publications, pub_key);
		if(pub_list_by_topic==NULL){
			arrayList_create(&pub_list_by_topic);
			hashMap_put(manager->publications,pub_key,pub_list_by_topic);
		} else {
		    free(pub_key);
        }
		arrayList_add(pub_list_by_topic,pub);

		celixThreadMutex_unlock(&manager->publicationsLock);

		unsigned int j;
		double score = 0;
		double best_score = 0;
		pubsub_admin_service_pt best_psa = NULL;
		celixThreadMutex_lock(&manager->psaListLock);

		int size = celix_arrayList_size(manager->psaList);
		for (j=0; j<size; j++) {
			pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,j);
			psa->matchEndpoint(psa->admin,pub,&score);
			if(score>best_score){ /* We have a new winner! */
				best_score = score;
				best_psa = psa;
			}
		}

		if (best_psa != NULL && best_score > 0) {
			celix_status_t status = best_psa->addPublication(best_psa->admin,pub);
			if(status==CELIX_SUCCESS){
				celixThreadMutex_lock(&manager->discoveryListLock);
				hash_map_iterator_t iter = hashMapIterator_construct(manager->discoveryList);
				while(hashMapIterator_hasNext(&iter)) {
				    long svcId = (long)hashMapIterator_nextKey(&iter);
				    celix_bundleContext_useServiceWithId(manager->context, svcId, PUBSUB_ANNOUNCE_ENDPOINT_LISTENER_SERVICE, pub, tm_callAnnounce);
				}
				celixThreadMutex_unlock(&manager->discoveryListLock);
			}
		}

		celixThreadMutex_unlock(&manager->psaListLock);

	}
}


void pubsub_topologyManager_publisherTrackerRemoved(void *handle, const celix_service_tracker_info_t *info) {
	pubsub_topology_manager_t *manager = handle;

	pubsub_endpoint_pt pubcmp = NULL;
	if(pubsubEndpoint_createFromListenerHookInfo(manager->context, info, true, &pubcmp) == CELIX_SUCCESS){
		unsigned int j,k;
		celixThreadMutex_lock(&manager->psaListLock);
		celixThreadMutex_lock(&manager->publicationsLock);

		char *pub_key = pubsubEndpoint_createScopeTopicKey(properties_get(pubcmp->properties, PUBSUB_ENDPOINT_TOPIC_SCOPE), properties_get(pubcmp->properties, PUBSUB_ENDPOINT_TOPIC_NAME));
		array_list_pt pub_list_by_topic = hashMap_get(manager->publications,pub_key);
		if(pub_list_by_topic!=NULL){
			for(j=0;j<arrayList_size(pub_list_by_topic);j++){
				pubsub_endpoint_pt pub = arrayList_get(pub_list_by_topic,j);
				if(pubsubEndpoint_equals(pub,pubcmp)){
					for(k=0;k<arrayList_size(manager->psaList);k++){
						pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,k);
						celix_status_t  status = psa->removePublication(psa->admin,pub);
						if(status==CELIX_SUCCESS){ /* We found the one that manages this endpoint */
							celixThreadMutex_lock(&manager->discoveryListLock);
							hash_map_iterator_pt iter = hashMapIterator_create(manager->discoveryList);
							while(hashMapIterator_hasNext(iter)){
								service_reference_pt disc_sr = (service_reference_pt)hashMapIterator_nextKey(iter);
								pubsub_announce_endpoint_listener_t *disc = NULL;
								bundleContext_getService(manager->context, disc_sr, (void**) &disc);
								disc->removeEndpoint(disc->handle,pub->properties);
								bundleContext_ungetService(manager->context, disc_sr, NULL);
							}
							hashMapIterator_destroy(iter);
							celixThreadMutex_unlock(&manager->discoveryListLock);
						}
						else if(status ==  CELIX_ILLEGAL_ARGUMENT){ /* Not a real error, just saying this psa does not handle this endpoint */
							status = CELIX_SUCCESS;
						}
					}
					//}
					arrayList_remove(pub_list_by_topic,j);

					/* If it was the last publisher for this topic, tell PSA to close the ZMQ socket and then inform the discovery */
					if(arrayList_size(pub_list_by_topic)==0){
						for(k=0;k<arrayList_size(manager->psaList);k++){
							pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,k);
							psa->closeAllPublications(psa->admin, (char*) properties_get(pub->properties, PUBSUB_ENDPOINT_TOPIC_SCOPE), (char*) properties_get(pub->properties, PUBSUB_ENDPOINT_TOPIC_NAME));
						}
					}

					pubsubEndpoint_destroy(pub);
				}

			}
		}

		celixThreadMutex_unlock(&manager->publicationsLock);
		celixThreadMutex_unlock(&manager->psaListLock);

		free(pub_key);

		pubsubEndpoint_destroy(pubcmp);

	}
}

static celix_status_t pubsub_topologyManager_addDiscoveredPublisher(void *handle, const celix_properties_t *pubProperties){
	celix_status_t status = CELIX_SUCCESS;
    pubsub_topology_manager_t *manager = handle;

    const char *topic = celix_properties_get(pubProperties, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
	const char *scope = celix_properties_get(pubProperties, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL);
	const char *fwUid = celix_properties_get(pubProperties, PUBSUB_ENDPOINT_FRAMEWORK_UUID, NULL);
	const char *uuid = celix_properties_get(pubProperties, PUBSUB_ENDPOINT_UUID, NULL);


    if (manager->verbose) {
        printf("PSTM: New publisher discovered for scope/topic %s/%s [fwUUID=%s, epUUID=%s]\n",
               scope, topic, fwUid, uuid);
    }


	celixThreadMutex_lock(&manager->psaListLock);
	celixThreadMutex_lock(&manager->publicationsLock);

	char *pub_key = pubsubEndpoint_createScopeTopicKey(scope, topic);

	array_list_pt pub_list_by_topic = hashMap_get(manager->publications,pub_key);
	if(pub_list_by_topic==NULL){
		arrayList_create(&pub_list_by_topic);
		hashMap_put(manager->publications,strdup(pub_key),pub_list_by_topic);
	}
	free(pub_key);

	/* Shouldn't be any other duplicate, since it's filtered out by the discovery */
	pubsub_endpoint_pt p = NULL;
	pubsubEndpoint_createFromProperties(pubProperties, &p);
	arrayList_add(pub_list_by_topic , p);

	unsigned int j;
	double score = 0;
	double best_score = 0;
	pubsub_admin_service_pt best_psa = NULL;

	for(j=0;j<arrayList_size(manager->psaList);j++){
		pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,j);
		psa->matchEndpoint(psa->admin , p, &score);
		if (score>best_score) { /* We have a new winner! */
			best_score = score;
			best_psa = psa;
		}
	}

	if(best_psa != NULL && best_score>0) {
        //TODO FIXME this the same call as used by publisher of service trackers. This is confusing.
        //remote discovered publication can be handle different.
		best_psa->addPublication(best_psa->admin,p);
	}
	else{
		status = CELIX_ILLEGAL_STATE;
	}

	celixThreadMutex_unlock(&manager->publicationsLock);
	celixThreadMutex_unlock(&manager->psaListLock);

	return status;
}

static celix_status_t pubsub_topologyManager_removeDiscoveredPublisher(void *handle, const celix_properties_t *props) {
    pubsub_topology_manager_t *manager = handle;

    if (manager->verbose) {
        printf("PSTM: Publisher removed for topic %s with scope %s [fwUUID=%s, epUUID=%s]\n",
			   celix_properties_get(props, PUBSUB_ENDPOINT_TOPIC_NAME, NULL),
               celix_properties_get(props, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL),
			   celix_properties_get(props, PUBSUB_ENDPOINT_FRAMEWORK_UUID, NULL),
			   celix_properties_get(props, PUBSUB_ENDPOINT_UUID, NULL));
    }

	celixThreadMutex_lock(&manager->psaListLock);
	celixThreadMutex_lock(&manager->publicationsLock);
	unsigned int i;

	char *pub_key = pubsubEndpoint_createScopeTopicKey(celix_properties_get(props, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL), celix_properties_get(props, PUBSUB_ENDPOINT_TOPIC_NAME, NULL));
	array_list_pt pub_list_by_topic = hashMap_get(manager->publications,pub_key);
	if(pub_list_by_topic==NULL){
		printf("PSTM: ERROR: Cannot find topic for known endpoint [%s,%s,%s]. Something is inconsistent.\n",pub_key,celix_properties_get(props, PUBSUB_ENDPOINT_FRAMEWORK_UUID, NULL),celix_properties_get(props, "pubsub.url", NULL));
	}
	else{

		pubsub_endpoint_pt p = NULL;
		bool found = false;

		for(i=0;!found && i<arrayList_size(pub_list_by_topic);i++){
			p = (pubsub_endpoint_pt)arrayList_get(pub_list_by_topic,i);
			found = pubsubEndpoint_equalsWithProperties(p,props);
		}

		if(found && p !=NULL){

			for(i=0;i<arrayList_size(manager->psaList);i++){
				pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,i);
				/* No problem with invoking removal on all psa's, only the one that manage this topic will do something */
				psa->removePublication(psa->admin,p);
			}

			arrayList_removeElement(pub_list_by_topic,p);

			/* If it was the last publisher for this topic, tell PSA to close the ZMQ socket */
			if(arrayList_size(pub_list_by_topic)==0){

				for(i=0;i<arrayList_size(manager->psaList);i++){
					pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,i);
					psa->closeAllPublications(psa->admin, (char*) celix_properties_get(p->properties, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL), (char*) celix_properties_get(p->properties, PUBSUB_ENDPOINT_TOPIC_NAME, NULL));
				}
			}

			pubsubEndpoint_destroy(p);
		}


	}
	free(pub_key);
	celixThreadMutex_unlock(&manager->publicationsLock);
	celixThreadMutex_unlock(&manager->psaListLock);


	return CELIX_SUCCESS;
}

celix_status_t pubsub_topologyManager_addDiscoveredEndpoint(void *handle, const celix_properties_t *properties) {
	const char *type = celix_properties_get(properties, PUBSUB_ENDPOINT_TYPE, NULL);
	if (type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0) {
		return pubsub_topologyManager_addDiscoveredPublisher(handle, properties);
	} else if (type != NULL && strncmp(PUBSUB_SUBSCRIBER_ENDPOINT_TYPE, type, strlen(PUBSUB_SUBSCRIBER_ENDPOINT_TYPE)) == 0) {
		//nop //TODO add subscription to pubsub admins
	} else {
		fprintf(stderr, "Invalid endpoint. Endpoint has no endpoint type (key: %s)\n", PUBSUB_ENDPOINT_TYPE);
	}
	return CELIX_SUCCESS;
}

celix_status_t pubsub_topologyManager_removeDiscoveredEndpoint(void *handle, const celix_properties_t *properties) {
	const char *type = celix_properties_get(properties, PUBSUB_ENDPOINT_TYPE, NULL);
	if (type != NULL && strncmp(PUBSUB_PUBLISHER_ENDPOINT_TYPE, type, strlen(PUBSUB_PUBLISHER_ENDPOINT_TYPE)) == 0) {
		pubsub_topologyManager_removeDiscoveredPublisher(handle, properties);
	} else if (type != NULL && strncmp(PUBSUB_SUBSCRIBER_ENDPOINT_TYPE, type, strlen(PUBSUB_SUBSCRIBER_ENDPOINT_TYPE)) == 0) {
		//nop //TODO remove subscription from pubsub admins
	} else {
		fprintf(stderr, "Invalid endpoint. Endpoint has no endpoint type (key: %s)\n", PUBSUB_ENDPOINT_TYPE);
	}
	return CELIX_SUCCESS;
}


