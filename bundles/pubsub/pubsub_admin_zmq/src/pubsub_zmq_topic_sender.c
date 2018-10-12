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

#include <pubsub_serializer.h>
#include <stdlib.h>
#include <memory.h>
#include <pubsub_constants.h>
#include <pubsub/publisher.h>
#include <utils.h>
#include <pubsub_common.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <czmq.h>
#include <log_helper.h>
#include "pubsub_zmq_topic_sender.h"
#include "pubsub_psa_zmq_constants.h"
#include "pubsub_zmq_common.h"

#define FIRST_SEND_DELAY_IN_SECONDS             2
#define ZMQ_BIND_MAX_RETRY                      10

#define L_DEBUG(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(sender->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

struct pubsub_zmq_topic_sender {
    celix_bundle_context_t *ctx;
    log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;

    char *scope;
    char *topic;
    char *url;

    struct {
        celix_thread_mutex_t mutex;
        zsock_t *socket;
        zcert_t *cert;
    } zmq;

    struct {
        long svcId;
        celix_service_factory_t factory;
    } publisher;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map;  //key = bndId, value = psa_zmq_bounded_service_entry_t
    } boundedServices;
};

typedef struct psa_zmq_bounded_service_entry {
    pubsub_zmq_topic_sender_t *parent;
    pubsub_publisher_t service;
    long bndId;
    hash_map_t *msgTypes;
    int getCount;

    struct {
        celix_thread_mutex_t mutex;
        bool inProgress;
        celix_array_list_t *parts;
    } multipart;
} psa_zmq_bounded_service_entry_t;


typedef struct pubsub_msg {
    pubsub_msg_header_t *header;
    char* payload;
    int payloadSize;
} pubsub_msg_t;

static void* psa_zmq_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static void psa_zmq_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static unsigned int rand_range(unsigned int min, unsigned int max);
static void delay_first_send_for_late_joiners(pubsub_zmq_topic_sender_t *sender);

static int psa_zmq_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *msg);
static int psa_zmq_topicPublicationSendMultipart(void *handle, unsigned int msgTypeId, const void *inMsg, int flags);

pubsub_zmq_topic_sender_t* pubsub_zmqTopicSender_create(
        celix_bundle_context_t *ctx,
        log_helper_t *logHelper,
        const char *scope,
        const char *topic,
        long serializerSvcId,
        pubsub_serializer_service_t *ser,
        const char *bindIP,
        unsigned int basePort,
        unsigned int maxPort) {
    pubsub_zmq_topic_sender_t *sender = calloc(1, sizeof(*sender));
    sender->ctx = ctx;
    sender->logHelper = logHelper;
    sender->serializerSvcId = serializerSvcId;
    sender->serializer = ser;

    //setting up zmq socket for ZMQ TopicSender
    {
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

        zsock_t* socket = zsock_new(ZMQ_PUB);
        if (socket==NULL) {
#ifdef BUILD_WITH_ZMQ_SECURITY
            if (pubEP->is_secure) {
				zcert_destroy(&pub_cert);
			}
#endif
            perror("Error for zmq_socket");
        }
#ifdef BUILD_WITH_ZMQ_SECURITY
        if (pubEP->is_secure) {
		    zcert_apply (pub_cert, socket); // apply certificate to socket
		    zsock_set_curve_server (socket, true); // setup the publisher's socket to use the curve functions
	    }
#endif

        int rv = -1, retry=0;
        while(rv==-1 && retry < ZMQ_BIND_MAX_RETRY ) {
            /* Randomized part due to same bundle publishing on different topics */
            unsigned int port = rand_range(basePort,maxPort);

            size_t len = (size_t)snprintf(NULL, 0, "tcp://%s:%u", bindIP, port) + 1;
            char *url = calloc(len, sizeof(char*));
            snprintf(url, len, "tcp://%s:%u", bindIP, port);

            len = (size_t)snprintf(NULL, 0, "tcp://0.0.0.0:%u", port) + 1;
            char *bindUrl = calloc(len, sizeof(char));
            snprintf(bindUrl, len, "tcp://0.0.0.0:%u", port);

            rv = zsock_bind (socket, "%s", bindUrl);
            if (rv == -1) {
                perror("Error for zmq_bind");
                free(url);
            } else {
                sender->url = url;
                sender->zmq.socket = socket;
            }
            retry++;
            free(bindUrl);
        }
    }

    if (sender->url != NULL) {
        sender->scope = strndup(scope, 1024 * 1024);
        sender->topic = strndup(topic, 1024 * 1024);

        celixThreadMutex_create(&sender->boundedServices.mutex, NULL);
        celixThreadMutex_create(&sender->zmq.mutex, NULL);
        sender->boundedServices.map = hashMap_create(NULL, NULL, NULL, NULL);
    }

    //register publisher services using a service factory
    if (sender->url != NULL) {
        sender->publisher.factory.handle = sender;
        sender->publisher.factory.getService = psa_zmq_getPublisherService;
        sender->publisher.factory.ungetService = psa_zmq_ungetPublisherService;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_PUBLISHER_TOPIC, sender->topic);
        celix_properties_set(props, PUBSUB_PUBLISHER_SCOPE, sender->scope);

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.factory = &sender->publisher.factory;
        opts.serviceName = PUBSUB_PUBLISHER_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_PUBLISHER_SERVICE_VERSION;
        opts.properties = props;

        sender->publisher.svcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);
    }

    if (sender->url == NULL) {
        free(sender);
        sender = NULL;
    }

    return sender;
}

void pubsub_zmqTopicSender_destroy(pubsub_zmq_topic_sender_t *sender) {
    if (sender != NULL) {
        celix_bundleContext_unregisterService(sender->ctx, sender->publisher.svcId);

        zsock_destroy(&sender->zmq.socket);

        celixThreadMutex_lock(&sender->boundedServices.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(sender->boundedServices.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_zmq_bounded_service_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                sender->serializer->destroySerializerMap(sender->serializer->handle, entry->msgTypes);
                celixThreadMutex_destroy(&entry->multipart.mutex);
                for (int i = 0; i < celix_arrayList_size(entry->multipart.parts); ++i) {
                    pubsub_msg_t *msg = celix_arrayList_get(entry->multipart.parts, i);
                    free(msg->header);
                    free(msg->payload);
                    free(msg);
                }
                free(entry);
            }
        }
        hashMap_destroy(sender->boundedServices.map, false, false);
        celixThreadMutex_unlock(&sender->boundedServices.mutex);

        celixThreadMutex_destroy(&sender->boundedServices.mutex);
        celixThreadMutex_destroy(&sender->zmq.mutex);

        free(sender->scope);
        free(sender->topic);
        free(sender->url);
        free(sender);
    }
}

long pubsub_zmqTopicSender_serializerSvcId(pubsub_zmq_topic_sender_t *sender) {
    return sender->serializerSvcId;
}

const char* pubsub_zmqTopicSender_scope(pubsub_zmq_topic_sender_t *sender) {
    return sender->scope;
}

const char* pubsub_zmqTopicSender_topic(pubsub_zmq_topic_sender_t *sender) {
    return sender->topic;
}

const char* pubsub_zmqTopicSender_url(pubsub_zmq_topic_sender_t *sender) {
    return sender->url;
}

void pubsub_zmqTopicSender_connectTo(pubsub_zmq_topic_sender_t *sender, const celix_properties_t *endpoint) {
    //TODO subscriber count -> topic info
}

void pubsub_zmqTopicSender_disconnectFrom(pubsub_zmq_topic_sender_t *sender, const celix_properties_t *endpoint) {
    //TODO
}

static void* psa_zmq_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_zmq_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_zmq_bounded_service_entry_t *entry = hashMap_get(sender->boundedServices.map, (void*)bndId);
    if (entry != NULL) {
        entry->getCount += 1;
    } else {
        entry = calloc(1, sizeof(*entry));
        entry->getCount = 1;
        entry->parent = sender;
        entry->bndId = bndId;

        int rc = sender->serializer->createSerializerMap(sender->serializer->handle, (celix_bundle_t*)requestingBundle, &entry->msgTypes);
        if (rc == 0) {
            entry->service.handle = entry;
            entry->service.localMsgTypeIdForMsgType = psa_zmq_localMsgTypeIdForMsgType;
            entry->service.send = psa_zmq_topicPublicationSend;
            entry->service.sendMultipart = psa_zmq_topicPublicationSendMultipart;
            hashMap_put(sender->boundedServices.map, (void*)bndId, entry);
        } else {
            L_ERROR("Error creating serializer map for ZMQ TopicSender %s/%s", sender->scope, sender->topic);
        }



    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);

    return &entry->service;
}

static void psa_zmq_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties __attribute__((unused))) {
    pubsub_zmq_topic_sender_t *sender = handle;
    long bndId = celix_bundle_getId(requestingBundle);

    celixThreadMutex_lock(&sender->boundedServices.mutex);
    psa_zmq_bounded_service_entry_t *entry = hashMap_get(sender->boundedServices.map, (void*)bndId);
    if (entry != NULL) {
        entry->getCount -= 1;
    }
    if (entry != NULL && entry->getCount == 0) {
        //free entry
        hashMap_remove(sender->boundedServices.map, (void*)bndId);
        int rc = sender->serializer->destroySerializerMap(sender->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            L_ERROR("Error destroying publisher service, serializer not available / cannot get msg serializer map\n");
        }

        free(entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);
}

static int psa_zmq_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *msg) {
    return psa_zmq_topicPublicationSendMultipart( handle, msgTypeId, msg, PUBSUB_PUBLISHER_FIRST_MSG | PUBSUB_PUBLISHER_LAST_MSG);
}

static bool psa_zmq_sendMsg(pubsub_zmq_topic_sender_t *sender, pubsub_msg_t *msg, bool last){

    bool ret = true;

    zframe_t* headerMsg = zframe_new(msg->header, sizeof(struct pubsub_msg_header));
    if (headerMsg == NULL) ret=false;
    zframe_t* payloadMsg = zframe_new(msg->payload, (size_t)msg->payloadSize);
    if (payloadMsg == NULL) ret=false;

    delay_first_send_for_late_joiners(sender);

    celixThreadMutex_lock(&sender->zmq.mutex);
    if (sender->zmq.socket == NULL) {
        L_ERROR("[PSA_ZMQ] TopicSender zmq socket is NULL");
    } else {
        if (zframe_send(&headerMsg, sender->zmq.socket, ZFRAME_MORE) == -1) ret = false;

        if (!last) {
            if (zframe_send(&payloadMsg, sender->zmq.socket, ZFRAME_MORE) == -1) ret = false;
        } else {
            if (zframe_send(&payloadMsg, sender->zmq.socket, 0) == -1) ret = false;
        }
    }
    celixThreadMutex_unlock(&sender->zmq.mutex);

    if (!ret){
        zframe_destroy(&headerMsg);
        zframe_destroy(&payloadMsg);
    }

    free(msg->header);
    free(msg->payload);
    free(msg);

    return ret;

}

static bool psa_zmq_sendMsgParts(pubsub_zmq_topic_sender_t *sender, celix_array_list_t *parts){
    bool ret = true;
    unsigned int i = 0;
    unsigned int mp_num = (unsigned int)celix_arrayList_size(parts);
    for (; i<mp_num; i++) {
        ret = ret && psa_zmq_sendMsg(sender, (pubsub_msg_t*)celix_arrayList_get(parts,i), (i==mp_num-1));
    }
    celix_arrayList_clear(parts);
    return ret;
}

static int psa_zmq_topicPublicationSendMultipart(void *handle, unsigned int msgTypeId, const void *inMsg, int flags) {
    int status = 0;
    psa_zmq_bounded_service_entry_t *bound = handle;
    pubsub_zmq_topic_sender_t *sender = bound->parent;

    celixThreadMutex_lock(&bound->multipart.mutex);
    if( (flags & PUBSUB_PUBLISHER_FIRST_MSG) && !(flags & PUBSUB_PUBLISHER_LAST_MSG) && bound->multipart.inProgress){ //means a real mp_msg
        L_INFO("PSA_ZMQ_TP: Multipart send already in progress. Cannot process a new one.\n");
        celixThreadMutex_unlock(&bound->multipart.mutex);
        return -3;
    }

    pubsub_msg_serializer_t* msgSer = hashMap_get(bound->msgTypes, (void*)(uintptr_t)msgTypeId);

    if (msgSer!= NULL) {
        int major=0, minor=0;

        pubsub_msg_header_t *msg_hdr = calloc(1,sizeof(struct pubsub_msg_header));
        strncpy( msg_hdr->topic, bound->parent->topic, MAX_TOPIC_LEN-1);
        msg_hdr->type = msgTypeId;

        if (msgSer->msgVersion != NULL){
            version_getMajor(msgSer->msgVersion, &major);
            version_getMinor(msgSer->msgVersion, &minor);
            msg_hdr->major = (unsigned char)major;
            msg_hdr->minor = (unsigned char)minor;
        }

        void *serializedOutput = NULL;
        size_t serializedOutputLen = 0;
        status = msgSer->serialize(msgSer,inMsg,&serializedOutput, &serializedOutputLen);
        if(status == CELIX_SUCCESS) {
            pubsub_msg_t *msg = calloc(1, sizeof(struct pubsub_msg));
            msg->header = msg_hdr;
            msg->payload = (char *) serializedOutput;
            msg->payloadSize = (int) serializedOutputLen;
            bool snd = true;

            switch (flags) {
                case PUBSUB_PUBLISHER_FIRST_MSG:
                    bound->multipart.inProgress = true;
                    celix_arrayList_add(bound->multipart.parts, msg);
                    break;
                case PUBSUB_PUBLISHER_PART_MSG:
                    if (!bound->multipart.inProgress) {
                        L_INFO("PSA_ZMQ_TP: ERROR: received msg part without the first part.\n");
                        status = -4;
                    } else {
                        celix_arrayList_add(bound->multipart.parts, msg);
                    }
                    break;
                case PUBSUB_PUBLISHER_LAST_MSG:
                    if (!bound->multipart.inProgress) {
                        L_INFO("PSA_ZMQ_TP: ERROR: received end msg without the first part.\n");
                        status = -4;
                    } else {
                        celix_arrayList_add(bound->multipart.parts, msg);
                        snd = psa_zmq_sendMsgParts(bound->parent, bound->multipart.parts);
                        bound->multipart.inProgress = false;
                        assert(celix_arrayList_size(bound->multipart.parts) == 0); //should be cleanup by sendMsg
                    }
                    break;
                case PUBSUB_PUBLISHER_FIRST_MSG | PUBSUB_PUBLISHER_LAST_MSG:    //Normal send case
                    snd = psa_zmq_sendMsg(bound->parent, msg, true);
                    break;
                default:
                    L_INFO("PSA_ZMQ_TP: ERROR: Invalid MP flags combination\n");
                    status = -4;
                    break;
            }

            if (status == -4) {
                free(msg);
            }

            if (!snd) {
                L_WARN("[PSA_ZMQ] Failed to send %s message %u.\n",
                       flags == (PUBSUB_PUBLISHER_FIRST_MSG | PUBSUB_PUBLISHER_LAST_MSG) ? "single" : "multipart",
                       msgTypeId);
                status = -1;
            }
        } else {
            L_WARN("[PSA_ZMQ] Failed to serialize %s message %u.\n",
                   flags == (PUBSUB_PUBLISHER_FIRST_MSG | PUBSUB_PUBLISHER_LAST_MSG) ? "single" : "multipart",
                   msgTypeId);
            status = -1;
        }

    } else {
        L_ERROR("[PSA_ZMQ] No msg serializer available for msg type id %d\n", msgTypeId);
        status=-1;
    }

    celixThreadMutex_unlock(&(bound->multipart.mutex));

    return status;

}

static void delay_first_send_for_late_joiners(pubsub_zmq_topic_sender_t *sender) {

    static bool firstSend = true;

    if(firstSend){
        L_INFO("PSA_UDP_MC_TP: Delaying first send for late joiners...\n");
        sleep(FIRST_SEND_DELAY_IN_SECONDS);
        firstSend = false;
    }
}

static unsigned int rand_range(unsigned int min, unsigned int max){
    double scaled = ((double)random())/((double)RAND_MAX);
    return (unsigned int)((max-min+1)*scaled + min);
}
