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

#include "pubsub/subscriber.h"
#include "pubsub/publisher.h"
#include "pubsub_utils.h"

#ifdef BUILD_WITH_ZMQ_SECURITY
#include "zmq_crypto.h"

#define MAX_CERT_PATH_LENGTH 512
#endif

#define POLL_TIMEOUT  	250
#define ZMQ_POLL_TIMEOUT_MS_ENV 	"ZMQ_POLL_TIMEOUT_MS"

#define PSA_ZMQ_RECEIVE_TIMEOUT_MICROSEC "PSA_ZMQ_RECEIVE_TIMEOUT_MICROSEC"
#define PSA_ZMQ_RECV_DEFAULT_TIMEOUT_STR "1000"
#define PSA_ZMQ_RECV_DEFAULT_TIMEOUT 1000


struct topic_subscription{

	zsock_t* zmq_socket;
	zcert_t * zmq_cert;
	zcert_t * zmq_pub_cert;
	pthread_mutex_t socket_lock;
	service_tracker_pt tracker;
	array_list_pt sub_ep_list;
	celix_thread_t recv_thread;
	bool running;
	celix_thread_mutex_t ts_lock;
	bundle_context_pt context;

	struct {
		const char* type;
		pubsub_serializer_service_t *svc;
	} serializer;

	hash_map_pt servicesMap; // key = service, value = msg types map

	celix_thread_mutex_t pendingConnections_lock;
	array_list_pt pendingConnections;

	array_list_pt pendingDisconnections;
	celix_thread_mutex_t pendingDisconnections_lock;

	unsigned int nrSubscribers;
	int zmqReceiveTimeout;
};

typedef struct complete_zmq_msg{
	zframe_t* header;
	zframe_t* payload;
}* complete_zmq_msg_pt;

typedef struct mp_handle{
	hash_map_pt svc_msg_db;
	hash_map_pt rcv_msg_map;
}* mp_handle_pt;

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
static mp_handle_pt create_mp_handle(hash_map_pt svc_msg_db,array_list_pt rcv_msg_list);
static void destroy_mp_handle(mp_handle_pt mp_handle);
static void connectPendingPublishers(topic_subscription_pt sub);
static void disconnectPendingPublishers(topic_subscription_pt sub);
static unsigned int get_zmq_receive_timeout(bundle_context_pt context);


static unsigned int get_zmq_receive_timeout(bundle_context_pt context) {
	unsigned int timeout;
	const char* timeout_str = NULL;
	bundleContext_getPropertyWithDefault(context,
										 PSA_ZMQ_RECEIVE_TIMEOUT_MICROSEC,
										 PSA_ZMQ_RECV_DEFAULT_TIMEOUT_STR,
										 &timeout_str);
	timeout = strtoul(timeout_str, NULL, 10);
	if (timeout == 0) {
		// on errror strtol returns 0
		timeout = PSA_ZMQ_RECV_DEFAULT_TIMEOUT;
	}

	return timeout;
}

celix_status_t pubsub_topicSubscriptionCreate(bundle_context_pt bundle_context, char* scope, char* topic, pubsub_serializer_service_t *best_serializer, const char *serType, topic_subscription_pt* out){
	celix_status_t status = CELIX_SUCCESS;

#ifdef BUILD_WITH_ZMQ_SECURITY
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

	printf("PSA_ZMQ_PSA_ZMQ_TS: Loading subscriber key '%s'\n", sub_cert_path);
	printf("PSA_ZMQ_PSA_ZMQ_TS: Loading publisher key '%s'\n", pub_cert_path);

	zcert_t* sub_cert = get_zcert_from_encoded_file((char *) keys_file_path, (char *) keys_file_name, sub_cert_path);
	if (sub_cert == NULL){
		printf("PSA_ZMQ_PSA_ZMQ_TS: Cannot load key '%s'\n", sub_cert_path);
		return CELIX_SERVICE_EXCEPTION;
	}

	zcert_t* pub_cert = zcert_load(pub_cert_path);
	if (pub_cert == NULL){
		zcert_destroy(&sub_cert);
		printf("PSA_ZMQ_PSA_ZMQ_TS: Cannot load key '%s'\n", pub_cert_path);
		return CELIX_SERVICE_EXCEPTION;
	}

	const char* pub_key = zcert_public_txt(pub_cert);
#endif

	zsock_t* zmq_s = zsock_new (ZMQ_SUB);
	if(zmq_s==NULL){
#ifdef BUILD_WITH_ZMQ_SECURITY
		zcert_destroy(&sub_cert);
		zcert_destroy(&pub_cert);
#endif

		return CELIX_SERVICE_EXCEPTION;
	}

#ifdef BUILD_WITH_ZMQ_SECURITY
	zcert_apply (sub_cert, zmq_s);
	zsock_set_curve_serverkey (zmq_s, pub_key); //apply key of publisher to socket of subscriber
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
	ts->serializer.svc = best_serializer;
	ts->zmqReceiveTimeout = get_zmq_receive_timeout(bundle_context);
#ifdef BUILD_WITH_ZMQ_SECURITY
	ts->zmq_cert = sub_cert;
	ts->zmq_pub_cert = pub_cert;
#endif

	celixThreadMutex_create(&ts->socket_lock, NULL);
	celixThreadMutex_create(&ts->ts_lock,NULL);
	arrayList_create(&ts->sub_ep_list);
	ts->servicesMap = hashMap_create(NULL, NULL, NULL, NULL);

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

	*out=ts;

	return status;
}

celix_status_t pubsub_topicSubscriptionDestroy(topic_subscription_pt ts){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&ts->ts_lock);

	ts->running = false;
	serviceTracker_destroy(ts->tracker);
	arrayList_clear(ts->sub_ep_list);
	arrayList_destroy(ts->sub_ep_list);
	/* TODO: Destroy all the serializer maps? */
	hashMap_destroy(ts->servicesMap,false,false);

	celixThreadMutex_lock(&ts->pendingConnections_lock);
	arrayList_destroy(ts->pendingConnections);
	celixThreadMutex_unlock(&ts->pendingConnections_lock);
	celixThreadMutex_destroy(&ts->pendingConnections_lock);

	celixThreadMutex_lock(&ts->pendingDisconnections_lock);
	arrayList_destroy(ts->pendingDisconnections);
	celixThreadMutex_unlock(&ts->pendingDisconnections_lock);
	celixThreadMutex_destroy(&ts->pendingDisconnections_lock);

	celixThreadMutex_unlock(&ts->ts_lock);

	celixThreadMutex_lock(&ts->socket_lock);
	zsock_destroy(&(ts->zmq_socket));
#ifdef BUILD_WITH_ZMQ_SECURITY
	zcert_destroy(&(ts->zmq_cert));
	zcert_destroy(&(ts->zmq_pub_cert));
#endif
	celixThreadMutex_unlock(&ts->socket_lock);
	celixThreadMutex_destroy(&ts->socket_lock);

	celixThreadMutex_destroy(&ts->ts_lock);

	free(ts);

	return status;
}

celix_status_t pubsub_topicSubscriptionStart(topic_subscription_pt ts){
	celix_status_t status = CELIX_SUCCESS;

	status = serviceTracker_open(ts->tracker);

	ts->running = true;

	if(status==CELIX_SUCCESS){
		status=celixThread_create(&ts->recv_thread,NULL,zmq_recv_thread_func,ts);
	}

	return status;
}

celix_status_t pubsub_topicSubscriptionStop(topic_subscription_pt ts){
	celix_status_t status = CELIX_SUCCESS;

	ts->running = false;

	pthread_kill(ts->recv_thread.thread,SIGUSR1);

	celixThread_join(ts->recv_thread,NULL);

	status = serviceTracker_close(ts->tracker);

	return status;
}

celix_status_t pubsub_topicSubscriptionConnectPublisher(topic_subscription_pt ts, char* pubURL){
	celix_status_t status = CELIX_SUCCESS;
	celixThreadMutex_lock(&ts->socket_lock);
	if(!zsock_is(ts->zmq_socket) || zsock_connect(ts->zmq_socket,"%s",pubURL) != 0){
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
	if(!zsock_is(ts->zmq_socket) || zsock_disconnect(ts->zmq_socket,"%s",pubURL) != 0){
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

array_list_pt pubsub_topicSubscriptionGetSubscribersList(topic_subscription_pt sub){
	return sub->sub_ep_list;
}

static celix_status_t topicsub_subscriberTracked(void * handle, service_reference_pt reference, void * service){
	celix_status_t status = CELIX_SUCCESS;
	topic_subscription_pt ts = handle;

	celixThreadMutex_lock(&ts->ts_lock);
	if (!hashMap_containsKey(ts->servicesMap, service)) {
		bundle_pt bundle = NULL;
		hash_map_pt msgTypes = NULL;

		serviceReference_getBundle(reference, &bundle);

		if(ts->serializer.svc != NULL && bundle!=NULL){
			ts->serializer.svc->createSerializerMap(ts->serializer.svc->handle,bundle,&msgTypes);
			if(msgTypes != NULL){
				hashMap_put(ts->servicesMap, service, msgTypes);
				printf("PSA_ZMQ_TS: New subscriber registered.\n");
			}
		}
		else{
			printf("PSA_ZMQ_TS: Cannot register new subscriber.\n");
			status = CELIX_SERVICE_EXCEPTION;
		}
	}
	celixThreadMutex_unlock(&ts->ts_lock);

	return status;
}

static celix_status_t topicsub_subscriberUntracked(void * handle, service_reference_pt reference, void * service){
	celix_status_t status = CELIX_SUCCESS;
	topic_subscription_pt ts = handle;

	celixThreadMutex_lock(&ts->ts_lock);
	if (hashMap_containsKey(ts->servicesMap, service)) {
		hash_map_pt msgTypes = hashMap_remove(ts->servicesMap, service);
		if(msgTypes!=NULL && ts->serializer.svc!=NULL){
			ts->serializer.svc->destroySerializerMap(ts->serializer.svc->handle,msgTypes);
			printf("PSA_ZMQ_TS: Subscriber unregistered.\n");
		}
		else{
			printf("PSA_ZMQ_TS: Cannot unregister subscriber.\n");
			status = CELIX_SERVICE_EXCEPTION;
		}
	}
	celixThreadMutex_unlock(&ts->ts_lock);

	return status;
}


static void process_msg(topic_subscription_pt sub,array_list_pt msg_list){

	pubsub_msg_header_pt first_msg_hdr = (pubsub_msg_header_pt)zframe_data(((complete_zmq_msg_pt)arrayList_get(msg_list,0))->header);

	hash_map_iterator_pt iter = hashMapIterator_create(sub->servicesMap);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		pubsub_subscriber_pt subsvc = hashMapEntry_getKey(entry);
		hash_map_pt msgTypes = hashMapEntry_getValue(entry);

		pubsub_msg_serializer_t *msgSer = hashMap_get(msgTypes,(void*)(uintptr_t )first_msg_hdr->type);
		if (msgSer == NULL) {
			printf("PSA_ZMQ_TS: Primary message %d not supported. NOT sending any part of the whole message.\n",first_msg_hdr->type);
		}
		else{
			void *msgInst = NULL;
			bool validVersion = checkVersion(msgSer->msgVersion,first_msg_hdr);

			if(validVersion){

				celix_status_t status = msgSer->deserialize(msgSer, (const void *) zframe_data(((complete_zmq_msg_pt)arrayList_get(msg_list,0))->payload), 0, &msgInst);

				if (status == CELIX_SUCCESS) {
					bool release = true;
					mp_handle_pt mp_handle = create_mp_handle(msgTypes,msg_list);
					pubsub_multipart_callbacks_t mp_callbacks;
					mp_callbacks.handle = mp_handle;
					mp_callbacks.localMsgTypeIdForMsgType = pubsub_localMsgTypeIdForMsgType;
					mp_callbacks.getMultipart = pubsub_getMultipart;
					subsvc->receive(subsvc->handle, msgSer->msgName, first_msg_hdr->type, msgInst, &mp_callbacks, &release);

					if(release){
						msgSer->freeMsg(msgSer,msgInst); // pubsubSerializer_freeMsg(msgType, msgInst);
					}
					if(mp_handle!=NULL){
						destroy_mp_handle(mp_handle);
					}
				}
				else{
					printf("PSA_ZMQ_TS: Cannot deserialize msgType %s.\n",msgSer->msgName);
				}

			}
			else{
				int major=0,minor=0;
				version_getMajor(msgSer->msgVersion,&major);
				version_getMinor(msgSer->msgVersion,&minor);
				printf("PSA_ZMQ_TS: Version mismatch for primary message '%s' (have %d.%d, received %u.%u). NOT sending any part of the whole message.\n",
						msgSer->msgName,major,minor,first_msg_hdr->major,first_msg_hdr->minor);
			}

		}
	}
	hashMapIterator_destroy(iter);

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

		zframe_t* headerMsg = zframe_recv_nowait(sub->zmq_socket);
		if (headerMsg == NULL) {
			if(errno == EAGAIN) {
				usleep(sub->zmqReceiveTimeout);
			} else if (errno == EINTR) {
				//It means we got a signal and we have to exit...
				printf("PSA_ZMQ_TS: header_recv thread for topic got a signal and will exit.\n");
			} else {
				perror("PSA_ZMQ_TS: header_recv thread");
			}
		}
		else {

			pubsub_msg_header_pt hdr = (pubsub_msg_header_pt) zframe_data(headerMsg);

			if (zframe_more(headerMsg)) {

				zframe_t* payloadMsg = zframe_recv(sub->zmq_socket);
				if (payloadMsg == NULL) {
					if (errno == EINTR) {
						//It means we got a signal and we have to exit...
						printf("PSA_ZMQ_TS: payload_recv thread for topic got a signal and will exit.\n");
					} else {
						perror("PSA_ZMQ_TS: payload_recv");
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
								printf("PSA_ZMQ_TS: h_recv thread for topic got a signal and will exit.\n");
							} else {
								perror("PSA_ZMQ_TS: h_recv");
							}
							break;
						}

						zframe_t* p_msg = zframe_recv(sub->zmq_socket);
						if (p_msg == NULL) {
							if (errno == EINTR) {
								//It means we got a signal and we have to exit...
								printf("PSA_ZMQ_TS: p_recv thread for topic got a signal and will exit.\n");
							} else {
								perror("PSA_ZMQ_TS: p_recv");
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
				printf("PSA_ZMQ_TS: received message %u for topic %s without payload!\n", hdr->type, hdr->topic);
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
	printf("PSA_ZMQ_TS: Topic subscription being shut down...\n");
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

	mp_handle_pt mp_handle = (mp_handle_pt)handle;
	msg_map_entry_pt entry = hashMap_get(mp_handle->rcv_msg_map, (void*)(uintptr_t) msgTypeId);
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

static mp_handle_pt create_mp_handle(hash_map_pt svc_msg_db,array_list_pt rcv_msg_list){

	if(arrayList_size(rcv_msg_list)==1){ //Means it's not a multipart message
		return NULL;
	}

	mp_handle_pt mp_handle = calloc(1,sizeof(struct mp_handle));
	mp_handle->svc_msg_db = svc_msg_db;
	mp_handle->rcv_msg_map = hashMap_create(NULL, NULL, NULL, NULL);

	int i=1; //We skip the first message, it will be handle differently
	for(;i<arrayList_size(rcv_msg_list);i++){
		complete_zmq_msg_pt c_msg = (complete_zmq_msg_pt)arrayList_get(rcv_msg_list,i);
		pubsub_msg_header_pt header = (pubsub_msg_header_pt)zframe_data(c_msg->header);

		pubsub_msg_serializer_t* msgSer = hashMap_get(svc_msg_db, (void*)(uintptr_t)(header->type));

		if (msgSer!= NULL) {
			void *msgInst = NULL;

			bool validVersion = checkVersion(msgSer->msgVersion,header);

			if(validVersion){
				celix_status_t status = msgSer->deserialize(msgSer, (const void*)zframe_data(c_msg->payload), 0, &msgInst);

				if(status == CELIX_SUCCESS){
					msg_map_entry_pt entry = calloc(1,sizeof(struct msg_map_entry));
					entry->msgInst = msgInst;
					hashMap_put(mp_handle->rcv_msg_map, (void*)(uintptr_t)header->type,entry);
				}
			}
		}
	}

	return mp_handle;

}

static void destroy_mp_handle(mp_handle_pt mp_handle){

	hash_map_iterator_pt iter = hashMapIterator_create(mp_handle->rcv_msg_map);
	while(hashMapIterator_hasNext(iter)){
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		unsigned int msgId = (unsigned int)(uintptr_t)hashMapEntry_getKey(entry);
		msg_map_entry_pt msgEntry = hashMapEntry_getValue(entry);
		pubsub_msg_serializer_t* msgSer = hashMap_get(mp_handle->svc_msg_db, (void*)(uintptr_t)msgId);

		if(msgSer!=NULL){
			if(!msgEntry->retain){
				msgSer->freeMsg(msgSer->handle,msgEntry->msgInst);
			}
		}
		else{
			printf("PSA_ZMQ_TS: ERROR: Cannot find messageSerializer for msg %u, so cannot destroy it!\n",msgId);
		}

		free(msgEntry);
	}
	hashMapIterator_destroy(iter);

	hashMap_destroy(mp_handle->rcv_msg_map,false,false);
	free(mp_handle);
}
