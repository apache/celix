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
 * pubsub_admin_impl.c
 *
 *  \date       Sep 30, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdio.h>
#include <stdlib.h>

#ifndef ANDROID
#include <ifaddrs.h>
#endif

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <netdb.h>

#include "constants.h"
#include "utils.h"
#include "hash_map.h"
#include "array_list.h"
#include "bundle_context.h"
#include "bundle.h"
#include "service_reference.h"
#include "service_registration.h"
#include "log_helper.h"
#include "log_service.h"
#include "celix_threads.h"
#include "service_factory.h"

#include "pubsub_admin_impl.h"
#include "topic_subscription.h"
#include "pubsub_publish_service_private.h"
#include "pubsub_endpoint.h"
#include "subscriber.h"

static const char *DEFAULT_MC_IP = "224.100.1.1";
static char *DEFAULT_MC_PREFIX = "224.100";

static celix_status_t pubsubAdmin_getIpAddress(const char* interface, char** ip);
static celix_status_t pubsubAdmin_addSubscriptionToPendingList(pubsub_admin_pt admin,pubsub_endpoint_pt subEP);
static celix_status_t pubsubAdmin_addAnySubscription(pubsub_admin_pt admin,pubsub_endpoint_pt subEP);
static celix_status_t pubsubAdmin_match(pubsub_admin_pt admin, pubsub_endpoint_pt psEP, double* score);

celix_status_t pubsubAdmin_create(bundle_context_pt context, pubsub_admin_pt *admin) {
	celix_status_t status = CELIX_SUCCESS;

	*admin = calloc(1, sizeof(**admin));

	if (!*admin) {
		status = CELIX_ENOMEM;
	}
	else{

		char *mc_ip = NULL;
		char *if_ip = NULL;
		(*admin)->bundle_context= context;
		(*admin)->localPublications = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		(*admin)->subscriptions = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		(*admin)->pendingSubscriptions = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		(*admin)->externalPublications = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

		celixThreadMutex_create(&(*admin)->localPublicationsLock, NULL);
		celixThreadMutex_create(&(*admin)->subscriptionsLock, NULL);
		celixThreadMutex_create(&(*admin)->pendingSubscriptionsLock, NULL);
		celixThreadMutex_create(&(*admin)->externalPublicationsLock, NULL);

		if (logHelper_create(context, &(*admin)->loghelper) == CELIX_SUCCESS) {
			logHelper_start((*admin)->loghelper);
		}
		const char *mc_ip_prop = NULL;
		bundleContext_getProperty(context,PSA_IP , &mc_ip_prop);
		if(mc_ip_prop) {
		    mc_ip = strdup(mc_ip_prop);
		}
#ifndef ANDROID
		if (mc_ip == NULL) {
		    const char *mc_prefix = NULL;
            const char *interface = NULL;
            int b0 = 224, b1 = 100, b2 = 1, b3 = 1;
            bundleContext_getProperty(context,PSA_MULTICAST_IP_PREFIX , &mc_prefix);
            if(mc_prefix == NULL) {
                mc_prefix = DEFAULT_MC_PREFIX;
            }

			bundleContext_getProperty(context, PSA_ITF, &interface);
			if (pubsubAdmin_getIpAddress(interface, &if_ip) != CELIX_SUCCESS) {
				logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_WARNING, "PSA: Could not retrieve IP address for interface %s", interface);
			}

			printf("IP Detected : %s\n", if_ip);
			if(if_ip && sscanf(if_ip, "%i.%i.%i.%i", &b0, &b1, &b2, &b3) != 4) {
			    logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_WARNING, "PSA: Could not parse IP address %s", if_ip);
			    b2 = 1;
			    b3 = 1;
			}

            asprintf(&mc_ip, "%s.%d.%d",mc_prefix, b2, b3);

	        int sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
	        if(sendSocket == -1) {
	            perror("pubsubAdmin_create:socket");
	            status = CELIX_SERVICE_EXCEPTION;
	        }

	        if (status == CELIX_SUCCESS){
				char loop = 1;
				if(setsockopt(sendSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) != 0) {
					perror("pubsubAdmin_create:setsockopt(IP_MULTICAST_LOOP)");
					status = CELIX_SERVICE_EXCEPTION;
				}

				if (status == CELIX_SUCCESS){
					struct in_addr multicast_interface;
					inet_aton(if_ip, &multicast_interface);
					if(setsockopt(sendSocket,  IPPROTO_IP, IP_MULTICAST_IF, &multicast_interface, sizeof(multicast_interface)) != 0) {
						perror("pubsubAdmin_create:setsockopt(IP_MULTICAST_IF)");
						status = CELIX_SERVICE_EXCEPTION;
					}

					(*admin)->sendSocket = sendSocket;
				}

	        }

		}
#endif
        if (if_ip != NULL) {
            logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_INFO, "PSA: Using %s as interface for multicast communication", if_ip);
            (*admin)->ifIpAddress = if_ip;
        } else {
            (*admin)->ifIpAddress = strdup("127.0.0.1");
        }

		if (mc_ip != NULL) {
			logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_INFO, "PSA: Using %s for service annunciation", mc_ip);
			(*admin)->mcIpAddress = mc_ip;
		}
		else {
			logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_WARNING, "PSA: No IP address for service annunciation set. Using %s", DEFAULT_MC_IP);
			(*admin)->mcIpAddress = strdup(DEFAULT_MC_IP);
		}

		if (status != CELIX_SUCCESS){
			pubsubAdmin_destroy(*admin);
		}

	}

	return status;
}


celix_status_t pubsubAdmin_destroy(pubsub_admin_pt admin)
{
	celix_status_t status = CELIX_SUCCESS;

	free(admin->mcIpAddress);
    free(admin->ifIpAddress);

	celixThreadMutex_lock(&admin->pendingSubscriptionsLock);
	hash_map_iterator_pt iter = hashMapIterator_create(admin->pendingSubscriptions);
	while(hashMapIterator_hasNext(iter)){
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		free((char*)hashMapEntry_getKey(entry));
		arrayList_destroy((array_list_pt)hashMapEntry_getValue(entry));
	}
	hashMapIterator_destroy(iter);
	hashMap_destroy(admin->pendingSubscriptions,false,false);
	celixThreadMutex_unlock(&admin->pendingSubscriptionsLock);

	celixThreadMutex_lock(&admin->subscriptionsLock);
	hashMap_destroy(admin->subscriptions,false,false);
	celixThreadMutex_unlock(&admin->subscriptionsLock);

	celixThreadMutex_lock(&admin->localPublicationsLock);
	hashMap_destroy(admin->localPublications,true,false);
	celixThreadMutex_unlock(&admin->localPublicationsLock);

	celixThreadMutex_lock(&admin->externalPublicationsLock);
	iter = hashMapIterator_create(admin->externalPublications);
	while(hashMapIterator_hasNext(iter)){
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		free((char*)hashMapEntry_getKey(entry));
		arrayList_destroy((array_list_pt)hashMapEntry_getValue(entry));
	}
	hashMapIterator_destroy(iter);
	hashMap_destroy(admin->externalPublications,false,false);
	celixThreadMutex_unlock(&admin->externalPublicationsLock);

	celixThreadMutex_destroy(&admin->pendingSubscriptionsLock);
	celixThreadMutex_destroy(&admin->subscriptionsLock);
	celixThreadMutex_destroy(&admin->localPublicationsLock);
	celixThreadMutex_destroy(&admin->externalPublicationsLock);

	logHelper_stop(admin->loghelper);

	logHelper_destroy(&admin->loghelper);

	free(admin);

	return status;
}

celix_status_t pubsubAdmin_stop(pubsub_admin_pt admin) {
	celix_status_t status = CELIX_SUCCESS;

	return status;
}

static celix_status_t pubsubAdmin_addAnySubscription(pubsub_admin_pt admin,pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&admin->subscriptionsLock);

	topic_subscription_pt any_sub = hashMap_get(admin->subscriptions,PUBSUB_ANY_SUB_TOPIC);

	if(any_sub==NULL){

		int i;

		status += pubsub_topicSubscriptionCreate(admin->ifIpAddress, admin->bundle_context, PUBSUB_SUBSCRIBER_SCOPE_DEFAULT, PUBSUB_ANY_SUB_TOPIC,&any_sub);

		/* Connect all internal publishers */
		celixThreadMutex_lock(&admin->localPublicationsLock);
		hash_map_iterator_pt lp_iter =hashMapIterator_create(admin->localPublications);
		while(hashMapIterator_hasNext(lp_iter)){
			service_factory_pt factory = (service_factory_pt)hashMapIterator_nextValue(lp_iter);
			topic_publication_pt topic_pubs = (topic_publication_pt)factory->handle;
			array_list_pt topic_publishers = pubsub_topicPublicationGetPublisherList(topic_pubs);

			if(topic_publishers!=NULL){
				for(i=0;i<arrayList_size(topic_publishers);i++){
					pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(topic_publishers,i);
					if(pubEP->endpoint !=NULL){
						status += pubsub_topicSubscriptionConnectPublisher(any_sub,pubEP->endpoint);
					}
				}
			}
		}
		hashMapIterator_destroy(lp_iter);
		celixThreadMutex_unlock(&admin->localPublicationsLock);

		/* Connect also all external publishers */
		celixThreadMutex_lock(&admin->externalPublicationsLock);
		hash_map_iterator_pt extp_iter =hashMapIterator_create(admin->externalPublications);
		while(hashMapIterator_hasNext(extp_iter)){
			array_list_pt ext_pub_list = (array_list_pt)hashMapIterator_nextValue(extp_iter);
			if(ext_pub_list!=NULL){
				for(i=0;i<arrayList_size(ext_pub_list);i++){
					pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(ext_pub_list,i);
					if(pubEP->endpoint !=NULL){
						status += pubsub_topicSubscriptionConnectPublisher(any_sub,pubEP->endpoint);
					}
				}
			}
		}
		hashMapIterator_destroy(extp_iter);
		celixThreadMutex_unlock(&admin->externalPublicationsLock);


		pubsub_topicSubscriptionAddSubscriber(any_sub,subEP);

		status += pubsub_topicSubscriptionStart(any_sub);

		hashMap_put(admin->subscriptions,strdup(PUBSUB_ANY_SUB_TOPIC),any_sub);

	}

	celixThreadMutex_unlock(&admin->subscriptionsLock);

	return status;
}

celix_status_t pubsubAdmin_addSubscription(pubsub_admin_pt admin,pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

	printf("PSA: Received subscription [FWUUID=%s bundleID=%ld scope=%s, topic=%s]\n",subEP->frameworkUUID,subEP->serviceID,subEP->scope,subEP->topic);

	if(strcmp(subEP->topic,PUBSUB_ANY_SUB_TOPIC)==0){
		return pubsubAdmin_addAnySubscription(admin,subEP);
	}

	/* Check if we already know some publisher about this topic, otherwise let's put the subscription in the pending hashmap */
	celixThreadMutex_lock(&admin->localPublicationsLock);
	celixThreadMutex_lock(&admin->externalPublicationsLock);
	char* scope_topic = createScopeTopicKey(subEP->scope,subEP->topic);

	service_factory_pt factory = (service_factory_pt)hashMap_get(admin->localPublications,scope_topic);
	array_list_pt ext_pub_list = (array_list_pt)hashMap_get(admin->externalPublications,scope_topic);

	if(factory==NULL && ext_pub_list==NULL){ //No (local or external) publishers yet for this topic
		celixThreadMutex_lock(&admin->pendingSubscriptionsLock);
		pubsubAdmin_addSubscriptionToPendingList(admin,subEP);
		celixThreadMutex_unlock(&admin->pendingSubscriptionsLock);
	}
	else{
		int i;

		topic_subscription_pt subscription = hashMap_get(admin->subscriptions, scope_topic);

		if(subscription == NULL) {

			status += pubsub_topicSubscriptionCreate(admin->ifIpAddress, admin->bundle_context, subEP->scope, subEP->topic,&subscription);

			/* Try to connect internal publishers */
			if(factory!=NULL){
				topic_publication_pt topic_pubs = (topic_publication_pt)factory->handle;
				array_list_pt topic_publishers = pubsub_topicPublicationGetPublisherList(topic_pubs);

				if(topic_publishers!=NULL){
					for(i=0;i<arrayList_size(topic_publishers);i++){
						pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(topic_publishers,i);
						if(pubEP->endpoint !=NULL){
							status += pubsub_topicSubscriptionConnectPublisher(subscription,pubEP->endpoint);
						}
					}
				}

			}

			/* Look also for external publishers */
			if(ext_pub_list!=NULL){
				for(i=0;i<arrayList_size(ext_pub_list);i++){
					pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(ext_pub_list,i);
					if(pubEP->endpoint !=NULL){
						status += pubsub_topicSubscriptionConnectPublisher(subscription,pubEP->endpoint);
					}
				}
			}

			pubsub_topicSubscriptionAddSubscriber(subscription,subEP);

			status += pubsub_topicSubscriptionStart(subscription);

			if(status==CELIX_SUCCESS){
				celixThreadMutex_lock(&admin->subscriptionsLock);
				hashMap_put(admin->subscriptions,strdup(scope_topic),subscription);
				celixThreadMutex_unlock(&admin->subscriptionsLock);
			}
		}
		pubsub_topicIncreaseNrSubscribers(subscription);

	}
	free(scope_topic);
	celixThreadMutex_unlock(&admin->externalPublicationsLock);
	celixThreadMutex_unlock(&admin->localPublicationsLock);

	return status;

}

celix_status_t pubsubAdmin_removeSubscription(pubsub_admin_pt admin,pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

	printf("PSA: Removing subscription [FWUUID=%s bundleID=%ld scope=%s, topic=%s]\n",subEP->frameworkUUID,subEP->serviceID,subEP->scope, subEP->topic);

	celixThreadMutex_lock(&admin->subscriptionsLock);
	char* scope_topic = createScopeTopicKey(subEP->scope, subEP->topic);
	topic_subscription_pt sub = (topic_subscription_pt)hashMap_get(admin->subscriptions,scope_topic);
	free(scope_topic);
	if(sub!=NULL){
		pubsub_topicDecreaseNrSubscribers(sub);
		if(pubsub_topicGetNrSubscribers(sub) == 0) {
			status = pubsub_topicSubscriptionRemoveSubscriber(sub,subEP);
		}
	}
	else{
		status = CELIX_ILLEGAL_STATE;
	}

	celixThreadMutex_unlock(&admin->subscriptionsLock);

	return status;

}

celix_status_t pubsubAdmin_addPublication(pubsub_admin_pt admin,pubsub_endpoint_pt pubEP){
	celix_status_t status = CELIX_SUCCESS;

	printf("PSA: Received publication [FWUUID=%s bundleID=%ld scope=%s, topic=%s]\n",pubEP->frameworkUUID,pubEP->serviceID,pubEP->scope, pubEP->topic);

	const char* fwUUID = NULL;

	bundleContext_getProperty(admin->bundle_context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);
	if(fwUUID==NULL){
		printf("PSA: Cannot retrieve fwUUID.\n");
		return CELIX_INVALID_BUNDLE_CONTEXT;
	}
	char* scope_topic = createScopeTopicKey(pubEP->scope, pubEP->topic);

	if((strcmp(pubEP->frameworkUUID,fwUUID)==0) && (pubEP->endpoint==NULL)){

		celixThreadMutex_lock(&admin->localPublicationsLock);


		service_factory_pt factory = (service_factory_pt)hashMap_get(admin->localPublications,scope_topic);

		if (factory == NULL) {
			topic_publication_pt pub = NULL;
			status = pubsub_topicPublicationCreate(admin->sendSocket, pubEP,admin->mcIpAddress,&pub);
			if(status == CELIX_SUCCESS){
				status = pubsub_topicPublicationStart(admin->bundle_context,pub,&factory);
				if(status==CELIX_SUCCESS && factory !=NULL){
					hashMap_put(admin->localPublications,strdup(scope_topic),factory);
				}
			}
			else{
				printf("PSA: Cannot create a topicPublication for topic %s (bundle %ld).\n",pubEP->topic,pubEP->serviceID);
			}
		}
		else{
			//just add the new EP to the list
			topic_publication_pt pub = (topic_publication_pt)factory->handle;
			pubsub_topicPublicationAddPublisherEP(pub,pubEP);
		}


		celixThreadMutex_unlock(&admin->localPublicationsLock);
	}
	else{
		celixThreadMutex_lock(&admin->externalPublicationsLock);
		array_list_pt ext_pub_list = (array_list_pt)hashMap_get(admin->externalPublications,scope_topic);
		if(ext_pub_list==NULL){
			arrayList_create(&ext_pub_list);
			hashMap_put(admin->externalPublications,strdup(scope_topic),ext_pub_list);
		}

		arrayList_add(ext_pub_list,pubEP);

		celixThreadMutex_unlock(&admin->externalPublicationsLock);
	}

    /* Connect the new publisher to the subscription for his topic, if there is any */
    celixThreadMutex_lock(&admin->subscriptionsLock);

    topic_subscription_pt sub = (topic_subscription_pt)hashMap_get(admin->subscriptions,scope_topic);
    if(sub!=NULL && pubEP->endpoint!=NULL){
        pubsub_topicSubscriptionConnectPublisher(sub,pubEP->endpoint);
    }

    /* And check also for ANY subscription */
    topic_subscription_pt any_sub = (topic_subscription_pt)hashMap_get(admin->subscriptions,PUBSUB_ANY_SUB_TOPIC);
    if(any_sub!=NULL && pubEP->endpoint!=NULL){
        pubsub_topicSubscriptionConnectPublisher(any_sub,pubEP->endpoint);
    }

    celixThreadMutex_unlock(&admin->subscriptionsLock);

	/* Re-evaluate the pending subscriptions */
	celixThreadMutex_lock(&admin->pendingSubscriptionsLock);

	hash_map_entry_pt pendingSub = hashMap_getEntry(admin->pendingSubscriptions,scope_topic);
	if(pendingSub!=NULL){ //There were pending subscription for the just published topic. Let's connect them.
		char* key = (char*)hashMapEntry_getKey(pendingSub);
		array_list_pt pendingSubList = (array_list_pt)hashMapEntry_getValue(pendingSub);
		int i;
		for(i=0;i<arrayList_size(pendingSubList);i++){
			pubsub_endpoint_pt subEP = (pubsub_endpoint_pt)arrayList_get(pendingSubList,i);
			pubsubAdmin_addSubscription(admin,subEP);
		}
		hashMap_remove(admin->pendingSubscriptions,key);
		arrayList_clear(pendingSubList);
		arrayList_destroy(pendingSubList);
		free(key);
	}
    free(scope_topic);

    celixThreadMutex_unlock(&admin->pendingSubscriptionsLock);

	return status;

}

celix_status_t pubsubAdmin_removePublication(pubsub_admin_pt admin,pubsub_endpoint_pt pubEP){
	celix_status_t status = CELIX_SUCCESS;

	printf("PSA: Removing publication [FWUUID=%s bundleID=%ld scope=%s, topic=%s]\n",pubEP->frameworkUUID,pubEP->serviceID,pubEP->scope, pubEP->topic);

	const char* fwUUID = NULL;

	bundleContext_getProperty(admin->bundle_context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);
	if(fwUUID==NULL){
		printf("PSA: Cannot retrieve fwUUID.\n");
		return CELIX_INVALID_BUNDLE_CONTEXT;
	}
	char *scope_topic = createScopeTopicKey(pubEP->scope, pubEP->topic);

	if(strcmp(pubEP->frameworkUUID,fwUUID)==0){

		celixThreadMutex_lock(&admin->localPublicationsLock);

		service_factory_pt factory = (service_factory_pt)hashMap_get(admin->localPublications,scope_topic);
		if(factory!=NULL){
			topic_publication_pt pub = (topic_publication_pt)factory->handle;
			pubsub_topicPublicationRemovePublisherEP(pub,pubEP);
		}
		else{
			status = CELIX_ILLEGAL_STATE;
		}

		celixThreadMutex_unlock(&admin->localPublicationsLock);
	}
	else{

		celixThreadMutex_lock(&admin->externalPublicationsLock);
		array_list_pt ext_pub_list = (array_list_pt)hashMap_get(admin->externalPublications,scope_topic);
		if(ext_pub_list!=NULL){
			int i;
			bool found = false;
			for(i=0;!found && i<arrayList_size(ext_pub_list);i++){
				pubsub_endpoint_pt p  = (pubsub_endpoint_pt)arrayList_get(ext_pub_list,i);
				found = pubsubEndpoint_equals(pubEP,p);
				if(found){
					arrayList_remove(ext_pub_list,i);
				}
			}
            if(arrayList_size(ext_pub_list)==0){
                hash_map_entry_pt entry = hashMap_getEntry(admin->externalPublications,scope_topic);
                char* topic = (char*)hashMapEntry_getKey(entry);
                array_list_pt list = (array_list_pt)hashMapEntry_getValue(entry);
                hashMap_remove(admin->externalPublications,scope_topic);
                arrayList_destroy(list);
                free(topic);
            }
		}

		celixThreadMutex_unlock(&admin->externalPublicationsLock);
	}

	/* Check if this publisher was connected to one of our subscribers*/
	celixThreadMutex_lock(&admin->subscriptionsLock);

	topic_subscription_pt sub = (topic_subscription_pt)hashMap_get(admin->subscriptions,scope_topic);
	if(sub!=NULL && pubEP->endpoint!=NULL){
		pubsub_topicSubscriptionDisconnectPublisher(sub,pubEP->endpoint);
	}

	/* And check also for ANY subscription */
	topic_subscription_pt any_sub = (topic_subscription_pt)hashMap_get(admin->subscriptions,PUBSUB_ANY_SUB_TOPIC);
	if(any_sub!=NULL && pubEP->endpoint!=NULL){
		pubsub_topicSubscriptionDisconnectPublisher(any_sub,pubEP->endpoint);
	}
	free(scope_topic);
	celixThreadMutex_unlock(&admin->subscriptionsLock);

	return status;

}

celix_status_t pubsubAdmin_closeAllPublications(pubsub_admin_pt admin,char *scope, char* topic){
	celix_status_t status = CELIX_SUCCESS;

	printf("PSA: Closing all publications for scope=%s,topic=%s\n", scope, topic);

	celixThreadMutex_lock(&admin->localPublicationsLock);
	char* scope_topic =createScopeTopicKey(scope, topic);
	hash_map_entry_pt pubsvc_entry = (hash_map_entry_pt)hashMap_getEntry(admin->localPublications,scope_topic);
	if(pubsvc_entry!=NULL){
		char* key = (char*)hashMapEntry_getKey(pubsvc_entry);
		service_factory_pt factory= (service_factory_pt)hashMapEntry_getValue(pubsvc_entry);
		topic_publication_pt pub = (topic_publication_pt)factory->handle;
		status += pubsub_topicPublicationStop(pub);
		status += pubsub_topicPublicationDestroy(pub);

		hashMap_remove(admin->localPublications,scope_topic);
		free(key);
		free(factory);
	}
	free(scope_topic);
	celixThreadMutex_unlock(&admin->localPublicationsLock);

	return status;

}

celix_status_t pubsubAdmin_closeAllSubscriptions(pubsub_admin_pt admin,char *scope, char* topic){
	celix_status_t status = CELIX_SUCCESS;

	printf("PSA: Closing all subscriptions\n");

	celixThreadMutex_lock(&admin->subscriptionsLock);
	char* scope_topic =createScopeTopicKey(scope, topic);
	hash_map_entry_pt sub_entry = (hash_map_entry_pt)hashMap_getEntry(admin->subscriptions,scope_topic);
	if(sub_entry!=NULL){
		char* topic = (char*)hashMapEntry_getKey(sub_entry);

		topic_subscription_pt ts = (topic_subscription_pt)hashMapEntry_getValue(sub_entry);
		status += pubsub_topicSubscriptionStop(ts);
		status += pubsub_topicSubscriptionDestroy(ts);

		hashMap_remove(admin->subscriptions,topic);
		free(topic);

	}
	free(scope_topic);
	celixThreadMutex_unlock(&admin->subscriptionsLock);

	return status;

}

celix_status_t pubsubAdmin_matchPublisher(pubsub_admin_pt admin, pubsub_endpoint_pt pubEP, double* score){
	celix_status_t status = CELIX_SUCCESS;
	status = pubsubAdmin_match(admin, pubEP, score);
	return status;
}

celix_status_t pubsubAdmin_matchSubscriber(pubsub_admin_pt admin, pubsub_endpoint_pt subEP, double* score){
	celix_status_t status = CELIX_SUCCESS;
	status = pubsubAdmin_match(admin, subEP, score);
	return status;
}

static celix_status_t pubsubAdmin_match(pubsub_admin_pt admin, pubsub_endpoint_pt psEP, double* score){
	celix_status_t status = CELIX_SUCCESS;

	char topic_psa_prop[1024];
	snprintf(topic_psa_prop, 1024, "%s.psa", psEP->topic);

	const char* psa_to_use = NULL;
	bundleContext_getPropertyWithDefault(admin->bundle_context, topic_psa_prop, PSA_DEFAULT, &psa_to_use);

	*score = 0;
	if (strcmp(psa_to_use, "udp") == 0){
		*score += 100;
	}else{
		*score += 1;
	}

	return status;
}

#ifndef ANDROID
static celix_status_t pubsubAdmin_getIpAddress(const char* interface, char** ip) {
	celix_status_t status = CELIX_BUNDLE_EXCEPTION;

	struct ifaddrs *ifaddr, *ifa;
	char host[NI_MAXHOST];

	if (getifaddrs(&ifaddr) != -1)
	{
		for (ifa = ifaddr; ifa != NULL && status != CELIX_SUCCESS; ifa = ifa->ifa_next)
		{
			if (ifa->ifa_addr == NULL)
				continue;

			if ((getnameinfo(ifa->ifa_addr,sizeof(struct sockaddr_in), host, NI_MAXHOST, NULL, 0, NI_NUMERICHOST) == 0) && (ifa->ifa_addr->sa_family == AF_INET)) {
				if (interface == NULL) {
					*ip = strdup(host);
					status = CELIX_SUCCESS;
				}
				else if (strcmp(ifa->ifa_name, interface) == 0) {
					*ip = strdup(host);
					status = CELIX_SUCCESS;
				}
			}
		}

		freeifaddrs(ifaddr);
	}
	if(status == CELIX_SUCCESS) {

	}
	return status;
}
#endif

static celix_status_t pubsubAdmin_addSubscriptionToPendingList(pubsub_admin_pt admin,pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

	char* scope_topic =createScopeTopicKey(subEP->scope, subEP->topic);
	array_list_pt pendingListPerTopic = hashMap_get(admin->pendingSubscriptions,scope_topic);
	if(pendingListPerTopic==NULL){
		arrayList_create(&pendingListPerTopic);
		hashMap_put(admin->pendingSubscriptions,scope_topic,pendingListPerTopic);
	} else {
	    free(scope_topic);
	}
	arrayList_add(pendingListPerTopic,subEP);

	return status;
}
