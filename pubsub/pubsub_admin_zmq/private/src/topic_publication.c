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
 * topic_publication.c
 *
 *  \date       Sep 24, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include "pubsub_publish_service_private.h"
#include <czmq.h>
/* The following undefs prevent the collision between:
 * - sys/syslog.h (which is included within czmq)
 * - celix/dfi/dfi_log_util.h
 */
#undef LOG_DEBUG
#undef LOG_WARNING
#undef LOG_INFO
#undef LOG_WARNING

#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "array_list.h"
#include "celixbool.h"
#include "service_registration.h"
#include "utils.h"
#include "service_factory.h"
#include "version.h"

#include "pubsub_common.h"
#include "dyn_msg_utils.h"
#include "pubsub_utils.h"
#include "publisher.h"

#include "pubsub_serializer.h"

#ifdef USE_ZMQ_SECURITY
	#include "zmq_crypto.h"

	#define MAX_CERT_PATH_LENGTH 512
#endif

#define EP_ADDRESS_LEN		32
#define ZMQ_BIND_MAX_RETRY	5

#define FIRST_SEND_DELAY	2

struct topic_publication {
	zsock_t* zmq_socket;
	zcert_t * zmq_cert;
	char* endpoint;
	service_registration_pt svcFactoryReg;
	array_list_pt pub_ep_list; //List<pubsub_endpoint>
	hash_map_pt boundServices; //<bundle_pt,bound_service>
	celix_thread_mutex_t tp_lock;
};

typedef struct publish_bundle_bound_service {
	topic_publication_pt parent;
	pubsub_publisher_pt service;
	bundle_pt bundle;
	char *topic;
	hash_map_pt msgTypes;
	unsigned short getCount;
	celix_thread_mutex_t mp_lock;
	bool mp_send_in_progress;
	array_list_pt mp_parts;
}* publish_bundle_bound_service_pt;

typedef struct pubsub_msg{
	pubsub_msg_header_pt header;
	char* payload;
	int payloadSize;
}* pubsub_msg_pt;

static unsigned int rand_range(unsigned int min, unsigned int max);

static celix_status_t pubsub_topicPublicationGetService(void* handle, bundle_pt bundle, service_registration_pt registration, void **service);
static celix_status_t pubsub_topicPublicationUngetService(void* handle, bundle_pt bundle, service_registration_pt registration, void **service);

static publish_bundle_bound_service_pt pubsub_createPublishBundleBoundService(topic_publication_pt tp,bundle_pt bundle);
static void pubsub_destroyPublishBundleBoundService(publish_bundle_bound_service_pt boundSvc);

static int pubsub_topicPublicationSend(void* handle,unsigned int msgTypeId, void *msg);
static int pubsub_topicPublicationSendMultipart(void *handle, unsigned int msgTypeId, void *msg, int flags);
static int pubsub_localMsgTypeIdForUUID(void* handle, const char* msgType, unsigned int* msgTypeId);

static void delay_first_send_for_late_joiners(void);

celix_status_t pubsub_topicPublicationCreate(bundle_context_pt bundle_context, pubsub_endpoint_pt pubEP,char* bindIP, unsigned int basePort, unsigned int maxPort, topic_publication_pt *out){
	celix_status_t status = CELIX_SUCCESS;

#ifdef USE_ZMQ_SECURITY
	char* keys_bundle_dir = pubsub_getKeysBundleDir(bundle_context);
	if (keys_bundle_dir == NULL){
		return CELIX_SERVICE_EXCEPTION;
	}

	const char* keys_file_path = NULL;
	const char* keys_file_name = NULL;
	bundleContext_getProperty(bundle_context, PROPERTY_KEYS_FILE_PATH, &keys_file_path);
	bundleContext_getProperty(bundle_context, PROPERTY_KEYS_FILE_NAME, &keys_file_name);

	char cert_path[MAX_CERT_PATH_LENGTH];

	//certificate path ".cache/bundle{id}/version0.0/./META-INF/keys/publisher/private/pub_{topic}.key"
	snprintf(cert_path, MAX_CERT_PATH_LENGTH, "%s/META-INF/keys/publisher/private/pub_%s.key.enc", keys_bundle_dir, pubEP->topic);
	free(keys_bundle_dir);
	printf("PSA: Loading key '%s'\n", cert_path);

	zcert_t* pub_cert = get_zcert_from_encoded_file((char *) keys_file_path, (char *) keys_file_name, cert_path);
	if (pub_cert == NULL){
		printf("PSA: Cannot load key '%s'\n", cert_path);
		return CELIX_SERVICE_EXCEPTION;
	}
#endif

	zsock_t* socket = zsock_new (ZMQ_PUB);
	if(socket==NULL){
		#ifdef USE_ZMQ_SECURITY
			zcert_destroy(&pub_cert);
		#endif

        perror("Error for zmq_socket");
		return CELIX_SERVICE_EXCEPTION;
	}
#ifdef USE_ZMQ_SECURITY
	zcert_apply (pub_cert, socket); // apply certificate to socket
	zsock_set_curve_server (socket, true); // setup the publisher's socket to use the curve functions
#endif

	int rv = -1, retry=0;
	char* ep = malloc(EP_ADDRESS_LEN);
    char bindAddress[EP_ADDRESS_LEN];

	while(rv==-1 && retry<ZMQ_BIND_MAX_RETRY){
		/* Randomized part due to same bundle publishing on different topics */
		unsigned int port = rand_range(basePort,maxPort);
		memset(ep,0,EP_ADDRESS_LEN);
        memset(bindAddress, 0, EP_ADDRESS_LEN);

		snprintf(ep,EP_ADDRESS_LEN,"tcp://%s:%u",bindIP,port);
        snprintf(bindAddress, EP_ADDRESS_LEN, "tcp://0.0.0.0:%u", port); //NOTE using a different bind addres than endpoint address
		rv = zsock_bind (socket, bindAddress);
        if (rv == -1) {
            perror("Error for zmq_bind");
        }
		retry++;
	}

	if(rv == -1){
		free(ep);
		return CELIX_SERVICE_EXCEPTION;
	}

	/* ZMQ stuffs are all fine at this point. Let's create and initialize the structure */

	topic_publication_pt pub = calloc(1,sizeof(*pub));

	arrayList_create(&(pub->pub_ep_list));
	pub->boundServices = hashMap_create(NULL,NULL,NULL,NULL);
	celixThreadMutex_create(&(pub->tp_lock),NULL);

	pub->endpoint = ep;
	pub->zmq_socket = socket;

	#ifdef USE_ZMQ_SECURITY
	pub->zmq_cert = pub_cert;
	#endif

	pubsub_topicPublicationAddPublisherEP(pub,pubEP);

	*out = pub;

	return status;
}

celix_status_t pubsub_topicPublicationDestroy(topic_publication_pt pub){
	celix_status_t status = CELIX_SUCCESS;

	celixThreadMutex_lock(&(pub->tp_lock));

	free(pub->endpoint);
	arrayList_destroy(pub->pub_ep_list);

	hash_map_iterator_pt iter = hashMapIterator_create(pub->boundServices);
	while(hashMapIterator_hasNext(iter)){
		publish_bundle_bound_service_pt bound = hashMapIterator_nextValue(iter);
		pubsub_destroyPublishBundleBoundService(bound);
	}
	hashMapIterator_destroy(iter);
	hashMap_destroy(pub->boundServices,false,false);

	pub->svcFactoryReg = NULL;
	zsock_destroy(&(pub->zmq_socket));
	#ifdef USE_ZMQ_SECURITY
	zcert_destroy(&(pub->zmq_cert));
	#endif

	celixThreadMutex_unlock(&(pub->tp_lock));

	celixThreadMutex_destroy(&(pub->tp_lock));

	free(pub);

	return status;
}

celix_status_t pubsub_topicPublicationStart(bundle_context_pt bundle_context,topic_publication_pt pub,service_factory_pt* svcFactory){
	celix_status_t status = CELIX_SUCCESS;

	/* Let's register the new service */
	//celixThreadMutex_lock(&(pub->tp_lock));

	pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(pub->pub_ep_list,0);

	if(pubEP!=NULL){
		service_factory_pt factory = calloc(1, sizeof(*factory));
		factory->handle = pub;
		factory->getService = pubsub_topicPublicationGetService;
		factory->ungetService = pubsub_topicPublicationUngetService;

		properties_pt props = properties_create();
		properties_set(props,PUBSUB_PUBLISHER_TOPIC,pubEP->topic);
		properties_set(props,PUBSUB_PUBLISHER_SCOPE,pubEP->scope);
		properties_set(props,"service.version", PUBSUB_PUBLISHER_SERVICE_VERSION);

		status = bundleContext_registerServiceFactory(bundle_context,PUBSUB_PUBLISHER_SERVICE_NAME,factory,props,&(pub->svcFactoryReg));

		if(status != CELIX_SUCCESS){
			properties_destroy(props);
			printf("PSA: Cannot register ServiceFactory for topic %s (bundle %ld).\n",pubEP->topic,pubEP->serviceID);
		}
		else{
			*svcFactory = factory;
		}
	}
	else{
		printf("PSA: Cannot find pubsub_endpoint after adding it...Should never happen!\n");
		status = CELIX_SERVICE_EXCEPTION;
	}

	//celixThreadMutex_unlock(&(pub->tp_lock));

	return status;
}

celix_status_t pubsub_topicPublicationStop(topic_publication_pt pub){
	celix_status_t status = CELIX_SUCCESS;

	//celixThreadMutex_lock(&(pub->tp_lock));

	status = serviceRegistration_unregister(pub->svcFactoryReg);

	//celixThreadMutex_unlock(&(pub->tp_lock));

	return status;
}

celix_status_t pubsub_topicPublicationAddPublisherEP(topic_publication_pt pub,pubsub_endpoint_pt ep){

	celixThreadMutex_lock(&(pub->tp_lock));
	ep->endpoint = strdup(pub->endpoint);
	arrayList_add(pub->pub_ep_list,ep);
	celixThreadMutex_unlock(&(pub->tp_lock));

	return CELIX_SUCCESS;
}

celix_status_t pubsub_topicPublicationRemovePublisherEP(topic_publication_pt pub,pubsub_endpoint_pt ep){

	celixThreadMutex_lock(&(pub->tp_lock));
	for (int i = 0; i < arrayList_size(pub->pub_ep_list); i++) {
	        pubsub_endpoint_pt e = arrayList_get(pub->pub_ep_list, i);
	        if(pubsubEndpoint_equals(ep, e)) {
	            arrayList_removeElement(pub->pub_ep_list,ep);
	            break;
	        }
	}
	celixThreadMutex_unlock(&(pub->tp_lock));

	return CELIX_SUCCESS;
}

array_list_pt pubsub_topicPublicationGetPublisherList(topic_publication_pt pub){
	return pub->pub_ep_list;
}


static celix_status_t pubsub_topicPublicationGetService(void* handle, bundle_pt bundle, service_registration_pt registration, void **service) {
	celix_status_t  status = CELIX_SUCCESS;

	topic_publication_pt publish = (topic_publication_pt)handle;

	celixThreadMutex_lock(&(publish->tp_lock));

	publish_bundle_bound_service_pt bound = (publish_bundle_bound_service_pt)hashMap_get(publish->boundServices,bundle);
	if(bound==NULL){
		bound = pubsub_createPublishBundleBoundService(publish,bundle);
		if(bound!=NULL){
			hashMap_put(publish->boundServices,bundle,bound);
		}
	}
	else{
		bound->getCount++;
	}

	*service = bound->service;

	celixThreadMutex_unlock(&(publish->tp_lock));

	return status;
}

static celix_status_t pubsub_topicPublicationUngetService(void* handle, bundle_pt bundle, service_registration_pt registration, void **service)  {

	topic_publication_pt publish = (topic_publication_pt)handle;

	celixThreadMutex_lock(&(publish->tp_lock));

	publish_bundle_bound_service_pt bound = (publish_bundle_bound_service_pt)hashMap_get(publish->boundServices,bundle);
	if(bound!=NULL){

		bound->getCount--;
		if(bound->getCount==0){
			pubsub_destroyPublishBundleBoundService(bound);
			hashMap_remove(publish->boundServices,bundle);
		}

	}
	else{
		long bundleId = -1;
		bundle_getBundleId(bundle,&bundleId);
		printf("TP: Unexpected ungetService call for bundle %ld.\n", bundleId);
	}

	/* service should be never used for unget, so let's set the pointer to NULL */
	*service = NULL;

	celixThreadMutex_unlock(&(publish->tp_lock));

	return CELIX_SUCCESS;
}

static bool send_pubsub_msg(zsock_t* zmq_socket, pubsub_msg_pt msg, bool last){

	bool ret = true;

	zframe_t* headerMsg = zframe_new(msg->header, sizeof(struct pubsub_msg_header));
	if (headerMsg == NULL) ret=false;
	zframe_t* payloadMsg = zframe_new(msg->payload, msg->payloadSize);
	if (payloadMsg == NULL) ret=false;

	delay_first_send_for_late_joiners();

	if( zframe_send(&headerMsg,zmq_socket, ZFRAME_MORE) == -1) ret=false;

	if(!last){
		if( zframe_send(&payloadMsg,zmq_socket, ZFRAME_MORE) == -1) ret=false;
	}
	else{
		if( zframe_send(&payloadMsg,zmq_socket, 0) == -1) ret=false;
	}

	if (!ret){
		zframe_destroy(&headerMsg);
		zframe_destroy(&payloadMsg);
	}

	free(msg->header);
	free(msg->payload);
	free(msg);

	return ret;

}

static bool send_pubsub_mp_msg(zsock_t* zmq_socket, array_list_pt mp_msg_parts){

	bool ret = true;

	unsigned int i = 0;
	unsigned int mp_num = arrayList_size(mp_msg_parts);
	for(;i<mp_num;i++){
		ret = ret && send_pubsub_msg(zmq_socket, (pubsub_msg_pt)arrayList_get(mp_msg_parts,i), (i==mp_num-1));
	}
	arrayList_clear(mp_msg_parts);

	return ret;

}

static int pubsub_topicPublicationSend(void* handle, unsigned int msgTypeId, void *msg) {

	return pubsub_topicPublicationSendMultipart(handle,msgTypeId,msg, PUBSUB_PUBLISHER_FIRST_MSG | PUBSUB_PUBLISHER_LAST_MSG);

}

static int pubsub_topicPublicationSendMultipart(void *handle, unsigned int msgTypeId, void *msg, int flags){

	int status = 0;

	publish_bundle_bound_service_pt bound = (publish_bundle_bound_service_pt) handle;

	celixThreadMutex_lock(&(bound->mp_lock));
	if( (flags & PUBSUB_PUBLISHER_FIRST_MSG) && !(flags & PUBSUB_PUBLISHER_LAST_MSG) && bound->mp_send_in_progress){ //means a real mp_msg
		printf("TP: Multipart send already in progress. Cannot process a new one.\n");
		celixThreadMutex_unlock(&(bound->mp_lock));
		return -3;
	}

	pubsub_message_type *msgType = hashMap_get(bound->msgTypes, &msgTypeId);

	int major=0, minor=0;

	if (msgType != NULL) {

		version_pt msgVersion = pubsubSerializer_getVersion(msgType);

		pubsub_msg_header_pt msg_hdr = calloc(1,sizeof(struct pubsub_msg_header));

		strncpy(msg_hdr->topic,bound->topic,MAX_TOPIC_LEN-1);

		msg_hdr->type = msgTypeId;
		if (msgVersion != NULL){
			version_getMajor(msgVersion, &major);
			version_getMinor(msgVersion, &minor);
			msg_hdr->major = major;
			msg_hdr->minor = minor;
		}

		void* serializedOutput = NULL;
		int serializedOutputLen = 0;
		pubsubSerializer_serialize(msgType, msg, &serializedOutput, &serializedOutputLen);

		pubsub_msg_pt msg = calloc(1,sizeof(struct pubsub_msg));
		msg->header = msg_hdr;
		msg->payload = (char *) serializedOutput;
		msg->payloadSize = serializedOutputLen;

		bool snd = true;

		switch(flags){
		case PUBSUB_PUBLISHER_FIRST_MSG:
			bound->mp_send_in_progress = true;
			arrayList_add(bound->mp_parts,msg);
			break;
		case PUBSUB_PUBLISHER_PART_MSG:
			if(!bound->mp_send_in_progress){
				printf("TP: ERROR: received msg part without the first part.\n");
				status = -4;
			}
			else{
				arrayList_add(bound->mp_parts,msg);
			}
			break;
		case PUBSUB_PUBLISHER_LAST_MSG:
			if(!bound->mp_send_in_progress){
				printf("TP: ERROR: received end msg without the first part.\n");
				status = -4;
			}
			else{
				arrayList_add(bound->mp_parts,msg);
				celixThreadMutex_lock(&(bound->parent->tp_lock));
				snd = send_pubsub_mp_msg(bound->parent->zmq_socket,bound->mp_parts);
				bound->mp_send_in_progress = false;
				celixThreadMutex_unlock(&(bound->parent->tp_lock));
			}
			break;
		case PUBSUB_PUBLISHER_FIRST_MSG | PUBSUB_PUBLISHER_LAST_MSG:	//Normal send case
			celixThreadMutex_lock(&(bound->parent->tp_lock));
			snd = send_pubsub_msg(bound->parent->zmq_socket,msg,true);
			celixThreadMutex_unlock(&(bound->parent->tp_lock));
			break;
		default:
			printf("TP: ERROR: Invalid MP flags combination\n");
			status = -4;
			break;
		}

		if(!snd){
			printf("TP: Failed to send %s message %u.\n",flags == (PUBSUB_PUBLISHER_FIRST_MSG | PUBSUB_PUBLISHER_LAST_MSG) ? "single" : "multipart", msgTypeId);
		}

	} else {
		printf("TP: Message %u not supported.",msgTypeId);
		status=-1;
	}

	celixThreadMutex_unlock(&(bound->mp_lock));

	return status;

}

static int pubsub_localMsgTypeIdForUUID(void* handle, const char* msgType, unsigned int* msgTypeId){
	*msgTypeId = pubsubSerializer_hashCode(msgType);
	return 0;
}

static unsigned int rand_range(unsigned int min, unsigned int max){

	double scaled = (double)(((double)rand())/((double)RAND_MAX));
	return (max-min+1)*scaled + min;

}

static publish_bundle_bound_service_pt pubsub_createPublishBundleBoundService(topic_publication_pt tp,bundle_pt bundle){

	publish_bundle_bound_service_pt bound = calloc(1, sizeof(*bound));

	if (bound != NULL) {
		bound->service = calloc(1, sizeof(*bound->service));
	}

	if (bound != NULL && bound->service != NULL) {

		bound->parent = tp;
		bound->bundle = bundle;
		bound->getCount = 1;
		bound->mp_send_in_progress = false;
		celixThreadMutex_create(&bound->mp_lock,NULL);
		bound->msgTypes = hashMap_create(uintHash, NULL, uintEquals, NULL); //<int* (msgId),pubsub_message_type>
		arrayList_create(&bound->mp_parts);

		pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(bound->parent->pub_ep_list,0);
		bound->topic=strdup(pubEP->topic);

		bound->service->handle = bound;
		bound->service->localMsgTypeIdForMsgType = pubsub_localMsgTypeIdForUUID;
		bound->service->send = pubsub_topicPublicationSend;
		bound->service->sendMultipart = pubsub_topicPublicationSendMultipart;

		pubsubSerializer_fillMsgTypesMap(bound->msgTypes,bound->bundle);

	}
	else
	{
		if (bound != NULL) {
			free(bound->service);
		}
		free(bound);
		return NULL;
	}

	return bound;
}

static void pubsub_destroyPublishBundleBoundService(publish_bundle_bound_service_pt boundSvc){

	celixThreadMutex_lock(&boundSvc->mp_lock);

	if(boundSvc->service != NULL){
		free(boundSvc->service);
	}

	if(boundSvc->msgTypes != NULL){
		pubsubSerializer_emptyMsgTypesMap(boundSvc->msgTypes);
		hashMap_destroy(boundSvc->msgTypes,false,false);
	}

	if(boundSvc->mp_parts!=NULL){
		arrayList_destroy(boundSvc->mp_parts);
	}

	if(boundSvc->topic!=NULL){
		free(boundSvc->topic);
	}

	celixThreadMutex_unlock(&boundSvc->mp_lock);
	celixThreadMutex_destroy(&boundSvc->mp_lock);

	free(boundSvc);

}

static void delay_first_send_for_late_joiners(){

	static bool firstSend = true;

	if(firstSend){
		printf("TP: Delaying first send for late joiners...\n");
		sleep(FIRST_SEND_DELAY);
		firstSend = false;
	}
}
