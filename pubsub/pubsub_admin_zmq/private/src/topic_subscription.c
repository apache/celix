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
 * topic_subscription.c
 *
 *  \date       Oct 2, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include "topic_subscription.h"
#include <czmq.h>
/* The following undefs prevent the collision between:
 * - sys/syslog.h (which is included within czmq)
 * - celix/dfi/dfi_log_util.h
 */
#undef LOG_DEBUG
#undef LOG_WARNING
#undef LOG_INFO
#undef LOG_WARNING

#include <string.h>
#include <stdlib.h>
#include <signal.h>

#include "utils.h"
#include "celix_errno.h"
#include "constants.h"
#include "version.h"

#include "subscriber.h"
#include "publisher.h"
#include "pubsub_utils.h"

#ifdef BUILD_WITH_ZMQ_SECURITY
	#include "zmq_crypto.h"

	#define MAX_CERT_PATH_LENGTH 512
#endif

#define POLL_TIMEOUT  	250
#define ZMQ_POLL_TIMEOUT_MS_ENV 	"ZMQ_POLL_TIMEOUT_MS"

struct topic_subscription {
	zsock_t* zmq_socket;
	zcert_t * zmq_cert;
	zcert_t * zmq_pub_cert;
	pthread_mutex_t socket_lock; //Protects zmq_socket access
	service_tracker_pt tracker;
	array_list_pt sub_ep_list;
	celix_thread_t recv_thread;
	bool running;
	celix_thread_mutex_t ts_lock; //Protects topic_subscription data structure access
	bundle_context_pt context;

	hash_map_pt msgSerializerMapMap; // key = service ptr, value = pubsub_msg_serializer_map_t*
    hash_map_pt bundleMap; //key = service ptr, value = bundle_pt
	array_list_pt pendingConnections;
	array_list_pt pendingDisconnections;

	celix_thread_mutex_t pendingConnections_lock;
	celix_thread_mutex_t pendingDisconnections_lock;
	unsigned int nrSubscribers;
	pubsub_serializer_service_t* serializerSvc;
};

/* Note: correct locking order is
 * 1. socket_lock
 * 2. ts_lock
 */

typedef struct complete_zmq_msg {
	zframe_t* header;
	zframe_t* payload;
}* complete_zmq_msg_pt;

typedef struct mp_handle {
	pubsub_msg_serializer_map_t* map;
	hash_map_pt rcv_msg_map;
} mp_handle_t;

typedef struct msg_map_entry{
	bool retain;
	void* msgInst;
}* msg_map_entry_pt;

static celix_status_t topicsub_subscriberTracked(void * handle, service_reference_pt reference, void * service);
static celix_status_t topicsub_subscriberUntracked(void * handle, service_reference_pt reference, void * service);
static void* zmq_recv_thread_func(void* arg);
static bool checkVersion(version_pt msgVersion,pubsub_msg_header_pt hdr);
static void sigusr1_sighandler(int signo);
static int pubsub_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId);
static int pubsub_getMultipart(void *handle, unsigned int msgTypeId, bool retain, void **part);
static mp_handle_t* create_mp_handle(topic_subscription_pt sub, pubsub_msg_serializer_map_t* map, array_list_pt rcv_msg_list);
static void destroy_mp_handle(mp_handle_t* mp_handle);
static void connectPendingPublishers(topic_subscription_pt sub);
static void disconnectPendingPublishers(topic_subscription_pt sub);

celix_status_t pubsub_topicSubscriptionCreate(bundle_context_pt bundle_context, pubsub_endpoint_pt subEP, pubsub_serializer_service_t* serializer, char* scope, char* topic,topic_subscription_pt* out){
	celix_status_t status = CELIX_SUCCESS;

#ifdef BUILD_WITH_ZMQ_SECURITY
	if(strcmp(topic,PUBSUB_ANY_SUB_TOPIC) != 0){
		char* secure_topics = NULL;
		bundleContext_getProperty(bundle_context, "SECURE_TOPICS", (const char **) &secure_topics);

		if (secure_topics){
			array_list_pt secure_topics_list = pubsub_getTopicsFromString(secure_topics);

			int i;
			int secure_topics_size = arrayList_size(secure_topics_list);
			for (i = 0; i < secure_topics_size; i++){
				char* top = arrayList_get(secure_topics_list, i);
				if (strcmp(topic, top) == 0){
					printf("TS: Secure topic: '%s'\n", top);
					subEP->is_secure = true;
				}
				free(top);
				top = NULL;
			}

			arrayList_destroy(secure_topics_list);
		}
	}

	zcert_t* sub_cert = NULL;
	zcert_t* pub_cert = NULL;
	const char* pub_key = NULL;
	if (subEP->is_secure){
		char* keys_bundle_dir = pubsub_getKeysBundleDir(bundle_context);
		if (keys_bundle_dir == NULL){
			return CELIX_SERVICE_EXCEPTION;
		}

		const char* keys_file_path = NULL;
		const char* keys_file_name = NULL;
		bundleContext_getProperty(bundle_context, PROPERTY_KEYS_FILE_PATH, &keys_file_path);
		bundleContext_getProperty(bundle_context, PROPERTY_KEYS_FILE_NAME, &keys_file_name);

		char sub_cert_path[MAX_CERT_PATH_LENGTH];
		char pub_cert_path[MAX_CERT_PATH_LENGTH];

		//certificate path ".cache/bundle{id}/version0.0/./META-INF/keys/subscriber/private/sub_{topic}.key.enc"
		snprintf(sub_cert_path, MAX_CERT_PATH_LENGTH, "%s/META-INF/keys/subscriber/private/sub_%s.key.enc", keys_bundle_dir, topic);
		snprintf(pub_cert_path, MAX_CERT_PATH_LENGTH, "%s/META-INF/keys/publisher/public/pub_%s.pub", keys_bundle_dir, topic);
		free(keys_bundle_dir);

		printf("TS: Loading subscriber key '%s'\n", sub_cert_path);
		printf("TS: Loading publisher key '%s'\n", pub_cert_path);

		sub_cert = get_zcert_from_encoded_file((char *) keys_file_path, (char *) keys_file_name, sub_cert_path);
		if (sub_cert == NULL){
			printf("TS: Cannot load key '%s'\n", sub_cert_path);
			printf("TS: Topic '%s' NOT SECURED !\n", topic);
			subEP->is_secure = false;
		}

		pub_cert = zcert_load(pub_cert_path);
		if (sub_cert != NULL && pub_cert == NULL){
			zcert_destroy(&sub_cert);
			printf("TS: Cannot load key '%s'\n", pub_cert_path);
			printf("TS: Topic '%s' NOT SECURED !\n", topic);
			subEP->is_secure = false;
		}

		pub_key = zcert_public_txt(pub_cert);
	}
#endif

	zsock_t* zmq_s = zsock_new (ZMQ_SUB);
	if(zmq_s==NULL){
		#ifdef BUILD_WITH_ZMQ_SECURITY
		if (subEP->is_secure){
			zcert_destroy(&sub_cert);
			zcert_destroy(&pub_cert);
		}
		#endif

		return CELIX_SERVICE_EXCEPTION;
	}

	#ifdef BUILD_WITH_ZMQ_SECURITY
	if (subEP->is_secure){
		zcert_apply (sub_cert, zmq_s);
		zsock_set_curve_serverkey (zmq_s, pub_key); //apply key of publisher to socket of subscriber
	}
	#endif

	if(strcmp(topic,PUBSUB_ANY_SUB_TOPIC)==0){
		zsock_set_subscribe (zmq_s, "");
	}
	else{
		zsock_set_subscribe (zmq_s, topic);
	}

	topic_subscription_pt ts = (topic_subscription_pt) calloc(1,sizeof(*ts));
	ts->context = bundle_context;
	ts->zmq_socket = zmq_s;
	ts->running = false;
	ts->nrSubscribers = 0;
	ts->serializerSvc = NULL;

	#ifdef BUILD_WITH_ZMQ_SECURITY
	if (subEP->is_secure){
		ts->zmq_cert = sub_cert;
		ts->zmq_pub_cert = pub_cert;
	}
	#endif

	celixThreadMutex_create(&ts->socket_lock, NULL);
	celixThreadMutex_create(&ts->ts_lock,NULL);
	arrayList_create(&ts->sub_ep_list);
	ts->msgSerializerMapMap = hashMap_create(NULL, NULL, NULL, NULL);
    ts->bundleMap = hashMap_create(NULL, NULL, NULL, NULL);
	arrayList_create(&ts->pendingConnections);
	arrayList_create(&ts->pendingDisconnections);
	celixThreadMutex_create(&ts->pendingConnections_lock, NULL);
	celixThreadMutex_create(&ts->pendingDisconnections_lock, NULL);

	char filter[128];
	memset(filter,0,128);
	if(strncmp(PUBSUB_SUBSCRIBER_SCOPE_DEFAULT,scope,strlen(PUBSUB_SUBSCRIBER_SCOPE_DEFAULT)) == 0) {
	        // default scope, means that subscriber has not defined a scope property
	        snprintf(filter, 128, "(&(%s=%s)(%s=%s))",
	                (char*) OSGI_FRAMEWORK_OBJECTCLASS, PUBSUB_SUBSCRIBER_SERVICE_NAME,
	                PUBSUB_SUBSCRIBER_TOPIC,topic);

	} else {
        snprintf(filter, 128, "(&(%s=%s)(%s=%s)(%s=%s))",
                (char*) OSGI_FRAMEWORK_OBJECTCLASS, PUBSUB_SUBSCRIBER_SERVICE_NAME,
                PUBSUB_SUBSCRIBER_TOPIC,topic,
                PUBSUB_SUBSCRIBER_SCOPE,scope);
    }
	service_tracker_customizer_pt customizer = NULL;
	status += serviceTrackerCustomizer_create(ts,NULL,topicsub_subscriberTracked,NULL,topicsub_subscriberUntracked,&customizer);
	status += serviceTracker_createWithFilter(bundle_context, filter, customizer, &ts->tracker);

	struct sigaction actions;
	memset(&actions, 0, sizeof(actions));
	sigemptyset(&actions.sa_mask);
	actions.sa_flags = 0;
	actions.sa_handler = sigusr1_sighandler;

	sigaction(SIGUSR1,&actions,NULL);

	if (status == CELIX_SUCCESS) {
		*out=ts;
		pubsub_topicSubscriptionSetSerializer(ts, serializer);
	}

	return status;
}

celix_status_t pubsub_topicSubscriptionDestroy(topic_subscription_pt ts){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);

	ts->running = false;
	serviceTracker_destroy(ts->tracker);
	arrayList_clear(ts->sub_ep_list);
	arrayList_destroy(ts->sub_ep_list);
	hashMap_destroy(ts->msgSerializerMapMap,false,false);
    hashMap_destroy(ts->bundleMap,false,false);

	celixThreadMutex_lock(&ts->pendingConnections_lock);
	arrayList_destroy(ts->pendingConnections);
	celixThreadMutex_unlock(&ts->pendingConnections_lock);
	celixThreadMutex_destroy(&ts->pendingConnections_lock);

	celixThreadMutex_lock(&ts->pendingDisconnections_lock);
	arrayList_destroy(ts->pendingDisconnections);
	celixThreadMutex_unlock(&ts->pendingDisconnections_lock);
	celixThreadMutex_destroy(&ts->pendingDisconnections_lock);

	#ifdef BUILD_WITH_ZMQ_SECURITY
	zcert_destroy(&(ts->zmq_cert));
	zcert_destroy(&(ts->zmq_pub_cert));
	#endif

	celixThreadMutex_unlock(&ts->ts_lock);

	celixThreadMutex_lock(&ts->socket_lock);
	zsock_destroy(&(ts->zmq_socket));
	celixThreadMutex_unlock(&ts->socket_lock);
	celixThreadMutex_destroy(&ts->socket_lock);


	free(ts);

	return status;
}

celix_status_t pubsub_topicSubscriptionStart(topic_subscription_pt ts){
	celix_status_t status = CELIX_SUCCESS;

	//celixThreadMutex_lock(&ts->ts_lock);

	status = serviceTracker_open(ts->tracker);

	ts->running = true;

	if(status==CELIX_SUCCESS){
		status=celixThread_create(&ts->recv_thread,NULL,zmq_recv_thread_func,ts);
	}

	//celixThreadMutex_unlock(&ts->ts_lock);

	return status;
}

celix_status_t pubsub_topicSubscriptionStop(topic_subscription_pt ts){
	celix_status_t status = CELIX_SUCCESS;

	//celixThreadMutex_lock(&ts->ts_lock);

	ts->running = false;

	pthread_kill(ts->recv_thread.thread,SIGUSR1);

	celixThread_join(ts->recv_thread,NULL);

	status = serviceTracker_close(ts->tracker);

	//celixThreadMutex_unlock(&ts->ts_lock);

	return status;
}

celix_status_t pubsub_topicSubscriptionConnectPublisher(topic_subscription_pt ts, char* pubURL){
	celix_status_t status = CELIX_SUCCESS;
	celixThreadMutex_lock(&ts->socket_lock);
	if(!zsock_is(ts->zmq_socket) || zsock_connect(ts->zmq_socket, "%s", pubURL) != 0){
		status = CELIX_SERVICE_EXCEPTION;
	}
	celixThreadMutex_unlock(&ts->socket_lock);

	return status;
}

celix_status_t pubsub_topicSubscriptionAddConnectPublisherToPendingList(topic_subscription_pt ts, char* pubURL) {
    celix_status_t status = CELIX_SUCCESS;
    char *url = strdup(pubURL);
    celixThreadMutex_lock(&ts->pendingConnections_lock);
    arrayList_add(ts->pendingConnections, url);
    celixThreadMutex_unlock(&ts->pendingConnections_lock);
    return status;
}

celix_status_t pubsub_topicSubscriptionAddDisconnectPublisherToPendingList(topic_subscription_pt ts, char* pubURL) {
     celix_status_t status = CELIX_SUCCESS;
    char *url = strdup(pubURL);
    celixThreadMutex_lock(&ts->pendingDisconnections_lock);
    arrayList_add(ts->pendingDisconnections, url);
    celixThreadMutex_unlock(&ts->pendingDisconnections_lock);
    return status;
}

celix_status_t pubsub_topicSubscriptionDisconnectPublisher(topic_subscription_pt ts, char* pubURL){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->socket_lock);
	if(!zsock_is(ts->zmq_socket) || zsock_disconnect(ts->zmq_socket, "%s", pubURL) != 0){
		status = CELIX_SERVICE_EXCEPTION;
	}
	celixThreadMutex_unlock(&ts->socket_lock);

	return status;
}

celix_status_t pubsub_topicSubscriptionAddSubscriber(topic_subscription_pt ts, pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);

	arrayList_add(ts->sub_ep_list,subEP);

	celixThreadMutex_unlock(&ts->ts_lock);

	return status;

}

celix_status_t pubsub_topicIncreaseNrSubscribers(topic_subscription_pt ts) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);

	ts->nrSubscribers++;

	celixThreadMutex_unlock(&ts->ts_lock);

	return status;
}

celix_status_t pubsub_topicSubscriptionRemoveSubscriber(topic_subscription_pt ts, pubsub_endpoint_pt subEP){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);

	arrayList_removeElement(ts->sub_ep_list,subEP);

	celixThreadMutex_unlock(&ts->ts_lock);

	return status;

}

celix_status_t pubsub_topicDecreaseNrSubscribers(topic_subscription_pt ts) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);

	ts->nrSubscribers--;

	celixThreadMutex_unlock(&ts->ts_lock);

	return status;
}

unsigned int pubsub_topicGetNrSubscribers(topic_subscription_pt ts) {
	return ts->nrSubscribers;
}

celix_status_t pubsub_topicSubscriptionSetSerializer(topic_subscription_pt ts, pubsub_serializer_service_t* serializerSvc) {
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);
    //clear old
    if (ts->serializerSvc != NULL) {
        hash_map_iterator_t iter = hashMapIterator_construct(ts->msgSerializerMapMap);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(&iter);
            pubsub_subscriber_t* subsvc = hashMapEntry_getKey(entry);
            pubsub_msg_serializer_map_t* map = hashMapEntry_getValue(entry);
            ts->serializerSvc->destroySerializerMap(ts->serializerSvc->handle, map);
            hashMap_put(ts->msgSerializerMapMap, subsvc, NULL);

        }
    }
    ts->serializerSvc = serializerSvc;
    //init new
    if (ts->serializerSvc != NULL) {
        hash_map_iterator_t iter = hashMapIterator_construct(ts->msgSerializerMapMap);
        while (hashMapIterator_hasNext(&iter)) {
            pubsub_subscriber_t* subsvc = hashMapIterator_nextKey(&iter);
            bundle_pt bundle = hashMap_get(ts->bundleMap, subsvc);
            pubsub_msg_serializer_map_t* map = NULL;
            ts->serializerSvc->createSerializerMap(ts->serializerSvc->handle, bundle, &map);
            hashMap_put(ts->msgSerializerMapMap, subsvc, map);
        }
    }
	celixThreadMutex_unlock(&ts->ts_lock);

	return status;
}

celix_status_t pubsub_topicSubscriptionRemoveSerializer(topic_subscription_pt ts, pubsub_serializer_service_t* svc){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);
    if (ts->serializerSvc == svc) { //only act if svc removed is services used
        hash_map_iterator_t iter = hashMapIterator_construct(ts->msgSerializerMapMap);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(&iter);
            pubsub_subscriber_t* subsvc = hashMapEntry_getKey(entry);
            pubsub_msg_serializer_map_t* map = hashMapEntry_getValue(entry);
            ts->serializerSvc->destroySerializerMap(ts->serializerSvc->handle, map);
            hashMap_put(ts->msgSerializerMapMap, subsvc, NULL);
        }
        ts->serializerSvc = NULL;
    }
	celixThreadMutex_unlock(&ts->ts_lock);

	return status;
}

static celix_status_t topicsub_subscriberTracked(void * handle, service_reference_pt reference, void* svc) {
	celix_status_t status = CELIX_SUCCESS;
	topic_subscription_pt ts = handle;

	celixThreadMutex_lock(&ts->ts_lock);
    if (!hashMap_containsKey(ts->msgSerializerMapMap, svc)) {
        bundle_pt bundle = NULL;
        serviceReference_getBundle(reference, &bundle);

        if (ts->serializerSvc != NULL) {
            pubsub_msg_serializer_map_t* map = NULL;
            ts->serializerSvc->createSerializerMap(ts->serializerSvc->handle, bundle, &map);
            if (map != NULL) {
                hashMap_put(ts->msgSerializerMapMap, svc, map);
                hashMap_put(ts->bundleMap, svc, bundle);
            }
        }
    }
	celixThreadMutex_unlock(&ts->ts_lock);
	printf("TS: New subscriber registered.\n");
	return status;

}

static celix_status_t topicsub_subscriberUntracked(void * handle, service_reference_pt reference, void* svc) {
	celix_status_t status = CELIX_SUCCESS;
	topic_subscription_pt ts = handle;

	celixThreadMutex_lock(&ts->ts_lock);
    if (hashMap_containsKey(ts->msgSerializerMapMap, svc)) {
        pubsub_msg_serializer_map_t* map = hashMap_remove(ts->msgSerializerMapMap, svc);
        if (ts->serializerSvc != NULL){
            ts->serializerSvc->destroySerializerMap(ts->serializerSvc->handle, map);
            hashMap_remove(ts->bundleMap, svc);
            hashMap_remove(ts->msgSerializerMapMap, svc);
        }
    }
	celixThreadMutex_unlock(&ts->ts_lock);

	printf("TS: Subscriber unregistered.\n");
	return status;
}


static void process_msg(topic_subscription_pt sub, array_list_pt msg_list) {

	pubsub_msg_header_pt first_msg_hdr = (pubsub_msg_header_pt)zframe_data(((complete_zmq_msg_pt)arrayList_get(msg_list,0))->header);

	hash_map_iterator_t iter = hashMapIterator_construct(sub->msgSerializerMapMap);
	while (hashMapIterator_hasNext(&iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(&iter);
		pubsub_subscriber_pt subsvc = hashMapEntry_getKey(entry);
		pubsub_msg_serializer_map_t* map = hashMapEntry_getValue(entry);

		pubsub_msg_serializer_t* msgSer = hashMap_get(map->serializers, (void*)(uintptr_t )first_msg_hdr->type);
		if (msgSer == NULL) {
			printf("TS: Primary message %d not supported. NOT sending any part of the whole message.\n", first_msg_hdr->type);
		} else {
			void *msgInst = NULL;
			bool validVersion = checkVersion(msgSer->msgVersion, first_msg_hdr);
			if(validVersion){
				celix_status_t status = msgSer->deserialize(msgSer->handle, (const char*)zframe_data(((complete_zmq_msg_pt)arrayList_get(msg_list,0))->payload), 0, &msgInst);

				if (status == CELIX_SUCCESS) {
					bool release = true;

					mp_handle_t* mp_handle = create_mp_handle(sub, map, msg_list);
					pubsub_multipart_callbacks_t mp_callbacks;
					mp_callbacks.handle = mp_handle;
					mp_callbacks.localMsgTypeIdForMsgType = pubsub_localMsgTypeIdForMsgType;
					mp_callbacks.getMultipart = pubsub_getMultipart;
					subsvc->receive(subsvc->handle, msgSer->msgName, first_msg_hdr->type, msgInst, &mp_callbacks, &release);

					if (release) {
						msgSer->freeMsg(msgSer->handle, msgInst);
					}
					if (mp_handle!=NULL) {
						destroy_mp_handle(mp_handle);
					}
				}
				else{
					printf("TS: Cannot deserialize msgType %s.\n", msgSer->msgName);
				}

			} else {
				int major=0,minor=0;
				version_getMajor(msgSer->msgVersion, &major);
				version_getMinor(msgSer->msgVersion, &minor);
				printf("TS: Version mismatch for primary message '%s' (have %d.%d, received %u.%u). NOT sending any part of the whole message.\n",
					   msgSer->msgName, major, minor, first_msg_hdr->major, first_msg_hdr->minor);
			}
		}
	}

	int i = 0;
	for(;i<arrayList_size(msg_list);i++){
		complete_zmq_msg_pt c_msg = arrayList_get(msg_list,i);
		zframe_destroy(&(c_msg->header));
		zframe_destroy(&(c_msg->payload));
		free(c_msg);
	}

	arrayList_destroy(msg_list);

}

static void* zmq_recv_thread_func(void * arg) {
    topic_subscription_pt sub = (topic_subscription_pt) arg;

    while (sub->running) {

        celixThreadMutex_lock(&sub->socket_lock);

        zframe_t* headerMsg = zframe_recv(sub->zmq_socket);
        if (headerMsg == NULL) {
            if (errno == EINTR) {
                //It means we got a signal and we have to exit...
                printf("TS: header_recv thread for topic got a signal and will exit.\n");
            } else {
                perror("TS: header_recv thread");
            }
        } else {

			pubsub_msg_header_pt hdr = (pubsub_msg_header_pt) zframe_data(headerMsg);

			if (zframe_more(headerMsg)) {

				zframe_t* payloadMsg = zframe_recv(sub->zmq_socket);
				if (payloadMsg == NULL) {
					if (errno == EINTR) {
						//It means we got a signal and we have to exit...
						printf("TS: payload_recv thread for topic got a signal and will exit.\n");
					} else {
						perror("TS: payload_recv");
					}
					zframe_destroy(&headerMsg);
				} else {

					//Let's fetch all the messages from the socket
					array_list_pt msg_list = NULL;
					arrayList_create(&msg_list);
					complete_zmq_msg_pt firstMsg = calloc(1, sizeof(struct complete_zmq_msg));
					firstMsg->header = headerMsg;
					firstMsg->payload = payloadMsg;
					arrayList_add(msg_list, firstMsg);

					bool more = zframe_more(payloadMsg);
					while (more) {

						zframe_t* h_msg = zframe_recv(sub->zmq_socket);
						if (h_msg == NULL) {
							if (errno == EINTR) {
								//It means we got a signal and we have to exit...
								printf("TS: h_recv thread for topic got a signal and will exit.\n");
							} else {
								perror("TS: h_recv");
							}
							break;
						}

						zframe_t* p_msg = zframe_recv(sub->zmq_socket);
						if (p_msg == NULL) {
							if (errno == EINTR) {
								//It means we got a signal and we have to exit...
								printf("TS: p_recv thread for topic got a signal and will exit.\n");
							} else {
								perror("TS: p_recv");
							}
							zframe_destroy(&h_msg);
							break;
						}

						complete_zmq_msg_pt c_msg = calloc(1, sizeof(struct complete_zmq_msg));
						c_msg->header = h_msg;
						c_msg->payload = p_msg;
						arrayList_add(msg_list, c_msg);

						if (!zframe_more(p_msg)) {
							more = false;
						}
					}

					celixThreadMutex_lock(&sub->ts_lock);
					process_msg(sub, msg_list);
					celixThreadMutex_unlock(&sub->ts_lock);

				}

			} //zframe_more(headerMsg)
			else {
				free(headerMsg);
				printf("TS: received message %u for topic %s without payload!\n", hdr->type, hdr->topic);
			}

        } // headerMsg != NULL
        celixThreadMutex_unlock(&sub->socket_lock);
        connectPendingPublishers(sub);
        disconnectPendingPublishers(sub);
    } // while

    return NULL;
}

static void connectPendingPublishers(topic_subscription_pt sub) {
	celixThreadMutex_lock(&sub->pendingConnections_lock);
	while(!arrayList_isEmpty(sub->pendingConnections)) {
		char * pubEP = arrayList_remove(sub->pendingConnections, 0);
		pubsub_topicSubscriptionConnectPublisher(sub, pubEP);
		free(pubEP);
	}
	celixThreadMutex_unlock(&sub->pendingConnections_lock);
}

static void disconnectPendingPublishers(topic_subscription_pt sub) {
	celixThreadMutex_lock(&sub->pendingDisconnections_lock);
	while(!arrayList_isEmpty(sub->pendingDisconnections)) {
		char * pubEP = arrayList_remove(sub->pendingDisconnections, 0);
		pubsub_topicSubscriptionDisconnectPublisher(sub, pubEP);
		free(pubEP);
	}
	celixThreadMutex_unlock(&sub->pendingDisconnections_lock);
}

static void sigusr1_sighandler(int signo){
	printf("TS: Topic subscription being shut down...\n");
	return;
}

static bool checkVersion(version_pt msgVersion,pubsub_msg_header_pt hdr){
	bool check=false;
	int major=0,minor=0;

	if(msgVersion!=NULL){
		version_getMajor(msgVersion,&major);
		version_getMinor(msgVersion,&minor);
		if(hdr->major==((unsigned char)major)){ /* Different major means incompatible */
			check = (hdr->minor>=((unsigned char)minor)); /* Compatible only if the provider has a minor equals or greater (means compatible update) */
		}
	}

	return check;
}

static int pubsub_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId){
	*msgTypeId = utils_stringHash(msgType);
	return 0;
}

static int pubsub_getMultipart(void *handle, unsigned int msgTypeId, bool retain, void **part){

	if(handle==NULL){
		*part = NULL;
		return -1;
	}

	mp_handle_t* mp_handle = handle;
	msg_map_entry_pt entry = hashMap_get(mp_handle->rcv_msg_map,&msgTypeId);
	if(entry!=NULL){
		entry->retain = retain;
		*part = entry->msgInst;
	}
	else{
		printf("TP: getMultipart cannot find msg '%u'\n",msgTypeId);
		*part=NULL;
		return -2;
	}

	return 0;

}

static mp_handle_t* create_mp_handle(topic_subscription_pt sub, pubsub_msg_serializer_map_t* map, array_list_pt rcv_msg_list) {

	if(arrayList_size(rcv_msg_list)==1){ //Means it's not a multipart message
		return NULL;
	}

	mp_handle_t* mp_handle = calloc(1,sizeof(struct mp_handle));
	mp_handle->map = map;
	mp_handle->rcv_msg_map = hashMap_create(NULL, NULL, NULL, NULL);

	int i; //We skip the first message, it will be handle differently
	for (i=1 ; i<arrayList_size(rcv_msg_list) ; i++) {
		complete_zmq_msg_pt c_msg = arrayList_get(rcv_msg_list,i);
		pubsub_msg_header_pt header = (pubsub_msg_header_pt)zframe_data(c_msg->header);

		pubsub_msg_serializer_t* msgSer = hashMap_get(map->serializers, (void*)(uintptr_t)(header->type));
		if (msgSer != NULL) {
			void *msgInst = NULL;
			bool validVersion = checkVersion(msgSer->msgVersion, header);
			if(validVersion){
				//TODO make the getMultipart lazy?
				celix_status_t status = msgSer->deserialize(msgSer->handle, (const char*)zframe_data(c_msg->payload), 0, &msgInst);

				if(status == CELIX_SUCCESS){
					msg_map_entry_pt entry = calloc(1,sizeof(struct msg_map_entry));
					entry->msgInst = msgInst;
					hashMap_put(mp_handle->rcv_msg_map, (void*)(uintptr_t)(header->type), entry);
				}
			}
		}
	}
	return mp_handle;
}

static void destroy_mp_handle(mp_handle_t* mp_handle){

	hash_map_iterator_pt iter = hashMapIterator_create(mp_handle->rcv_msg_map);
	while(hashMapIterator_hasNext(iter)){
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		unsigned int msgId = (unsigned int)(uintptr_t)hashMapEntry_getKey(entry);
		msg_map_entry_pt msgEntry = hashMapEntry_getValue(entry);
		pubsub_msg_serializer_t* msgSer = hashMap_get(mp_handle->map->serializers, (void*)(uintptr_t)msgId);
		if (msgSer != NULL) {
			if (!msgEntry->retain) {
				msgSer->freeMsg(msgSer->handle, msgEntry->msgInst);
			}
		}
		else{
			printf("TS: ERROR: Cannot find pubsub_message_type for msg %u, so cannot destroy it!\n", msgId);
		}
	}
	hashMapIterator_destroy(iter);

	hashMap_destroy(mp_handle->rcv_msg_map,true,true);
	free(mp_handle);
}
