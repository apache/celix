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
#include "pubsub_utils.h"
#include "pubsub/publisher.h"

#include "topic_publication.h"

#include "pubsub_serializer.h"
#include "pubsub_psa_zmq_constants.h"

#ifdef BUILD_WITH_ZMQ_SECURITY
	#include "zmq_crypto.h"

	#define MAX_CERT_PATH_LENGTH 512
#endif

#define EP_ADDRESS_LEN		32
#define ZMQ_BIND_MAX_RETRY	5

#define FIRST_SEND_DELAY	2

struct topic_publication {
	zsock_t* zmq_socket;
	celix_thread_mutex_t socket_lock; //Protects zmq_socket access
	zcert_t * zmq_cert;
	char* endpoint;
	service_registration_pt svcFactoryReg;
	array_list_pt pub_ep_list; //List<pubsub_endpoint>
	hash_map_pt boundServices; //<bundle_pt,bound_service>
	struct {
		const char* type;
		pubsub_serializer_service_t *svc;
	} serializer;
	celix_thread_mutex_t tp_lock;
};

typedef struct publish_bundle_bound_service {
	topic_publication_pt parent;
	pubsub_publisher_t service;
	bundle_pt bundle;
	char *topic;
	hash_map_pt msgTypes;
	unsigned short getCount;
	celix_thread_mutex_t mp_lock; //Protects publish_bundle_bound_service data structure
	bool mp_send_in_progress;
	array_list_pt mp_parts;
}* publish_bundle_bound_service_pt;

/* Note: correct locking order is
 * 1. tp_lock
 * 2. mp_lock
 * 3. socket_lock
 *
 * tp_lock and socket_lock are independent.
 */

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

static int pubsub_topicPublicationSend(void* handle,unsigned int msgTypeId, const void *msg);
static int pubsub_topicPublicationSendMultipart(void *handle, unsigned int msgTypeId, const void *inMsg, int flags);
static int pubsub_localMsgTypeIdForUUID(void* handle, const char* msgType, unsigned int* msgTypeId);

static void delay_first_send_for_late_joiners(void);

celix_status_t pubsub_topicPublicationCreate(bundle_context_pt bundle_context, pubsub_endpoint_pt pubEP, pubsub_serializer_service_t *best_serializer, const char* serType, char* bindIP, unsigned int basePort, unsigned int maxPort, topic_publication_pt *out){
	celix_status_t status = CELIX_SUCCESS;

#ifdef BUILD_WITH_ZMQ_SECURITY
	char* secure_topics = NULL;
	bundleContext_getProperty(bundle_context, "SECURE_TOPICS", (const char **) &secure_topics);

	if (secure_topics){
		array_list_pt secure_topics_list = pubsub_getTopicsFromString(secure_topics);

		int i;
		int secure_topics_size = arrayList_size(secure_topics_list);
		for (i = 0; i < secure_topics_size; i++){
			char* top = arrayList_get(secure_topics_list, i);
			if (strcmp(pubEP->topic, top) == 0){
				printf("PSA_ZMQ_TP: Secure topic: '%s'\n", top);
				pubEP->is_secure = true;
			}
			free(top);
			top = NULL;
		}

		arrayList_destroy(secure_topics_list);
	}

	zcert_t* pub_cert = NULL;
	if (pubEP->is_secure){
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
		printf("PSA_ZMQ_TP: Loading key '%s'\n", cert_path);

		pub_cert = get_zcert_from_encoded_file((char *) keys_file_path, (char *) keys_file_name, cert_path);
		if (pub_cert == NULL){
			printf("PSA_ZMQ_TP: Cannot load key '%s'\n", cert_path);
			printf("PSA_ZMQ_TP: Topic '%s' NOT SECURED !\n", pubEP->topic);
			pubEP->is_secure = false;
		}
	}
#endif

	zsock_t* socket = zsock_new (ZMQ_PUB);
	if(socket==NULL){
		#ifdef BUILD_WITH_ZMQ_SECURITY
			if (pubEP->is_secure){
				zcert_destroy(&pub_cert);
			}
		#endif

        perror("Error for zmq_socket");
		return CELIX_SERVICE_EXCEPTION;
	}
#ifdef BUILD_WITH_ZMQ_SECURITY
	if (pubEP->is_secure){
		zcert_apply (pub_cert, socket); // apply certificate to socket
		zsock_set_curve_server (socket, true); // setup the publisher's socket to use the curve functions
	}
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
        snprintf(bindAddress, EP_ADDRESS_LEN, "tcp://0.0.0.0:%u", port); //NOTE using a different bind address than endpoint address
		rv = zsock_bind (socket, "%s", bindAddress);
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
	pub->serializer.svc = best_serializer;
	pub->serializer.type = serType;

	celixThreadMutex_create(&(pub->socket_lock),NULL);

#ifdef BUILD_WITH_ZMQ_SECURITY
	if (pubEP->is_secure){
		pub->zmq_cert = pub_cert;
	}
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
	pub->serializer.svc = NULL;
	pub->serializer.type = NULL;
#ifdef BUILD_WITH_ZMQ_SECURITY
	zcert_destroy(&(pub->zmq_cert));
#endif

	celixThreadMutex_unlock(&(pub->tp_lock));

	celixThreadMutex_destroy(&(pub->tp_lock));

	celixThreadMutex_lock(&(pub->socket_lock));
	zsock_destroy(&(pub->zmq_socket));
	celixThreadMutex_unlock(&(pub->socket_lock));

	celixThreadMutex_destroy(&(pub->socket_lock));

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
		properties_set(props,PUBSUB_PUBLISHER_TOPIC,properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
		properties_set(props,PUBSUB_PUBLISHER_SCOPE,properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE));
		properties_set(props,"service.version", PUBSUB_PUBLISHER_SERVICE_VERSION);

		status = bundleContext_registerServiceFactory(bundle_context,PUBSUB_PUBLISHER_SERVICE_NAME,factory,props,&(pub->svcFactoryReg));

		if(status != CELIX_SUCCESS){
			properties_destroy(props);
			printf("PSA_ZMQ_PSA_ZMQ_TP: Cannot register ServiceFactory for topic %s (bundle %s).\n",
				   properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME),
				   properties_get(pubEP->endpoint_props, PUBSUB_BUNDLE_ID));
		}
		else{
			*svcFactory = factory;
		}
	}
	else{
		printf("PSA_ZMQ_PSA_ZMQ_TP: Cannot find pubsub_endpoint after adding it...Should never happen!\n");
		status = CELIX_SERVICE_EXCEPTION;
	}

	return status;
}

celix_status_t pubsub_topicPublicationStop(topic_publication_pt pub){
	return serviceRegistration_unregister(pub->svcFactoryReg);
}

celix_status_t pubsub_topicPublicationAddPublisherEP(topic_publication_pt pub, pubsub_endpoint_pt ep) {

	celixThreadMutex_lock(&(pub->tp_lock));
	pubsubEndpoint_setField(ep, PUBSUB_ADMIN_TYPE_KEY, PSA_ZMQ_PUBSUB_ADMIN_TYPE);
	pubsubEndpoint_setField(ep, PUBSUB_SERIALIZER_TYPE_KEY, pub->serializer.type);
    pubsubEndpoint_setField(ep, PUBSUB_ENDPOINT_URL, pub->endpoint);
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

	*service = &bound->service;

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
		printf("PSA_ZMQ_TP: Unexpected ungetService call for bundle %ld.\n", bundleId);
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

static int pubsub_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *msg) {

	return pubsub_topicPublicationSendMultipart(handle,msgTypeId,msg, PUBSUB_PUBLISHER_FIRST_MSG | PUBSUB_PUBLISHER_LAST_MSG);

}

static int pubsub_topicPublicationSendMultipart(void *handle, unsigned int msgTypeId, const void *inMsg, int flags){

	int status = 0;

	publish_bundle_bound_service_pt bound = (publish_bundle_bound_service_pt) handle;

	celixThreadMutex_lock(&(bound->parent->tp_lock));
	celixThreadMutex_lock(&(bound->mp_lock));
	if( (flags & PUBSUB_PUBLISHER_FIRST_MSG) && !(flags & PUBSUB_PUBLISHER_LAST_MSG) && bound->mp_send_in_progress){ //means a real mp_msg
		printf("PSA_ZMQ_TP: Multipart send already in progress. Cannot process a new one.\n");
		celixThreadMutex_unlock(&(bound->mp_lock));
		celixThreadMutex_unlock(&(bound->parent->tp_lock));
		return -3;
	}

	pubsub_msg_serializer_t* msgSer = (pubsub_msg_serializer_t*)hashMap_get(bound->msgTypes, (void*)(uintptr_t)msgTypeId);

	if (msgSer!= NULL) {
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

		void *serializedOutput = NULL;
		size_t serializedOutputLen = 0;
		msgSer->serialize(msgSer,inMsg,&serializedOutput, &serializedOutputLen);

		pubsub_msg_pt msg = calloc(1,sizeof(struct pubsub_msg));
		msg->header = msg_hdr;
		msg->payload = (char*)serializedOutput;
		msg->payloadSize = serializedOutputLen;
		bool snd = true;

		switch(flags){
		case PUBSUB_PUBLISHER_FIRST_MSG:
			bound->mp_send_in_progress = true;
			arrayList_add(bound->mp_parts,msg);
			break;
		case PUBSUB_PUBLISHER_PART_MSG:
			if(!bound->mp_send_in_progress){
				printf("PSA_ZMQ_TP: ERROR: received msg part without the first part.\n");
				status = -4;
			}
			else{
				arrayList_add(bound->mp_parts,msg);
			}
			break;
		case PUBSUB_PUBLISHER_LAST_MSG:
			if(!bound->mp_send_in_progress){
				printf("PSA_ZMQ_TP: ERROR: received end msg without the first part.\n");
				status = -4;
			}
			else{
				arrayList_add(bound->mp_parts,msg);
				snd = send_pubsub_mp_msg(bound->parent->zmq_socket,bound->mp_parts);
				bound->mp_send_in_progress = false;
			}
			break;
		case PUBSUB_PUBLISHER_FIRST_MSG | PUBSUB_PUBLISHER_LAST_MSG:	//Normal send case
			snd = send_pubsub_msg(bound->parent->zmq_socket,msg,true);
			break;
		default:
			printf("PSA_ZMQ_TP: ERROR: Invalid MP flags combination\n");
			status = -4;
			break;
		}

		if(status==-4){
			free(msg);
		}

		if(!snd){
			printf("PSA_ZMQ_TP: Failed to send %s message %u.\n",flags == (PUBSUB_PUBLISHER_FIRST_MSG | PUBSUB_PUBLISHER_LAST_MSG) ? "single" : "multipart", msgTypeId);
		}

	} else {
        printf("PSA_ZMQ_TP: No msg serializer available for msg type id %d\n", msgTypeId);
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

	//PRECOND lock on tp->lock

	publish_bundle_bound_service_pt bound = calloc(1, sizeof(*bound));

	if (bound != NULL) {

		bound->parent = tp;
		bound->bundle = bundle;
		bound->getCount = 1;
		bound->mp_send_in_progress = false;
		celixThreadMutex_create(&bound->mp_lock,NULL);

		if(tp->serializer.svc != NULL){
			tp->serializer.svc->createSerializerMap(tp->serializer.svc->handle,bundle,&bound->msgTypes);
		}

		arrayList_create(&bound->mp_parts);

		pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(bound->parent->pub_ep_list,0);
		bound->topic=strdup(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));

		bound->service.handle = bound;
		bound->service.localMsgTypeIdForMsgType = pubsub_localMsgTypeIdForUUID;
		bound->service.send = pubsub_topicPublicationSend;
		bound->service.sendMultipart = pubsub_topicPublicationSendMultipart;

	}

	return bound;
}

static void pubsub_destroyPublishBundleBoundService(publish_bundle_bound_service_pt boundSvc){

	//PRECOND lock on tp->lock

	celixThreadMutex_lock(&boundSvc->mp_lock);


	if(boundSvc->parent->serializer.svc != NULL && boundSvc->msgTypes != NULL){
		boundSvc->parent->serializer.svc->destroySerializerMap(boundSvc->parent->serializer.svc->handle, boundSvc->msgTypes);
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
		printf("PSA_ZMQ_TP: Delaying first send for late joiners...\n");
		sleep(FIRST_SEND_DELAY);
		firstSend = false;
	}
}
