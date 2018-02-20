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

#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>

#include "array_list.h"
#include "celixbool.h"
#include "service_registration.h"
#include "utils.h"
#include "service_factory.h"
#include "version.h"

#include "topic_publication.h"
#include "pubsub_common.h"
#include "pubsub/publisher.h"
#include "large_udp.h"

#include "pubsub_serializer.h"
#include "pubsub_psa_udpmc_constants.h"

#define EP_ADDRESS_LEN		32

#define FIRST_SEND_DELAY	2

struct topic_publication {
	int sendSocket;
	char* endpoint;
	service_registration_pt svcFactoryReg;
	array_list_pt pub_ep_list; //List<pubsub_endpoint>
	hash_map_pt boundServices; //<bundle_pt,bound_service>
	celix_thread_mutex_t tp_lock;
	struct {
		const char* type;
		pubsub_serializer_service_t* svc;
	} serializer;
	struct sockaddr_in destAddr;
};

typedef struct publish_bundle_bound_service {
	topic_publication_pt parent;
	pubsub_publisher_t service;
	bundle_pt bundle;
	char *scope;
	char *topic;
	hash_map_pt msgTypes;
	unsigned short getCount;
	celix_thread_mutex_t mp_lock;
	largeUdp_pt largeUdpHandle;
}* publish_bundle_bound_service_pt;


typedef struct pubsub_msg{
	pubsub_msg_header_pt header;
	char* payload;
	unsigned int payloadSize;
} pubsub_msg_t;


static unsigned int rand_range(unsigned int min, unsigned int max);

static celix_status_t pubsub_topicPublicationGetService(void* handle, bundle_pt bundle, service_registration_pt registration, void **service);
static celix_status_t pubsub_topicPublicationUngetService(void* handle, bundle_pt bundle, service_registration_pt registration, void **service);

static publish_bundle_bound_service_pt pubsub_createPublishBundleBoundService(topic_publication_pt tp,bundle_pt bundle);
static void pubsub_destroyPublishBundleBoundService(publish_bundle_bound_service_pt boundSvc);

static int pubsub_topicPublicationSend(void* handle,unsigned int msgTypeId, const void *msg);

static int pubsub_localMsgTypeIdForUUID(void* handle, const char* msgType, unsigned int* msgTypeId);


static void delay_first_send_for_late_joiners(void);


celix_status_t pubsub_topicPublicationCreate(int sendSocket, pubsub_endpoint_pt pubEP, pubsub_serializer_service_t *best_serializer, const char* best_serializer_type, char* bindIP, topic_publication_pt *out){

	char* ep = malloc(EP_ADDRESS_LEN);
	memset(ep,0,EP_ADDRESS_LEN);

	long serviceId =strtol(properties_getWithDefault(pubEP->endpoint_props, PUBSUB_ENDPOINT_SERVICE_ID, "0"), NULL, 10);

	unsigned int port = serviceId + rand_range(UDP_BASE_PORT+serviceId+3, UDP_MAX_PORT);
	snprintf(ep,EP_ADDRESS_LEN,"udp://%s:%u",bindIP,port);


	topic_publication_pt pub = calloc(1,sizeof(*pub));

	arrayList_create(&(pub->pub_ep_list));
	pub->boundServices = hashMap_create(NULL,NULL,NULL,NULL);
	celixThreadMutex_create(&(pub->tp_lock),NULL);

	pub->endpoint = ep;
	pub->sendSocket = sendSocket;
	pub->destAddr.sin_family = AF_INET;
	pub->destAddr.sin_addr.s_addr = inet_addr(bindIP);
	pub->destAddr.sin_port = htons(port);

	pub->serializer.type = best_serializer_type;
	pub->serializer.svc = best_serializer;

	pubsub_topicPublicationAddPublisherEP(pub, pubEP);

	*out = pub;

	return CELIX_SUCCESS;
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
	pub->serializer.svc= NULL;
	pub->serializer.type= NULL;

	if(close(pub->sendSocket) != 0){
		status = CELIX_FILE_IO_EXCEPTION;
	}

	celixThreadMutex_unlock(&(pub->tp_lock));

	celixThreadMutex_destroy(&(pub->tp_lock));

	free(pub);

	return status;
}

celix_status_t pubsub_topicPublicationStart(bundle_context_pt bundle_context,topic_publication_pt pub,service_factory_pt* svcFactory){
	celix_status_t status = CELIX_SUCCESS;

	/* Let's register the new service */

	pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(pub->pub_ep_list,0);

	if(pubEP!=NULL){
		service_factory_pt factory = calloc(1, sizeof(*factory));
		factory->handle = pub;
		factory->getService = pubsub_topicPublicationGetService;
		factory->ungetService = pubsub_topicPublicationUngetService;

		properties_pt props = properties_create();
		properties_set(props,PUBSUB_PUBLISHER_SCOPE,properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE));
		properties_set(props,PUBSUB_PUBLISHER_TOPIC,properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
                properties_set(props,"service.version", PUBSUB_PUBLISHER_SERVICE_VERSION);


		status = bundleContext_registerServiceFactory(bundle_context,PUBSUB_PUBLISHER_SERVICE_NAME,factory,props,&(pub->svcFactoryReg));

		if(status != CELIX_SUCCESS){
			properties_destroy(props);
			printf("PSA_UDP_MC_PSA_UDP_MC_TP: Cannot register ServiceFactory for topic %s, topic %s (bundle %s).\n",
				   properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),
				   properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME),
				   properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_BUNDLE_ID));
		}
		else{
			*svcFactory = factory;
		}
	}
	else{
		printf("PSA_UDP_MC_PSA_UDP_MC_TP: Cannot find pubsub_endpoint after adding it...Should never happen!\n");
		status = CELIX_SERVICE_EXCEPTION;
	}

	return status;
}

celix_status_t pubsub_topicPublicationStop(topic_publication_pt pub){
	return serviceRegistration_unregister(pub->svcFactoryReg);
}

celix_status_t pubsub_topicPublicationAddPublisherEP(topic_publication_pt pub, pubsub_endpoint_pt ep) {

	celixThreadMutex_lock(&(pub->tp_lock));
	pubsubEndpoint_setField(ep, PUBSUB_ENDPOINT_URL, pub->endpoint);
	pubsubEndpoint_setField(ep, PUBSUB_ADMIN_TYPE_KEY, PSA_UDPMC_PUBSUB_ADMIN_TYPE);
	pubsubEndpoint_setField(ep, PUBSUB_SERIALIZER_TYPE_KEY, pub->serializer.type);
	arrayList_add(pub->pub_ep_list,ep);
	celixThreadMutex_unlock(&(pub->tp_lock));

	return CELIX_SUCCESS;
}

celix_status_t pubsub_topicPublicationRemovePublisherEP(topic_publication_pt pub,pubsub_endpoint_pt ep) {

	celixThreadMutex_lock(&(pub->tp_lock));
	arrayList_removeElement(pub->pub_ep_list,ep);
	celixThreadMutex_unlock(&(pub->tp_lock));

	return CELIX_SUCCESS;
}

array_list_pt pubsub_topicPublicationGetPublisherList(topic_publication_pt pub){
	array_list_pt list = NULL;
	celixThreadMutex_lock(&(pub->tp_lock));
	list = arrayList_clone(pub->pub_ep_list);
	celixThreadMutex_unlock(&(pub->tp_lock));
	return list;
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

	if (bound != NULL) {
		*service = &bound->service;
	}

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
		printf("PSA_UDP_MC_TP: Unexpected ungetService call for bundle %ld.\n", bundleId);
	}

	/* service should be never used for unget, so let's set the pointer to NULL */
	*service = NULL;

	celixThreadMutex_unlock(&(publish->tp_lock));

	return CELIX_SUCCESS;
}

static bool send_pubsub_msg(publish_bundle_bound_service_pt bound, pubsub_msg_t* msg, bool last, pubsub_release_callback_t *releaseCallback){
	const int iovec_len = 3; // header + size + payload
	bool ret = true;

	struct iovec msg_iovec[iovec_len];
	msg_iovec[0].iov_base = msg->header;
	msg_iovec[0].iov_len = sizeof(*msg->header);
	msg_iovec[1].iov_base = &msg->payloadSize;
	msg_iovec[1].iov_len = sizeof(msg->payloadSize);
	msg_iovec[2].iov_base = msg->payload;
	msg_iovec[2].iov_len = msg->payloadSize;

	delay_first_send_for_late_joiners();

	if(largeUdp_sendmsg(bound->largeUdpHandle, bound->parent->sendSocket, msg_iovec, iovec_len, 0, &bound->parent->destAddr, sizeof(bound->parent->destAddr)) == -1) {
		perror("send_pubsub_msg:sendSocket");
		ret = false;
	}

	if(releaseCallback) {
		releaseCallback->release(msg->payload, bound);
	}
	return ret;

}


static int pubsub_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *inMsg) {
	int status = 0;
	publish_bundle_bound_service_pt bound = (publish_bundle_bound_service_pt) handle;

	celixThreadMutex_lock(&(bound->parent->tp_lock));
	celixThreadMutex_lock(&(bound->mp_lock));

	pubsub_msg_serializer_t* msgSer = (pubsub_msg_serializer_t*)hashMap_get(bound->msgTypes, (void*)(intptr_t)msgTypeId);

	if (msgSer != NULL) {
		int major=0, minor=0;

		pubsub_msg_header_pt msg_hdr = calloc(1,sizeof(struct pubsub_msg_header));
		strncpy(msg_hdr->topic,bound->topic,MAX_TOPIC_LEN-1);
		msg_hdr->type = msgTypeId;


		if (msgSer->msgVersion != NULL){
			version_getMajor(msgSer->msgVersion, &major);
			version_getMinor(msgSer->msgVersion, &minor);
			msg_hdr->major = major;
			msg_hdr->minor = minor;
		}

		void* serializedOutput = NULL;
		size_t serializedOutputLen = 0;
		msgSer->serialize(msgSer,inMsg,&serializedOutput, &serializedOutputLen);

		pubsub_msg_t *msg = calloc(1,sizeof(pubsub_msg_t));
		msg->header = msg_hdr;
		msg->payload = (char*)serializedOutput;
		msg->payloadSize = serializedOutputLen;


		if(send_pubsub_msg(bound, msg,true, NULL) == false) {
			status = -1;
		}
		free(msg_hdr);
		free(msg);
		free(serializedOutput);


	} else {
		printf("PSA_UDP_MC_TP: No msg serializer available for msg type id %d\n", msgTypeId);
		status=-1;
	}

	celixThreadMutex_unlock(&(bound->mp_lock));
	celixThreadMutex_unlock(&(bound->parent->tp_lock));

	return status;
}

static int pubsub_localMsgTypeIdForUUID(void* handle, const char* msgType, unsigned int* msgTypeId){
	*msgTypeId = utils_stringHash(msgType);
	return 0;
}


static unsigned int rand_range(unsigned int min, unsigned int max){

	double scaled = (double)(((double)random())/((double)RAND_MAX));
	return (max-min+1)*scaled + min;

}

static publish_bundle_bound_service_pt pubsub_createPublishBundleBoundService(topic_publication_pt tp,bundle_pt bundle){

	publish_bundle_bound_service_pt bound = calloc(1, sizeof(*bound));

	if (bound != NULL) {

		bound->parent = tp;
		bound->bundle = bundle;
		bound->getCount = 1;
		celixThreadMutex_create(&bound->mp_lock,NULL);

		if (tp->serializer.svc != NULL){
			tp->serializer.svc->createSerializerMap(tp->serializer.svc->handle,bundle,&bound->msgTypes);
		}

		pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(bound->parent->pub_ep_list,0);
		bound->scope=strdup(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE));
		bound->topic=strdup(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
		bound->largeUdpHandle = largeUdp_create(1);

		bound->service.handle = bound;
		bound->service.localMsgTypeIdForMsgType = pubsub_localMsgTypeIdForUUID;
		bound->service.send = pubsub_topicPublicationSend;
		bound->service.sendMultipart = NULL;  //Multipart not supported for UDP

	}

	return bound;
}

static void pubsub_destroyPublishBundleBoundService(publish_bundle_bound_service_pt boundSvc){

	celixThreadMutex_lock(&boundSvc->mp_lock);

	if(boundSvc->parent->serializer.svc != NULL && boundSvc->msgTypes != NULL){
		boundSvc->parent->serializer.svc->destroySerializerMap(boundSvc->parent->serializer.svc->handle, boundSvc->msgTypes);
	}

	if(boundSvc->scope!=NULL){
		free(boundSvc->scope);
	}

	if(boundSvc->topic!=NULL){
		free(boundSvc->topic);
	}

	largeUdp_destroy(boundSvc->largeUdpHandle);

	celixThreadMutex_unlock(&boundSvc->mp_lock);
	celixThreadMutex_destroy(&boundSvc->mp_lock);

	free(boundSvc);

}

static void delay_first_send_for_late_joiners(){

	static bool firstSend = true;

	if(firstSend){
		printf("PSA_UDP_MC_TP: Delaying first send for late joiners...\n");
		sleep(FIRST_SEND_DELAY);
		firstSend = false;
	}
}
