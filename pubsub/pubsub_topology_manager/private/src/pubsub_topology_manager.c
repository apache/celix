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

#include "hash_map.h"
#include "array_list.h"
#include "bundle_context.h"
#include "constants.h"
#include "module.h"
#include "bundle.h"
#include "remote_service_admin.h"
#include "remote_constants.h"
#include "filter.h"
#include "listener_hook_service.h"
#include "utils.h"
#include "service_reference.h"
#include "service_registration.h"
#include "log_service.h"
#include "log_helper.h"

#include "publisher_endpoint_announce.h"
#include "pubsub_topology_manager.h"
#include "pubsub_endpoint.h"
#include "pubsub_admin.h"
#include "pubsub_utils.h"


celix_status_t pubsub_topologyManager_create(bundle_context_pt context, log_helper_pt logHelper, pubsub_topology_manager_pt *manager) {
	celix_status_t status = CELIX_SUCCESS;

	*manager = calloc(1, sizeof(**manager));
	if (!*manager) {
		return CELIX_ENOMEM;
	}

	(*manager)->context = context;

	celix_thread_mutexattr_t psaAttr;
	celixThreadMutexAttr_create(&psaAttr);
	celixThreadMutexAttr_settype(&psaAttr, CELIX_THREAD_MUTEX_RECURSIVE);
	status = celixThreadMutex_create(&(*manager)->psaListLock, &psaAttr);
    celixThreadMutexAttr_destroy(&psaAttr);

    status = celixThreadMutex_create(&(*manager)->publicationsLock, NULL);
	status = celixThreadMutex_create(&(*manager)->subscriptionsLock, NULL);
	status = celixThreadMutex_create(&(*manager)->discoveryListLock, NULL);
	status = celixThreadMutex_create(&(*manager)->serializerListLock, NULL);

	arrayList_create(&(*manager)->psaList);
	arrayList_create(&(*manager)->serializerList);

	(*manager)->discoveryList = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
	(*manager)->publications = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	(*manager)->subscriptions = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

	(*manager)->loghelper = logHelper;

	return status;
}

celix_status_t pubsub_topologyManager_destroy(pubsub_topology_manager_pt manager) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&manager->discoveryListLock);
	hashMap_destroy(manager->discoveryList, false, false);
	celixThreadMutex_unlock(&manager->discoveryListLock);
	celixThreadMutex_destroy(&manager->discoveryListLock);

	celixThreadMutex_lock(&manager->psaListLock);
	arrayList_destroy(manager->psaList);
	celixThreadMutex_unlock(&manager->psaListLock);
	celixThreadMutex_destroy(&manager->psaListLock);

	celixThreadMutex_lock(&manager->serializerListLock);
	arrayList_destroy(manager->serializerList);
	celixThreadMutex_unlock(&manager->serializerListLock);
	celixThreadMutex_destroy(&manager->serializerListLock);

	celixThreadMutex_lock(&manager->publicationsLock);
	hash_map_iterator_pt pubit = hashMapIterator_create(manager->publications);
	while(hashMapIterator_hasNext(pubit)){
		array_list_pt l = (array_list_pt)hashMapIterator_nextValue(pubit);
		int i;
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
		int i;
		for(i=0;i<arrayList_size(l);i++){
			pubsubEndpoint_destroy((pubsub_endpoint_pt)arrayList_get(l,i));
		}
		arrayList_destroy(l);
	}
	hashMapIterator_destroy(subit);
	hashMap_destroy(manager->subscriptions, true, false);
	celixThreadMutex_unlock(&manager->subscriptionsLock);
	celixThreadMutex_destroy(&manager->subscriptionsLock);

	free(manager);

	return status;
}


celix_status_t pubsub_topologyManager_psaAdding(void * handle, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_pt manager = handle;

	status = bundleContext_getService(manager->context, reference, service);

	return status;
}

celix_status_t pubsub_topologyManager_psaAdded(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_pt manager = handle;
	int i, j;

	pubsub_admin_service_pt new_psa = (pubsub_admin_service_pt) service;
	logHelper_log(manager->loghelper, OSGI_LOGSERVICE_INFO, "PSTM: Added PSA");

	// Add already detected subscriptions to new PSA
	celixThreadMutex_lock(&manager->subscriptionsLock);
	hash_map_iterator_pt subscriptionsIterator = hashMapIterator_create(manager->subscriptions);

	while (hashMapIterator_hasNext(subscriptionsIterator)) {
		array_list_pt sub_ep_list = hashMapIterator_nextValue(subscriptionsIterator);
		for(i=0;i<arrayList_size(sub_ep_list);i++){
			pubsub_endpoint_pt sub = (pubsub_endpoint_pt)arrayList_get(sub_ep_list,i);
			double new_psa_score;
			new_psa->matchSubscriber(new_psa->admin, sub, &new_psa_score);
			pubsub_admin_service_pt best_psa = NULL;
			double highest_score = 0;

			for(j=0;j<arrayList_size(manager->psaList);j++){
				pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,j);
				double score;
				psa->matchSubscriber(psa->admin, sub, &score);
				if (score > highest_score){
					highest_score = score;
					best_psa = psa;
				}
			}
			if (best_psa != NULL && (new_psa_score > highest_score)){
				best_psa->removeSubscription(best_psa->admin, sub);
			}
			if (new_psa_score > highest_score){
				status += new_psa->addSubscription(new_psa->admin, sub);
			}
		}
	}

	hashMapIterator_destroy(subscriptionsIterator);

	celixThreadMutex_unlock(&manager->subscriptionsLock);

	// Add already detected publications to new PSA
	status = celixThreadMutex_lock(&manager->publicationsLock);
	hash_map_iterator_pt publicationsIterator = hashMapIterator_create(manager->publications);

	while (hashMapIterator_hasNext(publicationsIterator)) {
		array_list_pt pub_ep_list = hashMapIterator_nextValue(publicationsIterator);
		for(i=0;i<arrayList_size(pub_ep_list);i++){
			pubsub_endpoint_pt pub = (pubsub_endpoint_pt)arrayList_get(pub_ep_list,i);
			double new_psa_score;
			new_psa->matchPublisher(new_psa->admin, pub, &new_psa_score);
			pubsub_admin_service_pt best_psa = NULL;
			double highest_score = 0;

			for(j=0;j<arrayList_size(manager->psaList);j++){
				pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,j);
				double score;
				psa->matchPublisher(psa->admin, pub, &score);
				if (score > highest_score){
					highest_score = score;
					best_psa = psa;
				}
			}
			if (best_psa != NULL && (new_psa_score > highest_score)){
				best_psa->removePublication(best_psa->admin, pub);
			}
			if (new_psa_score > highest_score){
				status += new_psa->addPublication(new_psa->admin, pub);
			}
		}
	}

	hashMapIterator_destroy(publicationsIterator);

	celixThreadMutex_unlock(&manager->publicationsLock);


	celixThreadMutex_lock(&manager->serializerListLock);
	unsigned int size = arrayList_size(manager->serializerList);
	if (size > 0) {
		pubsub_serializer_service_t* ser = arrayList_get(manager->serializerList, (size-1)); //last, same as result of add/remove serializer
		new_psa->setSerializer(new_psa->admin, ser);
	}
	celixThreadMutex_unlock(&manager->serializerListLock);

	celixThreadMutex_lock(&manager->psaListLock);
	arrayList_add(manager->psaList, new_psa);
	celixThreadMutex_unlock(&manager->psaListLock);


	return status;
}

celix_status_t pubsub_topologyManager_psaModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;

	// Nop...

	return status;
}

celix_status_t pubsub_topologyManager_psaRemoved(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_pt manager = handle;

	pubsub_admin_service_pt psa = (pubsub_admin_service_pt) service;

	celixThreadMutex_lock(&manager->psaListLock);

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
				publisher_endpoint_announce_pt disc = NULL;
				bundleContext_getService(manager->context, disc_sr, (void**) &disc);
				const char* fwUUID = NULL;
				bundleContext_getProperty(manager->context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);
				int i;
				for(i=0;i<arrayList_size(pubEP_list);i++){
					pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(pubEP_list,i);
					if(strcmp(pubEP->frameworkUUID,fwUUID)==0){
						disc->removePublisher(disc->handle,pubEP);
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

	arrayList_removeElement(manager->psaList, psa);

	celixThreadMutex_unlock(&manager->psaListLock);

	logHelper_log(manager->loghelper, OSGI_LOGSERVICE_INFO, "PSTM: Removed PSA");

	return status;
}

celix_status_t pubsub_topologyManager_pubsubSerializerAdding(void* handle, service_reference_pt reference, void** service) {
	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_pt manager = handle;

	bundleContext_getService(manager->context, reference, service);

	return status;
}

celix_status_t pubsub_topologyManager_pubsubSerializerAdded(void* handle, service_reference_pt reference, void* service) {
	celix_status_t status = CELIX_SUCCESS;

	pubsub_topology_manager_pt manager = handle;
	pubsub_serializer_service_t* new_serializer = (pubsub_serializer_service_t*) service;

	celixThreadMutex_lock(&manager->serializerListLock);

	logHelper_log(manager->loghelper, OSGI_LOGSERVICE_INFO, "PSTM: Added pubsub serializer");

	int i;
	for(i=0; i<arrayList_size(manager->psaList); i++){
		pubsub_admin_service_pt psa = (pubsub_admin_service_pt) arrayList_get(manager->psaList,i);
		psa->setSerializer(psa->admin, new_serializer);
	}

	arrayList_add(manager->serializerList, new_serializer);

	celixThreadMutex_unlock(&manager->serializerListLock);

	return status;
}

celix_status_t pubsub_topologyManager_pubsubSerializerModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;

	// Nop...

	return status;
}

celix_status_t pubsub_topologyManager_pubsubSerializerRemoved(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;

	pubsub_topology_manager_pt manager = handle;
	pubsub_serializer_service_t* new_serializer = (pubsub_serializer_service_t*) service;

	celixThreadMutex_lock(&manager->serializerListLock);

	logHelper_log(manager->loghelper, OSGI_LOGSERVICE_INFO, "PSTM: Removed pubsub serializer");

	int i, j;

	for(i=0; i<arrayList_size(manager->psaList); i++){
		pubsub_admin_service_pt psa = (pubsub_admin_service_pt) arrayList_get(manager->psaList,i);
		psa->removeSerializer(psa->admin, new_serializer);
	}

	arrayList_removeElement(manager->serializerList, new_serializer);

	if (arrayList_size(manager->serializerList) > 0){
		//there is another serializer available, change the admin so it is using another serializer
		pubsub_serializer_service_t* replacing_serializer = (pubsub_serializer_service_t*) arrayList_get(manager->serializerList,0);

		for(j=0; j<arrayList_size(manager->psaList); j++){
			pubsub_admin_service_pt psa = (pubsub_admin_service_pt) arrayList_get(manager->psaList,j);
			psa->setSerializer(psa->admin, replacing_serializer);
		}
	}

	celixThreadMutex_unlock(&manager->serializerListLock);


	return status;
}

celix_status_t pubsub_topologyManager_subscriberAdding(void * handle, service_reference_pt reference, void **service) {
	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_pt manager = handle;

	status = bundleContext_getService(manager->context, reference, service);

	return status;
}

celix_status_t pubsub_topologyManager_subscriberAdded(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_pt manager = handle;
	//subscriber_service_pt subscriber = (subscriber_service_pt)service;

	pubsub_endpoint_pt sub = NULL;
	if(pubsubEndpoint_createFromServiceReference(reference,&sub) == CELIX_SUCCESS){
		celixThreadMutex_lock(&manager->subscriptionsLock);
		char *sub_key = createScopeTopicKey(sub->scope, sub->topic);

		array_list_pt sub_list_by_topic = hashMap_get(manager->subscriptions,sub_key);
		if(sub_list_by_topic==NULL){
			arrayList_create(&sub_list_by_topic);
			hashMap_put(manager->subscriptions,strdup(sub_key),sub_list_by_topic);
		}
		free(sub_key);
		arrayList_add(sub_list_by_topic,sub);

		celixThreadMutex_unlock(&manager->subscriptionsLock);

		int j;
		celixThreadMutex_lock(&manager->psaListLock);
		double highest_score = -1;
		pubsub_admin_service_pt best_psa = NULL;

		for(j=0;j<arrayList_size(manager->psaList);j++){
			pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,j);
			double score;
			psa->matchSubscriber(psa->admin, sub, &score);
			if (score > highest_score){
				highest_score = score;
				best_psa = psa;
			}
		}
		if (best_psa != NULL){
			best_psa->addSubscription(best_psa->admin,sub);
		}

		// Inform discoveries for interest in the topic
        celixThreadMutex_lock(&manager->discoveryListLock);
		hash_map_iterator_pt iter = hashMapIterator_create(manager->discoveryList);
        while(hashMapIterator_hasNext(iter)){
            service_reference_pt disc_sr = (service_reference_pt)hashMapIterator_nextKey(iter);
            publisher_endpoint_announce_pt disc = NULL;
            bundleContext_getService(manager->context, disc_sr, (void**) &disc);
            disc->interestedInTopic(disc->handle, sub->scope, sub->topic);
            bundleContext_ungetService(manager->context, disc_sr, NULL);
        }
        hashMapIterator_destroy(iter);
        celixThreadMutex_unlock(&manager->discoveryListLock);

		celixThreadMutex_unlock(&manager->psaListLock);
	}
	else{
		status = CELIX_INVALID_BUNDLE_CONTEXT;
	}

	return status;
}

celix_status_t pubsub_topologyManager_subscriberModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;

	// Nop...

	return status;
}

celix_status_t pubsub_topologyManager_subscriberRemoved(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_pt manager = handle;

	pubsub_endpoint_pt subcmp = NULL;
	if(pubsubEndpoint_createFromServiceReference(reference,&subcmp) == CELIX_SUCCESS){

		int j,k;
		celixThreadMutex_lock(&manager->psaListLock);
		celixThreadMutex_lock(&manager->subscriptionsLock);

		// Inform discoveries that we not interested in the topic any more
        celixThreadMutex_lock(&manager->discoveryListLock);
        hash_map_iterator_pt iter = hashMapIterator_create(manager->discoveryList);
        while(hashMapIterator_hasNext(iter)){
            service_reference_pt disc_sr = (service_reference_pt)hashMapIterator_nextKey(iter);
            publisher_endpoint_announce_pt disc = NULL;
            bundleContext_getService(manager->context, disc_sr, (void**) &disc);
            disc->uninterestedInTopic(disc->handle, subcmp->scope, subcmp->topic);
            bundleContext_ungetService(manager->context, disc_sr, NULL);
        }
        hashMapIterator_destroy(iter);
        celixThreadMutex_unlock(&manager->discoveryListLock);

		char *sub_key = createScopeTopicKey(subcmp->scope,subcmp->topic);
		array_list_pt sub_list_by_topic = hashMap_get(manager->subscriptions,sub_key);
		free(sub_key);
		if(sub_list_by_topic!=NULL){
			for(j=0;j<arrayList_size(sub_list_by_topic);j++){
				pubsub_endpoint_pt sub = arrayList_get(sub_list_by_topic,j);
				if(pubsubEndpoint_equals(sub,subcmp)){
					double highest_score = -1;
					pubsub_admin_service_pt best_psa = NULL;
					for(k=0;k<arrayList_size(manager->psaList);k++){
						pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,k);
						double score;
						psa->matchSubscriber(psa->admin, sub, &score);
						if (score > highest_score){
							highest_score = score;
							best_psa = psa;
						}
					}
					if (best_psa != NULL){
						best_psa->removeSubscription(best_psa->admin,sub);
					}

				}
				arrayList_remove(sub_list_by_topic,j);

				/* If it was the last subscriber for this topic, tell PSA to close the ZMQ socket */
				if(arrayList_size(sub_list_by_topic)==0){
					for(k=0;k<arrayList_size(manager->psaList);k++){
						pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,k);
						psa->closeAllSubscriptions(psa->admin,sub->scope, sub->topic);
					}
				}

				pubsubEndpoint_destroy(sub);

			}
		}

		celixThreadMutex_unlock(&manager->subscriptionsLock);
		celixThreadMutex_unlock(&manager->psaListLock);

		pubsubEndpoint_destroy(subcmp);

	}
	else{
		status = CELIX_INVALID_BUNDLE_CONTEXT;
	}

	return status;

}

celix_status_t pubsub_topologyManager_pubsubDiscoveryAdding(void* handle, service_reference_pt reference, void** service) {
	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_pt manager = handle;

	bundleContext_getService(manager->context, reference, service);

	return status;
}

celix_status_t pubsub_topologyManager_pubsubDiscoveryAdded(void* handle, service_reference_pt reference, void* service) {
	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_pt manager = (pubsub_topology_manager_pt)handle;
	publisher_endpoint_announce_pt disc = (publisher_endpoint_announce_pt)service;

	const char* fwUUID = NULL;

	bundleContext_getProperty(manager->context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);
	if(fwUUID==NULL){
		printf("PSD: ERRROR: Cannot retrieve fwUUID.\n");
		return CELIX_INVALID_BUNDLE_CONTEXT;
	}

	celixThreadMutex_lock(&manager->publicationsLock);

	celixThreadMutex_lock(&manager->discoveryListLock);
	hashMap_put(manager->discoveryList, reference, NULL);
	celixThreadMutex_unlock(&manager->discoveryListLock);

	hash_map_iterator_pt iter = hashMapIterator_create(manager->publications);
	while(hashMapIterator_hasNext(iter)){
		array_list_pt pubEP_list = (array_list_pt)hashMapIterator_nextValue(iter);
		for(int i = 0; i < arrayList_size(pubEP_list); i++) {
			pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(pubEP_list,i);
			if( (strcmp(pubEP->frameworkUUID,fwUUID)==0) && (pubEP->endpoint!=NULL)){
				status += disc->announcePublisher(disc->handle,pubEP);
			}
		}
	}
	hashMapIterator_destroy(iter);

	celixThreadMutex_unlock(&manager->publicationsLock);

	celixThreadMutex_lock(&manager->subscriptionsLock);
	iter = hashMapIterator_create(manager->subscriptions);

	while(hashMapIterator_hasNext(iter)) {
	    array_list_pt l = (array_list_pt)hashMapIterator_nextValue(iter);
	    int i;
	    for(i=0;i<arrayList_size(l);i++){
	        pubsub_endpoint_pt subEp = (pubsub_endpoint_pt)arrayList_get(l,i);

	        disc->interestedInTopic(disc->handle, subEp->scope, subEp->topic);
	    }
	}
	hashMapIterator_destroy(iter);
    celixThreadMutex_unlock(&manager->subscriptionsLock);

	return status;
}

celix_status_t pubsub_topologyManager_pubsubDiscoveryModified(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;

	status = pubsub_topologyManager_pubsubDiscoveryRemoved(handle, reference, service);
	if (status == CELIX_SUCCESS) {
		status = pubsub_topologyManager_pubsubDiscoveryAdded(handle, reference, service);
	}

	return status;
}

celix_status_t pubsub_topologyManager_pubsubDiscoveryRemoved(void * handle, service_reference_pt reference, void * service) {
	celix_status_t status = CELIX_SUCCESS;

	pubsub_topology_manager_pt manager = handle;

	celixThreadMutex_lock(&manager->discoveryListLock);


	if (hashMap_remove(manager->discoveryList, reference)) {
		logHelper_log(manager->loghelper, OSGI_LOGSERVICE_INFO, "EndpointListener Removed");
	}

	celixThreadMutex_unlock(&manager->discoveryListLock);

	return status;
}


celix_status_t pubsub_topologyManager_publisherTrackerAdded(void *handle, array_list_pt listeners) {

	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_pt manager = handle;

	int l_index;

	for (l_index = 0; l_index < arrayList_size(listeners); l_index++) {

		listener_hook_info_pt info = arrayList_get(listeners, l_index);

		const char* fwUUID=NULL;
		bundleContext_getProperty(info->context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);

		char* scope = pubsub_getScopeFromFilter(info->filter);
		char* topic = pubsub_getTopicFromFilter(info->filter);
		if(scope == NULL) {
			scope = strdup(PUBSUB_PUBLISHER_SCOPE_DEFAULT);
		}

		//TODO: Can we use a better serviceID??
		bundle_pt bundle = NULL;
		long bundleId = -1;
		bundleContext_getBundle(info->context,&bundle);
		bundle_getBundleId(bundle,&bundleId);

		if(fwUUID !=NULL && topic !=NULL){

			pubsub_endpoint_pt pub = NULL;
			if(pubsubEndpoint_create(fwUUID, scope, topic,bundleId,NULL,&pub) == CELIX_SUCCESS){

				celixThreadMutex_lock(&manager->publicationsLock);
				char *pub_key = createScopeTopicKey(scope, topic);
				array_list_pt pub_list_by_topic = hashMap_get(manager->publications, pub_key);
				if(pub_list_by_topic==NULL){
					arrayList_create(&pub_list_by_topic);
					hashMap_put(manager->publications,strdup(pub_key),pub_list_by_topic);
				}
				free(pub_key);
				arrayList_add(pub_list_by_topic,pub);

				celixThreadMutex_unlock(&manager->publicationsLock);

				int j;
				celixThreadMutex_lock(&manager->psaListLock);

				double highest_score = -1;
				pubsub_admin_service_pt best_psa = NULL;

				for(j=0;j<arrayList_size(manager->psaList);j++){
					pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,j);
					double score;
					psa->matchPublisher(psa->admin, pub, &score);
					if (score > highest_score){
						highest_score = score;
						best_psa = psa;
					}
				}
				if (best_psa != NULL){
					status = best_psa->addPublication(best_psa->admin,pub);
					if(status==CELIX_SUCCESS){
						celixThreadMutex_lock(&manager->discoveryListLock);
						hash_map_iterator_pt iter = hashMapIterator_create(manager->discoveryList);
						while(hashMapIterator_hasNext(iter)){
							service_reference_pt disc_sr = (service_reference_pt)hashMapIterator_nextKey(iter);
							publisher_endpoint_announce_pt disc = NULL;
							bundleContext_getService(manager->context, disc_sr, (void**) &disc);
							disc->announcePublisher(disc->handle,pub);
							bundleContext_ungetService(manager->context, disc_sr, NULL);
						}
						hashMapIterator_destroy(iter);
						celixThreadMutex_unlock(&manager->discoveryListLock);
					}
				}

				celixThreadMutex_unlock(&manager->psaListLock);

			}

		}
		else{
			status=CELIX_INVALID_BUNDLE_CONTEXT;
		}

		free(topic);
        free(scope);
	}

	return status;

}


celix_status_t pubsub_topologyManager_publisherTrackerRemoved(void *handle, array_list_pt listeners) {
	celix_status_t status = CELIX_SUCCESS;
	pubsub_topology_manager_pt manager = handle;

	int l_index;

	for (l_index = 0; l_index < arrayList_size(listeners); l_index++) {

		listener_hook_info_pt info = arrayList_get(listeners, l_index);

		char* pub_scope = pubsub_getScopeFromFilter(info->filter);
		char* pub_topic = pubsub_getTopicFromFilter(info->filter);

		const char* fwUUID=NULL;
		bundleContext_getProperty(info->context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);

		//TODO: Can we use a better serviceID??
		bundle_pt bundle = NULL;
		long bundleId = -1;
		bundleContext_getBundle(info->context,&bundle);
		bundle_getBundleId(bundle,&bundleId);

		if(bundle !=NULL && pub_topic !=NULL && bundleId>0){

			pubsub_endpoint_pt pubcmp = NULL;
			if(pubsubEndpoint_create(fwUUID, pub_scope, pub_topic,bundleId,NULL,&pubcmp) == CELIX_SUCCESS){

				int j,k;
                celixThreadMutex_lock(&manager->psaListLock);
                celixThreadMutex_lock(&manager->publicationsLock);

                char *pub_key = createScopeTopicKey(pub_scope, pub_topic);
				array_list_pt pub_list_by_topic = hashMap_get(manager->publications,pub_key);
				if(pub_list_by_topic!=NULL){
					for(j=0;j<arrayList_size(pub_list_by_topic);j++){
						pubsub_endpoint_pt pub = arrayList_get(pub_list_by_topic,j);
						if(pubsubEndpoint_equals(pub,pubcmp)){
							double highest_score = -1;
							pubsub_admin_service_pt best_psa = NULL;

							for(k=0;k<arrayList_size(manager->psaList);k++){
								pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,k);
								double score;
								psa->matchPublisher(psa->admin, pub, &score);
								if (score > highest_score){
									highest_score = score;
									best_psa = psa;
								}
							}
							if (best_psa != NULL){
								status = best_psa->removePublication(best_psa->admin,pub);
								if(status==CELIX_SUCCESS){
									celixThreadMutex_lock(&manager->discoveryListLock);
									hash_map_iterator_pt iter = hashMapIterator_create(manager->discoveryList);
									while(hashMapIterator_hasNext(iter)){
										service_reference_pt disc_sr = (service_reference_pt)hashMapIterator_nextKey(iter);
										publisher_endpoint_announce_pt disc = NULL;
										bundleContext_getService(manager->context, disc_sr, (void**) &disc);
										disc->removePublisher(disc->handle,pub);
										bundleContext_ungetService(manager->context, disc_sr, NULL);
									}
									hashMapIterator_destroy(iter);
									celixThreadMutex_unlock(&manager->discoveryListLock);
								}
							}
						}
						arrayList_remove(pub_list_by_topic,j);

						/* If it was the last publisher for this topic, tell PSA to close the ZMQ socket and then inform the discovery */
						if(arrayList_size(pub_list_by_topic)==0){
							for(k=0;k<arrayList_size(manager->psaList);k++){
								pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,k);
								psa->closeAllPublications(psa->admin,pub->scope, pub->topic);
							}
						}

						pubsubEndpoint_destroy(pub);

					}
				}

				celixThreadMutex_unlock(&manager->publicationsLock);
				celixThreadMutex_unlock(&manager->psaListLock);

				pubsubEndpoint_destroy(pubcmp);

				free(pub_key);

			}

		}
		else{
			status=CELIX_INVALID_BUNDLE_CONTEXT;
		}

		free(pub_scope);
		free(pub_topic);
	}

	return status;
}

celix_status_t pubsub_topologyManager_announcePublisher(void *handle, pubsub_endpoint_pt pubEP){
	celix_status_t status = CELIX_SUCCESS;
	printf("PSTM: New publisher discovered for topic %s [fwUUID=%s, ep=%s]\n",pubEP->topic,pubEP->frameworkUUID,pubEP->endpoint);

	pubsub_topology_manager_pt manager = handle;
	celixThreadMutex_lock(&manager->psaListLock);
	celixThreadMutex_lock(&manager->publicationsLock);

	int i;

	char *pub_key = createScopeTopicKey(pubEP->scope, pubEP->topic);

	array_list_pt pub_list_by_topic = hashMap_get(manager->publications,pub_key);
	if(pub_list_by_topic==NULL){
		arrayList_create(&pub_list_by_topic);
		hashMap_put(manager->publications,strdup(pub_key),pub_list_by_topic);
	}
	free(pub_key);

	/* Shouldn't be any other duplicate, since it's filtered out by the discovery */
	pubsub_endpoint_pt p = NULL;
	pubsubEndpoint_create(pubEP->frameworkUUID,pubEP->scope,pubEP->topic,pubEP->serviceID,pubEP->endpoint,&p);
	arrayList_add(pub_list_by_topic,p);

	double highest_score = -1;
	pubsub_admin_service_pt best_psa = NULL;

	for(i=0;i<arrayList_size(manager->psaList);i++){
		pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,i);
		double score;
		psa->matchPublisher(psa->admin, p, &score);
		if (score > highest_score){
			highest_score = score;
			best_psa = psa;
		}
	}
	if (best_psa != NULL){
		status += best_psa->addPublication(best_psa->admin,p);
	}

	celixThreadMutex_unlock(&manager->publicationsLock);
	celixThreadMutex_unlock(&manager->psaListLock);

	return status;
}

celix_status_t pubsub_topologyManager_removePublisher(void *handle, pubsub_endpoint_pt pubEP){
	celix_status_t status = CELIX_SUCCESS;
	printf("PSTM: Publisher removed for topic %s [fwUUID=%s, ep=%s]\n",pubEP->topic,pubEP->frameworkUUID,pubEP->endpoint);

	pubsub_topology_manager_pt manager = handle;
	celixThreadMutex_lock(&manager->psaListLock);
	celixThreadMutex_lock(&manager->publicationsLock);
	int i;

	char *pub_key = createScopeTopicKey(pubEP->scope, pubEP->topic);
	array_list_pt pub_list_by_topic = hashMap_get(manager->publications,pub_key);
	if(pub_list_by_topic==NULL){
		printf("PSTM: ERROR: Cannot find topic for known endpoint [%s,%s,%s]. Something is inconsistent.\n",pub_key,pubEP->frameworkUUID,pubEP->endpoint);
		status = CELIX_ILLEGAL_STATE;
	}
	else{

		pubsub_endpoint_pt p = NULL;
		bool found = false;

		for(i=0;!found && i<arrayList_size(pub_list_by_topic);i++){
			p = (pubsub_endpoint_pt)arrayList_get(pub_list_by_topic,i);
			found = pubsubEndpoint_equals(p,pubEP);
		}

		if(found && p !=NULL){

			double highest_score = -1;
			pubsub_admin_service_pt best_psa = NULL;

			for(i=0;i<arrayList_size(manager->psaList);i++){
				pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,i);
				double score;
				psa->matchPublisher(psa->admin, p, &score);
				if (score > highest_score){
					highest_score = score;
					best_psa = psa;
				}
			}
			if (best_psa != NULL){
				status += best_psa->removePublication(best_psa->admin,p);
			}

			arrayList_removeElement(pub_list_by_topic,p);

			/* If it was the last publisher for this topic, tell PSA to close the ZMQ socket */
			if(arrayList_size(pub_list_by_topic)==0){

				for(i=0;i<arrayList_size(manager->psaList);i++){
					pubsub_admin_service_pt psa = (pubsub_admin_service_pt)arrayList_get(manager->psaList,i);
					status += psa->closeAllPublications(psa->admin,p->scope, p->topic);
				}
			}

			pubsubEndpoint_destroy(p);
		}


	}
	free(pub_key);
	celixThreadMutex_unlock(&manager->publicationsLock);
	celixThreadMutex_unlock(&manager->psaListLock);


	return status;
}

