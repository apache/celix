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
#include <assert.h>

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
#include "topic_publication.h"
#include "pubsub_endpoint.h"
#include "pubsub/subscriber.h"
#include "pubsub_admin_match.h"

static const char *DEFAULT_MC_IP = "224.100.1.1";
static char *DEFAULT_MC_PREFIX = "224.100";

static celix_status_t pubsubAdmin_getIpAddress(const char* interface, char** ip);
static celix_status_t pubsubAdmin_addSubscriptionToPendingList(pubsub_admin_pt admin,pubsub_endpoint_pt subEP);
static celix_status_t pubsubAdmin_addAnySubscription(pubsub_admin_pt admin,pubsub_endpoint_pt subEP);

static celix_status_t pubsubAdmin_getBestSerializer(pubsub_admin_pt admin, pubsub_endpoint_pt ep, pubsub_serializer_service_t **out, const char **serType);
static void connectTopicPubSubToSerializer(pubsub_admin_pt admin,pubsub_serializer_service_t *serializer,void *topicPubSub,bool isPublication);
static void disconnectTopicPubSubFromSerializer(pubsub_admin_pt admin,void *topicPubSub,bool isPublication);

celix_status_t pubsubAdmin_create(bundle_context_pt context, pubsub_admin_pt *admin) {
	celix_status_t status = CELIX_SUCCESS;

	*admin = calloc(1, sizeof(**admin));

	if (!*admin) {
		return CELIX_ENOMEM;
	}

	char *mc_ip = NULL;
	char *if_ip = NULL;
	int sendSocket = -1;

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
		int b0 = 0, b1 = 0, b2 = 0, b3 = 0;
		bundleContext_getProperty(context,PSA_MULTICAST_IP_PREFIX , &mc_prefix);
		if(mc_prefix == NULL) {
			mc_prefix = DEFAULT_MC_PREFIX;
		}

		bundleContext_getProperty(context, PSA_ITF, &interface);
		if (pubsubAdmin_getIpAddress(interface, &if_ip) != CELIX_SUCCESS) {
			logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_WARNING, "PSA_UDP_MC: Could not retrieve IP address for interface %s", interface);
		}

		printf("IP Detected : %s\n", if_ip);
		if(if_ip && sscanf(if_ip, "%i.%i.%i.%i", &b0, &b1, &b2, &b3) != 4) {
			logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_WARNING, "PSA_UDP_MC: Could not parse IP address %s", if_ip);
			b2 = 1;
			b3 = 1;
		}

		asprintf(&mc_ip, "%s.%d.%d",mc_prefix, b2, b3);

		sendSocket = socket(AF_INET, SOCK_DGRAM, 0);
		if(sendSocket == -1) {
			perror("pubsubAdmin_create:socket");
			status = CELIX_SERVICE_EXCEPTION;
		}

		if(status == CELIX_SUCCESS){
			char loop = 1;
			if(setsockopt(sendSocket, IPPROTO_IP, IP_MULTICAST_LOOP, &loop, sizeof(loop)) != 0) {
				perror("pubsubAdmin_create:setsockopt(IP_MULTICAST_LOOP)");
				status = CELIX_SERVICE_EXCEPTION;
			}
		}

		if(status == CELIX_SUCCESS){
			struct in_addr multicast_interface;
			inet_aton(if_ip, &multicast_interface);
			if(setsockopt(sendSocket,  IPPROTO_IP, IP_MULTICAST_IF, &multicast_interface, sizeof(multicast_interface)) != 0) {
				perror("pubsubAdmin_create:setsockopt(IP_MULTICAST_IF)");
				status = CELIX_SERVICE_EXCEPTION;
			}
		}

	}


	if(status != CELIX_SUCCESS){
		logHelper_stop((*admin)->loghelper);
		logHelper_destroy(&((*admin)->loghelper));
		if(sendSocket >=0){
			close(sendSocket);
		}
		if(if_ip != NULL){
			free(if_ip);
		}
		if(mc_ip != NULL){
			free(mc_ip);
		}
		return status;
	}
	else{
		(*admin)->sendSocket = sendSocket;
	}

#endif

	(*admin)->bundle_context= context;
	(*admin)->localPublications = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	(*admin)->subscriptions = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	(*admin)->pendingSubscriptions = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	(*admin)->externalPublications = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	(*admin)->topicSubscriptionsPerSerializer = hashMap_create(NULL, NULL, NULL, NULL);
	(*admin)->topicPublicationsPerSerializer  = hashMap_create(NULL, NULL, NULL, NULL);
	arrayList_create(&((*admin)->noSerializerSubscriptions));
	arrayList_create(&((*admin)->noSerializerPublications));
	arrayList_create(&((*admin)->serializerList));

	celixThreadMutex_create(&(*admin)->localPublicationsLock, NULL);
	celixThreadMutex_create(&(*admin)->subscriptionsLock, NULL);
	celixThreadMutex_create(&(*admin)->externalPublicationsLock, NULL);
	celixThreadMutex_create(&(*admin)->serializerListLock, NULL);
	celixThreadMutex_create(&(*admin)->usedSerializersLock, NULL);

	celixThreadMutexAttr_create(&(*admin)->noSerializerPendingsAttr);
	celixThreadMutexAttr_settype(&(*admin)->noSerializerPendingsAttr, CELIX_THREAD_MUTEX_RECURSIVE);
	celixThreadMutex_create(&(*admin)->noSerializerPendingsLock, &(*admin)->noSerializerPendingsAttr);

	celixThreadMutexAttr_create(&(*admin)->pendingSubscriptionsAttr);
	celixThreadMutexAttr_settype(&(*admin)->pendingSubscriptionsAttr, CELIX_THREAD_MUTEX_RECURSIVE);
	celixThreadMutex_create(&(*admin)->pendingSubscriptionsLock, &(*admin)->pendingSubscriptionsAttr);

	if (if_ip != NULL) {
		logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_INFO, "PSA_UDP_MC: Using %s as interface for multicast communication", if_ip);
		(*admin)->ifIpAddress = if_ip;
	} else {
		(*admin)->ifIpAddress = strdup("127.0.0.1");
	}

	if (mc_ip != NULL) {
		logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_INFO, "PSA_UDP_MC: Using %s for service annunciation", mc_ip);
		(*admin)->mcIpAddress = mc_ip;
	}
	else {
		logHelper_log((*admin)->loghelper, OSGI_LOGSERVICE_WARNING, "PSA_UDP_MC: No IP address for service annunciation set. Using %s", DEFAULT_MC_IP);
		(*admin)->mcIpAddress = strdup(DEFAULT_MC_IP);
	}

	(*admin)->defaultScore = PSA_UDPMC_DEFAULT_SCORE;
	(*admin)->qosSampleScore = PSA_UDPMC_DEFAULT_QOS_SAMPLE_SCORE;
	(*admin)->qosControlScore = PSA_UDPMC_DEFAULT_QOS_CONTROL_SCORE;

	const char *defaultScoreStr = NULL;
	const char *sampleScoreStr = NULL;
	const char *controlScoreStr = NULL;
	bundleContext_getProperty(context, PSA_UDPMC_DEFAULT_SCORE_KEY, &defaultScoreStr);
	bundleContext_getProperty(context, PSA_UDPMC_QOS_SAMPLE_SCORE_KEY, &sampleScoreStr);
	bundleContext_getProperty(context, PSA_UDPMC_QOS_CONTROL_SCORE_KEY, &controlScoreStr);

	if (defaultScoreStr != NULL) {
		(*admin)->defaultScore = strtof(defaultScoreStr, NULL);
	}
	if (sampleScoreStr != NULL) {
		(*admin)->qosSampleScore = strtof(sampleScoreStr, NULL);
	}
	if (controlScoreStr != NULL) {
		(*admin)->qosControlScore = strtof(controlScoreStr, NULL);
	}

	(*admin)->verbose = PSA_UDPMC_DEFAULT_VERBOSE;
	const char *verboseStr = NULL;
	bundleContext_getProperty(context, PSA_UDPMC_VERBOSE_KEY, &verboseStr);
	if (verboseStr != NULL) {
		(*admin)->verbose = strncasecmp("true", verboseStr, strlen("true")) == 0;
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

	celixThreadMutex_lock(&admin->serializerListLock);
	arrayList_destroy(admin->serializerList);
	celixThreadMutex_unlock(&admin->serializerListLock);

	celixThreadMutex_lock(&admin->noSerializerPendingsLock);
	arrayList_destroy(admin->noSerializerSubscriptions);
	arrayList_destroy(admin->noSerializerPublications);
	celixThreadMutex_unlock(&admin->noSerializerPendingsLock);

	celixThreadMutex_lock(&admin->usedSerializersLock);

	iter = hashMapIterator_create(admin->topicSubscriptionsPerSerializer);
	while(hashMapIterator_hasNext(iter)){
		arrayList_destroy((array_list_pt)hashMapIterator_nextValue(iter));
	}
	hashMapIterator_destroy(iter);
	hashMap_destroy(admin->topicSubscriptionsPerSerializer,false,false);

	iter = hashMapIterator_create(admin->topicPublicationsPerSerializer);
	while(hashMapIterator_hasNext(iter)){
		arrayList_destroy((array_list_pt)hashMapIterator_nextValue(iter));
	}
	hashMapIterator_destroy(iter);
	hashMap_destroy(admin->topicPublicationsPerSerializer,false,false);

	celixThreadMutex_unlock(&admin->usedSerializersLock);

	celixThreadMutex_destroy(&admin->usedSerializersLock);
	celixThreadMutex_destroy(&admin->serializerListLock);

	celixThreadMutexAttr_destroy(&admin->noSerializerPendingsAttr);
	celixThreadMutex_destroy(&admin->noSerializerPendingsLock);

	celixThreadMutex_destroy(&admin->pendingSubscriptionsLock);
	celixThreadMutexAttr_destroy(&admin->pendingSubscriptionsAttr);

	celixThreadMutex_destroy(&admin->subscriptionsLock);
	celixThreadMutex_destroy(&admin->localPublicationsLock);
	celixThreadMutex_destroy(&admin->externalPublicationsLock);

	logHelper_stop(admin->loghelper);

	logHelper_destroy(&admin->loghelper);

	free(admin);

	return status;
}

static celix_status_t pubsubAdmin_addAnySubscription(pubsub_admin_pt admin,pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&admin->subscriptionsLock);

	topic_subscription_pt any_sub = hashMap_get(admin->subscriptions,PUBSUB_ANY_SUB_TOPIC);

	if(any_sub==NULL){

		int i;
		pubsub_serializer_service_t *best_serializer = NULL;
		if( (status=pubsubAdmin_getBestSerializer(admin, subEP, &best_serializer, NULL)) == CELIX_SUCCESS){
			status = pubsub_topicSubscriptionCreate(admin->bundle_context, admin->ifIpAddress, PUBSUB_SUBSCRIBER_SCOPE_DEFAULT, PUBSUB_ANY_SUB_TOPIC, best_serializer, &any_sub);
		}
		else{
			if (admin->verbose) {
				printf("PSA_UDP_MC: Cannot find a serializer for subscribing topic %s. Adding it to pending list.\n",
					   properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
			}

			celixThreadMutex_lock(&admin->noSerializerPendingsLock);
			arrayList_add(admin->noSerializerSubscriptions,subEP);
			celixThreadMutex_unlock(&admin->noSerializerPendingsLock);
		}

		if (status == CELIX_SUCCESS){

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
						if(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL) !=NULL){
							status += pubsub_topicSubscriptionConnectPublisher(any_sub, (char*) properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL));
						}
					}
					arrayList_destroy(topic_publishers);
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
						if(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL) !=NULL){
							status += pubsub_topicSubscriptionConnectPublisher(any_sub, (char*) properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL));
						}
					}
				}
			}
			hashMapIterator_destroy(extp_iter);
			celixThreadMutex_unlock(&admin->externalPublicationsLock);


			pubsub_topicSubscriptionAddSubscriber(any_sub,subEP);

			status += pubsub_topicSubscriptionStart(any_sub);

		}

		if (status == CELIX_SUCCESS){
			hashMap_put(admin->subscriptions,strdup(PUBSUB_ANY_SUB_TOPIC),any_sub);
			connectTopicPubSubToSerializer(admin, best_serializer, any_sub, false);
		}

	}

	celixThreadMutex_unlock(&admin->subscriptionsLock);

	return status;
}

celix_status_t pubsubAdmin_addSubscription(pubsub_admin_pt admin,pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

	if(strcmp(properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME),PUBSUB_ANY_SUB_TOPIC)==0){
		return pubsubAdmin_addAnySubscription(admin,subEP);
	}

	/* Check if we already know some publisher about this topic, otherwise let's put the subscription in the pending hashmap */
	celixThreadMutex_lock(&admin->pendingSubscriptionsLock);
	celixThreadMutex_lock(&admin->subscriptionsLock);
	celixThreadMutex_lock(&admin->localPublicationsLock);
	celixThreadMutex_lock(&admin->externalPublicationsLock);

	char* scope_topic = pubsubEndpoint_createScopeTopicKey(properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));

	service_factory_pt factory = (service_factory_pt)hashMap_get(admin->localPublications,scope_topic);
	array_list_pt ext_pub_list = (array_list_pt)hashMap_get(admin->externalPublications,scope_topic);

	if (factory==NULL && ext_pub_list==NULL) { //No (local or external) publishers yet for this topic
		pubsubAdmin_addSubscriptionToPendingList(admin,subEP);
	} else {
		int i;
		topic_subscription_pt subscription = hashMap_get(admin->subscriptions, scope_topic);

		if (subscription == NULL) {
			pubsub_serializer_service_t *best_serializer = NULL;
            const char *serType = NULL;
			if( (status=pubsubAdmin_getBestSerializer(admin, subEP, &best_serializer, &serType)) == CELIX_SUCCESS){
				status += pubsub_topicSubscriptionCreate(admin->bundle_context,admin->ifIpAddress, (char*) properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE), (char*) properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME), best_serializer, &subscription);
			} else {
				if (admin->verbose) {
					printf("PSA_UDP_MC: Cannot find a serializer for subscribing topic %s. Adding it to pending list.\n",
						   properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
				}

				celixThreadMutex_lock(&admin->noSerializerPendingsLock);
				arrayList_add(admin->noSerializerSubscriptions,subEP);
				celixThreadMutex_unlock(&admin->noSerializerPendingsLock);
			}

			if (status==CELIX_SUCCESS){

				/* Try to connect internal publishers */
				if(factory!=NULL){
					topic_publication_pt topic_pubs = (topic_publication_pt)factory->handle;
					array_list_pt topic_publishers = pubsub_topicPublicationGetPublisherList(topic_pubs);

					if(topic_publishers!=NULL){
						for(i=0;i<arrayList_size(topic_publishers);i++){
							pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(topic_publishers,i);
							if(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL) !=NULL){
								status += pubsub_topicSubscriptionConnectPublisher(subscription, (char*) properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL));
							}
						}
						arrayList_destroy(topic_publishers);
					}

				}

				/* Look also for external publishers */
				if(ext_pub_list!=NULL){
					for(i=0;i<arrayList_size(ext_pub_list);i++){
						pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(ext_pub_list,i);
						if(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL) !=NULL){
							status += pubsub_topicSubscriptionConnectPublisher(subscription, (char*) properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL));
						}
					}
				}

				pubsub_topicSubscriptionAddSubscriber(subscription,subEP);

				status += pubsub_topicSubscriptionStart(subscription);

			}

			if(status==CELIX_SUCCESS){

				hashMap_put(admin->subscriptions,strdup(scope_topic),subscription);

				connectTopicPubSubToSerializer(admin, best_serializer, subscription, false);
			}
		}

		if (status == CELIX_SUCCESS){
			pubsub_topicIncreaseNrSubscribers(subscription);
		}
	}

	free(scope_topic);
	celixThreadMutex_unlock(&admin->externalPublicationsLock);
	celixThreadMutex_unlock(&admin->localPublicationsLock);
	celixThreadMutex_unlock(&admin->subscriptionsLock);
	celixThreadMutex_unlock(&admin->pendingSubscriptionsLock);

    if (admin->verbose) {
        printf("PSA_UDPMC: Added subscription [FWUUID=%s endpointUUID=%s scope=%s, topic=%s]\n",
               properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID),
               properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_UUID),
               properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),
               properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
        printf("PSA_UDPMC: \t [psa type = %s, ser type = %s, pubsub endpoint type = %s]\n",
               properties_get(subEP->endpoint_props, PUBSUB_ADMIN_TYPE_KEY),
               properties_get(subEP->endpoint_props, PUBSUB_SERIALIZER_TYPE_KEY),
               properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TYPE));
        printf("PSA_UDPMC: \t [endpoint url = %s]\n", properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_URL));
    }

	return status;

}

celix_status_t pubsubAdmin_removeSubscription(pubsub_admin_pt admin,pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

    if (admin->verbose) {
        printf("PSA_UDPMC: Removing subscription [FWUUID=%s endpointUUID=%s scope=%s, topic=%s]\n",
               properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID),
               properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_UUID),
               properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),
               properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
        printf("PSA_UDPMC: \t [psa type = %s, ser type = %s, pubsub endpoint type = %s]\n",
               properties_get(subEP->endpoint_props, PUBSUB_ADMIN_TYPE_KEY),
               properties_get(subEP->endpoint_props, PUBSUB_SERIALIZER_TYPE_KEY),
               properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TYPE));
        printf("PSA_UDPMC: \t [endpoint url = %s]\n", properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_URL));
    }

	char* scope_topic = pubsubEndpoint_createScopeTopicKey(properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE), properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));

	celixThreadMutex_lock(&admin->subscriptionsLock);
	topic_subscription_pt sub = (topic_subscription_pt)hashMap_get(admin->subscriptions,scope_topic);
	if(sub!=NULL){
		pubsub_topicDecreaseNrSubscribers(sub);
		if(pubsub_topicGetNrSubscribers(sub) == 0) {
			status = pubsub_topicSubscriptionRemoveSubscriber(sub,subEP);
		}
	}
	celixThreadMutex_unlock(&admin->subscriptionsLock);

	if(sub==NULL){
		/* Maybe the endpoint was pending */
		celixThreadMutex_lock(&admin->noSerializerPendingsLock);
		if(!arrayList_removeElement(admin->noSerializerSubscriptions, subEP)){
			status = CELIX_ILLEGAL_STATE;
		}
		celixThreadMutex_unlock(&admin->noSerializerPendingsLock);
	}

	free(scope_topic);



	return status;

}

celix_status_t pubsubAdmin_addPublication(pubsub_admin_pt admin,pubsub_endpoint_pt pubEP){
	celix_status_t status = CELIX_SUCCESS;

    const char* fwUUID = NULL;
    bundleContext_getProperty(admin->bundle_context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);
    if(fwUUID==NULL){
        printf("PSA_UDP_MC: Cannot retrieve fwUUID.\n");
        return CELIX_INVALID_BUNDLE_CONTEXT;
    }

    const char *epFwUUID = properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID);
    bool isOwn = strncmp(fwUUID, epFwUUID, 128) == 0;

    if (isOwn) {
        //should be null, willl be set in this call
        assert(properties_get(pubEP->endpoint_props, PUBSUB_ADMIN_TYPE_KEY) == NULL);
        assert(properties_get(pubEP->endpoint_props, PUBSUB_SERIALIZER_TYPE_KEY) == NULL);
    }

    if (isOwn) {
        properties_set(pubEP->endpoint_props, PUBSUB_ADMIN_TYPE_KEY, PSA_UDPMC_PUBSUB_ADMIN_TYPE);
    }

	char* scope_topic = pubsubEndpoint_createScopeTopicKey(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE), properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));

	if ((strcmp(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID), fwUUID) == 0) && (properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL) == NULL)) {

		celixThreadMutex_lock(&admin->localPublicationsLock);

		service_factory_pt factory = (service_factory_pt) hashMap_get(admin->localPublications, scope_topic);

		if (factory == NULL) {
			topic_publication_pt pub = NULL;
			pubsub_serializer_service_t *best_serializer = NULL;
			const char* serType = NULL;
			if( (status=pubsubAdmin_getBestSerializer(admin, pubEP, &best_serializer, &serType)) == CELIX_SUCCESS){
				status = pubsub_topicPublicationCreate(admin->sendSocket, pubEP, best_serializer, serType, admin->mcIpAddress, &pub);
                if (isOwn) {
                    properties_set(pubEP->endpoint_props, PUBSUB_SERIALIZER_TYPE_KEY, serType);
                }
			} else {
				printf("PSA_UDP_MC: Cannot find a serializer for publishing topic %s. Adding it to pending list.\n",
					   properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));

				celixThreadMutex_lock(&admin->noSerializerPendingsLock);
				arrayList_add(admin->noSerializerPublications,pubEP);
				celixThreadMutex_unlock(&admin->noSerializerPendingsLock);
			}

			if (status == CELIX_SUCCESS) {
				status = pubsub_topicPublicationStart(admin->bundle_context, pub, &factory);
				if (status == CELIX_SUCCESS && factory != NULL) {
					hashMap_put(admin->localPublications, strdup(scope_topic), factory);
					connectTopicPubSubToSerializer(admin, best_serializer, pub, true);
				}
			} else {
				printf("PSA_UDP_MC: Cannot create a topicPublication for scope=%s, topic=%s (bundle %s).\n",
					   properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),
					   properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME),
					   properties_get(pubEP->endpoint_props, PUBSUB_BUNDLE_ID));
			}
		} else {
			//just add the new EP to the list
			topic_publication_pt pub = (topic_publication_pt) factory->handle;
			pubsub_topicPublicationAddPublisherEP(pub, pubEP);
		}

		celixThreadMutex_unlock(&admin->localPublicationsLock);
	} else {

		celixThreadMutex_lock(&admin->externalPublicationsLock);
		array_list_pt ext_pub_list = (array_list_pt) hashMap_get(admin->externalPublications, scope_topic);
		if (ext_pub_list == NULL) {
			arrayList_create(&ext_pub_list);
			hashMap_put(admin->externalPublications, strdup(scope_topic), ext_pub_list);
		}

		arrayList_add(ext_pub_list, pubEP);

		celixThreadMutex_unlock(&admin->externalPublicationsLock);
	}

	/* Re-evaluate the pending subscriptions */
	celixThreadMutex_lock(&admin->pendingSubscriptionsLock);

	hash_map_entry_pt pendingSub = hashMap_getEntry(admin->pendingSubscriptions, scope_topic);
	if (pendingSub != NULL) { //There were pending subscription for the just published topic. Let's connect them.
		char* topic = (char*) hashMapEntry_getKey(pendingSub);
		array_list_pt pendingSubList = (array_list_pt) hashMapEntry_getValue(pendingSub);
		int i;
		for (i = 0; i < arrayList_size(pendingSubList); i++) {
			pubsub_endpoint_pt subEP = (pubsub_endpoint_pt) arrayList_get(pendingSubList, i);
			pubsubAdmin_addSubscription(admin, subEP);
		}
		hashMap_remove(admin->pendingSubscriptions, scope_topic);
		arrayList_clear(pendingSubList);
		arrayList_destroy(pendingSubList);
		free(topic);
	}

	celixThreadMutex_unlock(&admin->pendingSubscriptionsLock);

	/* Connect the new publisher to the subscription for his topic, if there is any */
	celixThreadMutex_lock(&admin->subscriptionsLock);

	topic_subscription_pt sub = (topic_subscription_pt) hashMap_get(admin->subscriptions, scope_topic);
	if (sub != NULL && properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL) != NULL) {
		pubsub_topicSubscriptionAddConnectPublisherToPendingList(sub, (char*) properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL));
	}

	/* And check also for ANY subscription */
	topic_subscription_pt any_sub = (topic_subscription_pt) hashMap_get(admin->subscriptions, PUBSUB_ANY_SUB_TOPIC);
	if (any_sub != NULL && properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL) != NULL) {
		pubsub_topicSubscriptionAddConnectPublisherToPendingList(any_sub, (char*) properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL));
	}

	free(scope_topic);

	celixThreadMutex_unlock(&admin->subscriptionsLock);

    if (admin->verbose) {
        printf("PSA_UDPMC: Added publication [FWUUID=%s endpointUUID=%s scope=%s, topic=%s]\n",
               properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID),
               properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_UUID),
               properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),
               properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
        printf("PSA_UDPMC: \t [psa type = %s, ser type = %s, pubsub endpoint type = %s]\n",
               properties_get(pubEP->endpoint_props, PUBSUB_ADMIN_TYPE_KEY),
               properties_get(pubEP->endpoint_props, PUBSUB_SERIALIZER_TYPE_KEY),
               properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TYPE));
        printf("PSA_UDPMC: \t [endpoint url = %s, own = %i]\n",
               properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL),
               isOwn);
    }

	return status;

}

celix_status_t pubsubAdmin_removePublication(pubsub_admin_pt admin,pubsub_endpoint_pt pubEP){
	celix_status_t status = CELIX_SUCCESS;
	int count = 0;

    if (admin->verbose) {
        printf("PSA_UDPMC: Adding publication [FWUUID=%s endpointUUID=%s scope=%s, topic=%s]\n",
               properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID),
               properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_UUID),
               properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),
               properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
        printf("PSA_UDPMC: \t [psa type = %s, ser type = %s, pubsub endpoint type = %s]\n",
               properties_get(pubEP->endpoint_props, PUBSUB_ADMIN_TYPE_KEY),
               properties_get(pubEP->endpoint_props, PUBSUB_SERIALIZER_TYPE_KEY),
               properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TYPE));
        printf("PSA_UDPMC: \t [endpoint url = %s]\n", properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL));
    }

	const char* fwUUID = NULL;

	bundleContext_getProperty(admin->bundle_context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);
	if(fwUUID==NULL){
		printf("PSA_UDP_MC: Cannot retrieve fwUUID.\n");
		return CELIX_INVALID_BUNDLE_CONTEXT;
	}
	char *scope_topic = pubsubEndpoint_createScopeTopicKey(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE), properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));

	if(strcmp(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID),fwUUID)==0){

		celixThreadMutex_lock(&admin->localPublicationsLock);
		service_factory_pt factory = (service_factory_pt)hashMap_get(admin->localPublications,scope_topic);
		if(factory!=NULL){
			topic_publication_pt pub = (topic_publication_pt)factory->handle;
			pubsub_topicPublicationRemovePublisherEP(pub,pubEP);
		}
		celixThreadMutex_unlock(&admin->localPublicationsLock);

		if(factory==NULL){
			/* Maybe the endpoint was pending */
			celixThreadMutex_lock(&admin->noSerializerPendingsLock);
			if(!arrayList_removeElement(admin->noSerializerPublications, pubEP)){
				status = CELIX_ILLEGAL_STATE;
			}
			celixThreadMutex_unlock(&admin->noSerializerPendingsLock);
		}

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
				if (found){
					arrayList_remove(ext_pub_list,i);
				}
			}
			// Check if there are more publishers on the same endpoint (happens when 1 celix-instance with multiple bundles publish in same topic)
			for(i=0; i<arrayList_size(ext_pub_list);i++) {
				pubsub_endpoint_pt p  = (pubsub_endpoint_pt)arrayList_get(ext_pub_list,i);
				if (strcmp(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL),properties_get(p->endpoint_props, PUBSUB_ENDPOINT_URL)) == 0) {
					count++;
				}
			}

			if(arrayList_size(ext_pub_list)==0){
				hash_map_entry_pt entry = hashMap_getEntry(admin->externalPublications,scope_topic);
				char* topic = (char*)hashMapEntry_getKey(entry);
				array_list_pt list = (array_list_pt)hashMapEntry_getValue(entry);
				hashMap_remove(admin->externalPublications,topic);
				arrayList_destroy(list);
				free(topic);
			}
		}

		celixThreadMutex_unlock(&admin->externalPublicationsLock);
	}

	/* Check if this publisher was connected to one of our subscribers*/
	celixThreadMutex_lock(&admin->subscriptionsLock);

	topic_subscription_pt sub = (topic_subscription_pt)hashMap_get(admin->subscriptions,scope_topic);
	if(sub!=NULL && properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL)!=NULL && count == 0){
		pubsub_topicSubscriptionAddDisconnectPublisherToPendingList(sub, (char*) properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL));
	}

	/* And check also for ANY subscription */
	topic_subscription_pt any_sub = (topic_subscription_pt)hashMap_get(admin->subscriptions,PUBSUB_ANY_SUB_TOPIC);
	if(any_sub!=NULL && properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL)!=NULL && count == 0){
		pubsub_topicSubscriptionAddDisconnectPublisherToPendingList(any_sub, (char*) properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL));
	}

	free(scope_topic);
	celixThreadMutex_unlock(&admin->subscriptionsLock);

	return status;

}

celix_status_t pubsubAdmin_closeAllPublications(pubsub_admin_pt admin,char *scope, char* topic){
	celix_status_t status = CELIX_SUCCESS;

	printf("PSA_UDP_MC: Closing all publications for scope=%s,topic=%s\n", scope, topic);

	celixThreadMutex_lock(&admin->localPublicationsLock);
	char* scope_topic = pubsubEndpoint_createScopeTopicKey(scope, topic);
	hash_map_entry_pt pubsvc_entry = (hash_map_entry_pt)hashMap_getEntry(admin->localPublications,scope_topic);
	if(pubsvc_entry!=NULL){
		char* key = (char*)hashMapEntry_getKey(pubsvc_entry);
		service_factory_pt factory= (service_factory_pt)hashMapEntry_getValue(pubsvc_entry);
		topic_publication_pt pub = (topic_publication_pt)factory->handle;
		status += pubsub_topicPublicationStop(pub);
		disconnectTopicPubSubFromSerializer(admin, pub, true);
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

	printf("PSA_UDP_MC: Closing all subscriptions\n");

	celixThreadMutex_lock(&admin->subscriptionsLock);
	char* scope_topic = pubsubEndpoint_createScopeTopicKey(scope, topic);
	hash_map_entry_pt sub_entry = (hash_map_entry_pt)hashMap_getEntry(admin->subscriptions,scope_topic);
	if(sub_entry!=NULL){
		char* topic = (char*)hashMapEntry_getKey(sub_entry);

		topic_subscription_pt ts = (topic_subscription_pt)hashMapEntry_getValue(sub_entry);
		status += pubsub_topicSubscriptionStop(ts);
		disconnectTopicPubSubFromSerializer(admin, ts, false);
		status += pubsub_topicSubscriptionDestroy(ts);
		hashMap_remove(admin->subscriptions,topic);
		free(topic);

	}
	free(scope_topic);
	celixThreadMutex_unlock(&admin->subscriptionsLock);

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

	return status;
}
#endif

static celix_status_t pubsubAdmin_addSubscriptionToPendingList(pubsub_admin_pt admin,pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

	char* scope_topic = pubsubEndpoint_createScopeTopicKey(properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE), properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
	array_list_pt pendingListPerTopic = hashMap_get(admin->pendingSubscriptions,scope_topic);
	if(pendingListPerTopic==NULL){
		arrayList_create(&pendingListPerTopic);
		hashMap_put(admin->pendingSubscriptions,strdup(scope_topic),pendingListPerTopic);
	}
	arrayList_add(pendingListPerTopic,subEP);
	free(scope_topic);

	return status;
}


celix_status_t pubsubAdmin_serializerAdded(void * handle, service_reference_pt reference, void * service){
	/* Assumption: serializers are all available at startup.
	 * If a new (possibly better) serializer is installed and started, already created topic_publications/subscriptions will not be destroyed and recreated */

	celix_status_t status = CELIX_SUCCESS;
	int i=0;

	const char *serType = NULL;
	serviceReference_getProperty(reference, PUBSUB_SERIALIZER_TYPE_KEY,&serType);
	if(serType == NULL){
		printf("PSA_UDPMC: Serializer serviceReference %p has no %s property specified\n",reference, PUBSUB_SERIALIZER_TYPE_KEY);
		return CELIX_SERVICE_EXCEPTION;
	}

	pubsub_admin_pt admin = (pubsub_admin_pt)handle;
	celixThreadMutex_lock(&admin->serializerListLock);
	arrayList_add(admin->serializerList, reference);
	celixThreadMutex_unlock(&admin->serializerListLock);

	/* Now let's re-evaluate the pending */
	celixThreadMutex_lock(&admin->noSerializerPendingsLock);

	for(i=0;i<arrayList_size(admin->noSerializerSubscriptions);i++){
		pubsub_endpoint_pt ep = (pubsub_endpoint_pt)arrayList_get(admin->noSerializerSubscriptions,i);
		pubsub_serializer_service_t *best_serializer = NULL;
		pubsubAdmin_getBestSerializer(admin, ep, &best_serializer, NULL);
		if(best_serializer != NULL){ /* Finally we have a valid serializer! */
			pubsubAdmin_addSubscription(admin, ep);
		}
	}

	for(i=0;i<arrayList_size(admin->noSerializerPublications);i++){
		pubsub_endpoint_pt ep = (pubsub_endpoint_pt)arrayList_get(admin->noSerializerPublications,i);
		pubsub_serializer_service_t *best_serializer = NULL;
		pubsubAdmin_getBestSerializer(admin, ep, &best_serializer, NULL);
		if(best_serializer != NULL){ /* Finally we have a valid serializer! */
			pubsubAdmin_addPublication(admin, ep);
		}
	}

	celixThreadMutex_unlock(&admin->noSerializerPendingsLock);

	if (admin->verbose) {
		printf("PSA_UDP_MC: %s serializer added\n", serType);
	}

	return status;
}

celix_status_t pubsubAdmin_serializerRemoved(void * handle, service_reference_pt reference, void * service){

	pubsub_admin_pt admin = (pubsub_admin_pt)handle;
	int i=0, j=0;
	const char *serType = NULL;

	serviceReference_getProperty(reference, PUBSUB_SERIALIZER_TYPE_KEY,&serType);
	if(serType == NULL){
		printf("Serializer serviceReference %p has no %s property specified\n",reference, PUBSUB_SERIALIZER_TYPE_KEY);
		return CELIX_SERVICE_EXCEPTION;
	}

	celixThreadMutex_lock(&admin->serializerListLock);
	/* Remove the serializer from the list */
	arrayList_removeElement(admin->serializerList, reference);
	celixThreadMutex_unlock(&admin->serializerListLock);

	celixThreadMutex_lock(&admin->usedSerializersLock);
	array_list_pt topicPubList = (array_list_pt)hashMap_remove(admin->topicPublicationsPerSerializer, service);
	array_list_pt topicSubList = (array_list_pt)hashMap_remove(admin->topicSubscriptionsPerSerializer, service);
	celixThreadMutex_unlock(&admin->usedSerializersLock);

	/* Now destroy the topicPublications, but first put back the pubsub_endpoints back to the noSerializer pending list */
	if(topicPubList!=NULL){
		for(i=0;i<arrayList_size(topicPubList);i++){
			topic_publication_pt topicPub = (topic_publication_pt)arrayList_get(topicPubList,i);
			/* Stop the topic publication */
			pubsub_topicPublicationStop(topicPub);
			/* Get the endpoints that are going to be orphan */
			array_list_pt pubList = pubsub_topicPublicationGetPublisherList(topicPub);
			for(j=0;j<arrayList_size(pubList);j++){
				pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(pubList,j);
				/* Remove the publication */
				pubsubAdmin_removePublication(admin, pubEP);
				/* Reset the endpoint field, so that will be recreated from scratch when a new serializer will be found */
				if(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL)!=NULL){
					properties_unset(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL);
				}
				/* Add the orphan endpoint to the noSerializer pending list */
				celixThreadMutex_lock(&admin->noSerializerPendingsLock);
				arrayList_add(admin->noSerializerPublications,pubEP);
				celixThreadMutex_unlock(&admin->noSerializerPendingsLock);
			}
			arrayList_destroy(pubList);

			/* Cleanup also the localPublications hashmap*/
			celixThreadMutex_lock(&admin->localPublicationsLock);
			hash_map_iterator_pt iter = hashMapIterator_create(admin->localPublications);
			char *key = NULL;
			service_factory_pt factory = NULL;
			while(hashMapIterator_hasNext(iter)){
				hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
				factory = (service_factory_pt)hashMapEntry_getValue(entry);
				topic_publication_pt pub = (topic_publication_pt)factory->handle;
				if(pub==topicPub){
					key = (char*)hashMapEntry_getKey(entry);
					break;
				}
			}
			hashMapIterator_destroy(iter);
			if(key!=NULL){
				hashMap_remove(admin->localPublications, key);
				free(factory);
				free(key);
			}
			celixThreadMutex_unlock(&admin->localPublicationsLock);

			/* Finally destroy the topicPublication */
			pubsub_topicPublicationDestroy(topicPub);
		}
		arrayList_destroy(topicPubList);
	}

	/* Now destroy the topicSubscriptions, but first put back the pubsub_endpoints back to the noSerializer pending list */
	if(topicSubList!=NULL){
		for(i=0;i<arrayList_size(topicSubList);i++){
			topic_subscription_pt topicSub = (topic_subscription_pt)arrayList_get(topicSubList,i);
			/* Stop the topic subscription */
			pubsub_topicSubscriptionStop(topicSub);
			/* Get the endpoints that are going to be orphan */
			array_list_pt subList = pubsub_topicSubscriptionGetSubscribersList(topicSub);
			for(j=0;j<arrayList_size(subList);j++){
				pubsub_endpoint_pt subEP = (pubsub_endpoint_pt)arrayList_get(subList,j);
				/* Remove the subscription */
				pubsubAdmin_removeSubscription(admin, subEP);
				/* Reset the endpoint field, so that will be recreated from scratch when a new serializer will be found */
				if(properties_get(subEP->endpoint_props, PUBSUB_ENDPOINT_URL)!=NULL){
					properties_unset(subEP->endpoint_props, PUBSUB_ENDPOINT_URL);
				}
				/* Add the orphan endpoint to the noSerializer pending list */
				celixThreadMutex_lock(&admin->noSerializerPendingsLock);
				arrayList_add(admin->noSerializerSubscriptions,subEP);
				celixThreadMutex_unlock(&admin->noSerializerPendingsLock);
			}

			/* Cleanup also the subscriptions hashmap*/
			celixThreadMutex_lock(&admin->subscriptionsLock);
			hash_map_iterator_pt iter = hashMapIterator_create(admin->subscriptions);
			char *key = NULL;
			while(hashMapIterator_hasNext(iter)){
				hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
				topic_subscription_pt sub = (topic_subscription_pt)hashMapEntry_getValue(entry);
				if(sub==topicSub){
					key = (char*)hashMapEntry_getKey(entry);
					break;
				}
			}
			hashMapIterator_destroy(iter);
			if(key!=NULL){
				hashMap_remove(admin->subscriptions, key);
				free(key);
			}
			celixThreadMutex_unlock(&admin->subscriptionsLock);

			/* Finally destroy the topicSubscription */
			pubsub_topicSubscriptionDestroy(topicSub);
		}
		arrayList_destroy(topicSubList);
	}

	if (admin->verbose) {
		printf("PSA_UDP_MC: %s serializer removed\n", serType);
	}


	return CELIX_SUCCESS;
}

celix_status_t pubsubAdmin_matchEndpoint(pubsub_admin_pt admin, pubsub_endpoint_pt endpoint, double* score){
	celix_status_t status = CELIX_SUCCESS;

    const char *fwUuid = NULL;
    bundleContext_getProperty(admin->bundle_context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &fwUuid);
    if (fwUuid == NULL) {
        return CELIX_ILLEGAL_STATE;
    }

	celixThreadMutex_lock(&admin->serializerListLock);
	status = pubsub_admin_match(endpoint, PSA_UDPMC_PUBSUB_ADMIN_TYPE, fwUuid, admin->qosSampleScore, admin->qosControlScore, admin->defaultScore, admin->serializerList,score);
	celixThreadMutex_unlock(&admin->serializerListLock);

	return status;
}

/* This one recall the same logic as in the match function */
static celix_status_t pubsubAdmin_getBestSerializer(pubsub_admin_pt admin, pubsub_endpoint_pt ep, pubsub_serializer_service_t **out, const char **serType){
	celix_status_t status = CELIX_SUCCESS;

	pubsub_serializer_service_t *serSvc = NULL;
	service_reference_pt svcRef = NULL;

	celixThreadMutex_lock(&admin->serializerListLock);
	status = pubsub_admin_get_best_serializer(ep->topic_props, admin->serializerList, &svcRef);
	celixThreadMutex_unlock(&admin->serializerListLock);

	if (svcRef != NULL) {
		bundleContext_getService(admin->bundle_context, svcRef, (void**)&serSvc);
		bundleContext_ungetService(admin->bundle_context, svcRef, NULL); //TODO, FIXME this should not be done this way. only unget if the service is not used any more
        if (serType != NULL) {
            serviceReference_getProperty(svcRef, PUBSUB_SERIALIZER_TYPE_KEY, serType);
        }
	}

	*out = serSvc;

	return status;
}

static void connectTopicPubSubToSerializer(pubsub_admin_pt admin,pubsub_serializer_service_t *serializer,void *topicPubSub,bool isPublication){

	celixThreadMutex_lock(&admin->usedSerializersLock);

	hash_map_pt map = isPublication?admin->topicPublicationsPerSerializer:admin->topicSubscriptionsPerSerializer;
	array_list_pt list = (array_list_pt)hashMap_get(map,serializer);
	if(list==NULL){
		arrayList_create(&list);
		hashMap_put(map,serializer,list);
	}
	arrayList_add(list,topicPubSub);

	celixThreadMutex_unlock(&admin->usedSerializersLock);

}

static void disconnectTopicPubSubFromSerializer(pubsub_admin_pt admin,void *topicPubSub,bool isPublication){

	celixThreadMutex_lock(&admin->usedSerializersLock);

	hash_map_pt map = isPublication?admin->topicPublicationsPerSerializer:admin->topicSubscriptionsPerSerializer;
	hash_map_iterator_pt iter = hashMapIterator_create(map);
	while(hashMapIterator_hasNext(iter)){
		array_list_pt list = (array_list_pt)hashMapIterator_nextValue(iter);
		if(arrayList_removeElement(list, topicPubSub)){ //Found it!
			break;
		}
	}
	hashMapIterator_destroy(iter);

	celixThreadMutex_unlock(&admin->usedSerializersLock);

}
