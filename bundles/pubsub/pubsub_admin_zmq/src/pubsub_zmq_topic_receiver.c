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
#include <pubsub/subscriber.h>
#include <memory.h>
#include <pubsub_constants.h>
#include <sys/epoll.h>
#include <assert.h>
#include <pubsub_endpoint.h>
#include <arpa/inet.h>
#include <czmq.h>
#include <log_helper.h>
#include "pubsub_zmq_topic_receiver.h"
#include "pubsub_psa_zmq_constants.h"
#include "pubsub_zmq_common.h"

//TODO see if block and wakeup (reset) also works
#define PSA_ZMQ_RECV_TIMEOUT 1000


#define L_DEBUG(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(receiver->logHelper, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

struct pubsub_zmq_topic_receiver {
    celix_bundle_context_t *ctx;
    log_helper_t *logHelper;
    long serializerSvcId;
    pubsub_serializer_service_t *serializer;
    char *scope;
    char *topic;

    zsock_t *zmqSocket;

    struct {
        celix_thread_t thread;
        celix_thread_mutex_t mutex;
        bool running;
    } recvThread;

    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = zmq url, value = psa_zmq_requested_connection_entry_t*
    } requestedConnections;

    long subscriberTrackerId;
    struct {
        celix_thread_mutex_t mutex;
        hash_map_t *map; //key = bnd id, value = psa_zmq_subscriber_entry_t
    } subscribers;
};

typedef struct psa_zmq_requested_connection_entry {
    char *url;
    bool connected;
} psa_zmq_requested_connection_entry_t;

typedef struct psa_zmq_subscriber_entry {
    int usageCount;
    hash_map_t *msgTypes; //map from serializer svc
    pubsub_subscriber_t *svc;
} psa_zmq_subscriber_entry_t;

typedef struct complete_zmq_msg {
    zframe_t* header;
    zframe_t* payload;
} complete_zmq_msg_t;

typedef struct mp_handle {
    hash_map_pt svc_msg_db;
    hash_map_pt rcv_msg_map;
} mp_handle_t;

typedef struct msg_map_entry {
    bool retain;
    void* msgInst;
} msg_map_entry_t;


static void pubsub_zmqTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void pubsub_zmqTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *owner);
static void psa_zmq_processMsg(pubsub_zmq_topic_receiver_t *receiver, celix_array_list_t *messages);
static void* psa_zmq_recvThread(void * data);


pubsub_zmq_topic_receiver_t* pubsub_zmqTopicReceiver_create(celix_bundle_context_t *ctx,
                                                              log_helper_t *logHelper,
                                                              const char *scope,
                                                                const char *topic,
                                                                long serializerSvcId,
                                                                pubsub_serializer_service_t *serializer) {
    pubsub_zmq_topic_receiver_t *receiver = calloc(1, sizeof(*receiver));
    receiver->ctx = ctx;
    receiver->logHelper = logHelper;
    receiver->serializerSvcId = serializerSvcId;
    receiver->serializer = serializer;


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
    receiver->zmqSocket = zsock_new (ZMQ_SUB);
    if (receiver->zmqSocket == NULL) {
#ifdef BUILD_WITH_ZMQ_SECURITY
        zcert_destroy(&sub_cert);
        zcert_destroy(&pub_cert);
#endif
    }

    if (receiver->zmqSocket != NULL) {
#ifdef BUILD_WITH_ZMQ_SECURITY
        zcert_apply (sub_cert, zmq_s);
        zsock_set_curve_serverkey (zmq_s, pub_key); //apply key of publisher to socket of subscriber
#endif
        zsock_set_subscribe(receiver->zmqSocket, topic);

#ifdef BUILD_WITH_ZMQ_SECURITY
        ts->zmq_cert = sub_cert;
	    ts->zmq_pub_cert = pub_cert;
#endif
    }

    if (receiver->zmqSocket != NULL) {
        receiver->scope = strndup(scope, 1024 * 1024);
        receiver->topic = strndup(topic, 1024 * 1024);

        celixThreadMutex_create(&receiver->subscribers.mutex, NULL);
        celixThreadMutex_create(&receiver->requestedConnections.mutex, NULL);
        celixThreadMutex_create(&receiver->recvThread.mutex, NULL);

        receiver->subscribers.map = hashMap_create(NULL, NULL, NULL, NULL);
        receiver->requestedConnections.map = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    }

    //track subscribers
    if (receiver->zmqSocket != NULL ) {
        int size = snprintf(NULL, 0, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        char buf[size+1];
        snprintf(buf, (size_t)size+1, "(%s=%s)", PUBSUB_SUBSCRIBER_TOPIC, topic);
        celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
        opts.filter.ignoreServiceLanguage = true;
        opts.filter.serviceName = PUBSUB_SUBSCRIBER_SERVICE_NAME;
        opts.filter.filter = buf;
        opts.callbackHandle = receiver;
        opts.addWithOwner = pubsub_zmqTopicReceiver_addSubscriber;
        opts.removeWithOwner = pubsub_zmqTopicReceiver_removeSubscriber;

        receiver->subscriberTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    }

    if (receiver->zmqSocket != NULL ) {
        receiver->recvThread.running = true;
        celixThread_create(&receiver->recvThread.thread, NULL, psa_zmq_recvThread, receiver);
        char name[64];
        snprintf(name, 64, "ZMQ TR %s/%s", scope, topic);
        celixThread_setName(&receiver->recvThread.thread, name);
    }

    if (receiver->zmqSocket == NULL) {
        free(receiver);
        receiver = NULL;
        L_ERROR("[PSA_ZMQ] Cannot create TopicReceiver for %s/%s", scope, topic);
    }

    return receiver;
}

void pubsub_zmqTopicReceiver_destroy(pubsub_zmq_topic_receiver_t *receiver) {
    if (receiver != NULL) {

        celixThreadMutex_lock(&receiver->recvThread.mutex);
        receiver->recvThread.running = false;
        celixThreadMutex_unlock(&receiver->recvThread.mutex);
        celixThread_join(receiver->recvThread.thread, NULL);

        celix_bundleContext_stopTracker(receiver->ctx, receiver->subscriberTrackerId);

        celixThreadMutex_lock(&receiver->subscribers.mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_zmq_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL)  {
                receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
                free(entry);
            }
        }
        hashMap_destroy(receiver->subscribers.map, false, false);
        celixThreadMutex_unlock(&receiver->subscribers.mutex);

        celixThreadMutex_lock(&receiver->requestedConnections.mutex);
        iter = hashMapIterator_construct(receiver->requestedConnections.map);
        while (hashMapIterator_hasNext(&iter)) {
            psa_zmq_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
            if (entry != NULL) {
                free(entry->url);
                free(entry);
            }
        }
        hashMap_destroy(receiver->requestedConnections.map, false, false);
        celixThreadMutex_unlock(&receiver->requestedConnections.mutex);

        celixThreadMutex_destroy(&receiver->subscribers.mutex);
        celixThreadMutex_destroy(&receiver->requestedConnections.mutex);
        celixThreadMutex_destroy(&receiver->recvThread.mutex);

        zsock_destroy(&receiver->zmqSocket);

        free(receiver->scope);
        free(receiver->topic);
    }
    free(receiver);
}

const char* pubsub_zmqTopicReceiver_scope(pubsub_zmq_topic_receiver_t *receiver) {
    return receiver->scope;
}
const char* pubsub_zmqTopicReceiver_topic(pubsub_zmq_topic_receiver_t *receiver) {
    return receiver->topic;
}

long pubsub_zmqTopicReceiver_serializerSvcId(pubsub_zmq_topic_receiver_t *receiver) {
    return receiver->serializerSvcId;
}

void pubsub_zmqTopicReceiver_listConnections(pubsub_zmq_topic_receiver_t *receiver, celix_array_list_t *connectedUrls, celix_array_list_t *unconnectedUrls) {
    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->requestedConnections.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_zmq_requested_connection_entry_t *entry = hashMapIterator_nextValue(&iter);
        if (entry->connected) {
            celix_arrayList_add(connectedUrls, strndup(entry->url, 1024));
        } else {
            celix_arrayList_add(unconnectedUrls, strndup(entry->url, 1024));
        }
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}


void pubsub_zmqTopicReceiver_connectTo(
        pubsub_zmq_topic_receiver_t *receiver,
        const char *url) {
    L_DEBUG("[PSA_ZMQ] TopicReceiver %s/%s connecting to zmq url %s", receiver->scope, receiver->topic, url);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_zmq_requested_connection_entry_t *entry = hashMap_get(receiver->requestedConnections.map, url);
    if (entry == NULL) {
        entry = calloc(1, sizeof(*entry));
        entry->url = strndup(url, 1024*1024);
        entry->connected = false;
        hashMap_put(receiver->requestedConnections.map, (void*)entry->url, entry);
    }
    if (!entry->connected) {
        if (zsock_connect(receiver->zmqSocket, "%s", url) == 0) {
            entry->connected = true;
        } else {
            L_WARN("[PSA_ZMQ] Error connecting to zmq url %s. (%s)", url, strerror(errno));
        }
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

void pubsub_zmqTopicReceiver_disconnectFrom(pubsub_zmq_topic_receiver_t *receiver, const char *url) {
    L_DEBUG("[PSA ZMQ] TopicReceiver %s/%s disconnect from zmq url %s", receiver->scope, receiver->topic, url);

    celixThreadMutex_lock(&receiver->requestedConnections.mutex);
    psa_zmq_requested_connection_entry_t *entry = hashMap_remove(receiver->requestedConnections.map, url);
    if (entry != NULL && entry->connected) {
        if (zsock_disconnect(receiver->zmqSocket,"%s",url) == 0) {
            entry->connected = false;
        } else {
            L_WARN("[PSA_ZMQ] Error disconnecting from zmq url %s. (%s)", url, strerror(errno));
        }
    }
    if (entry != NULL) {
        free(entry->url);
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->requestedConnections.mutex);
}

static void pubsub_zmqTopicReceiver_addSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_zmq_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);
    const char *subScope = celix_properties_get(props, PUBSUB_SUBSCRIBER_SCOPE, "default");
    if (strncmp(subScope, receiver->scope, strlen(receiver->scope)) != 0) {
        //not the same scope. ignore
        return;
    }

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_zmq_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
    if (entry != NULL) {
        entry->usageCount += 1;
    } else {
        //new create entry
        entry = calloc(1, sizeof(*entry));
        entry->usageCount = 1;
        entry->svc = svc;

        int rc = receiver->serializer->createSerializerMap(receiver->serializer->handle, (celix_bundle_t*)bnd, &entry->msgTypes);
        if (rc == 0) {
            hashMap_put(receiver->subscribers.map, (void*)bndId, entry);
        } else {
            L_ERROR("[PSA_ZMQ] Cannot create msg serializer map for TopicReceiver %s/%s", receiver->scope, receiver->topic);
            free(entry);
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void pubsub_zmqTopicReceiver_removeSubscriber(void *handle, void *svc, const celix_properties_t *props, const celix_bundle_t *bnd) {
    pubsub_zmq_topic_receiver_t *receiver = handle;

    long bndId = celix_bundle_getId(bnd);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    psa_zmq_subscriber_entry_t *entry = hashMap_get(receiver->subscribers.map, (void*)bndId);
    if (entry != NULL) {
        entry->usageCount -= 1;
    }
    if (entry != NULL && entry->usageCount <= 0) {
        //remove entry
        hashMap_remove(receiver->subscribers.map, (void*)bndId);
        int rc = receiver->serializer->destroySerializerMap(receiver->serializer->handle, entry->msgTypes);
        if (rc != 0) {
            L_ERROR("[PSA_ZMQ] Cannot destroy msg serializers map for TopicReceiver %s/%s", receiver->scope, receiver->topic);
        }
        free(entry);
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}

static void* psa_zmq_recvThread(void * data) {
    pubsub_zmq_topic_receiver_t *receiver = data;

    celixThreadMutex_lock(&receiver->recvThread.mutex);
    bool running = receiver->recvThread.running;
    celixThreadMutex_unlock(&receiver->recvThread.mutex);

    while (running) {
        zframe_t* headerMsg = zframe_recv_nowait(receiver->zmqSocket);
        if (headerMsg == NULL) {
            if(errno == EAGAIN) {
                usleep(PSA_ZMQ_RECV_TIMEOUT);
            } else if (errno == EINTR) {
                //It means we got a signal and we have to exit...
                L_INFO("PSA_ZMQ_TS: header_recv thread for topic got a signal and will exit.\n");
            } else {
                L_ERROR("PSA_ZMQ_TS: header_recv thread (%s)", strerror(errno));
            }
        } else {
            pubsub_msg_header_t *hdr = (pubsub_msg_header_t*)zframe_data(headerMsg);

            if (zframe_more(headerMsg)) {
                zframe_t* payloadMsg = zframe_recv(receiver->zmqSocket);
                if (payloadMsg == NULL) {
                    if (errno == EINTR) {
                        //It means we got a signal and we have to exit...
                        L_INFO("[PSA_ZMQ] TopicSender payload_recv thread for topic got a signal and will exit.\n");
                    } else {
                        L_WARN("[PSA_ZMQ] TopicSender: payload_recv (%s)", strerror(errno));
                    }
                    zframe_destroy(&headerMsg);
                } else {

                    //Let's fetch all the messages from the socket
                    celix_array_list_t *msg_list = celix_arrayList_create();
                    complete_zmq_msg_t *firstMsg = calloc(1, sizeof(struct complete_zmq_msg));
                    firstMsg->header = headerMsg;
                    firstMsg->payload = payloadMsg;
                    arrayList_add(msg_list, firstMsg);

                    bool more = (bool)zframe_more(payloadMsg);
                    while (more) {

                        zframe_t* h_msg = zframe_recv(receiver->zmqSocket);
                        if (h_msg == NULL) {
                            if (errno == EINTR) {
                                //It means we got a signal and we have to exit...
                                L_INFO("[PSA_ZMQ] TopicSender payload_recv thread for topic got a signal and will exit.\n");
                            } else {
                                L_WARN("[PSA_ZMQ] TopicSender: payload_recv (%s)", strerror(errno));
                            }
                            break;
                        }

                        zframe_t* p_msg = zframe_recv(receiver->zmqSocket);
                        if (p_msg == NULL) {
                            if (errno == EINTR) {
                                //It means we got a signal and we have to exit...
                                L_INFO("[PSA_ZMQ] TopicSender payload_recv thread for topic got a signal and will exit.\n");
                            } else {
                                L_WARN("[PSA_ZMQ] TopicSender: payload_recv (%s)", strerror(errno));
                            }
                            zframe_destroy(&h_msg);
                            break;
                        }

                        complete_zmq_msg_t *c_msg = calloc(1, sizeof(struct complete_zmq_msg));
                        c_msg->header = h_msg;
                        c_msg->payload = p_msg;
                        arrayList_add(msg_list, c_msg);

                        if (!zframe_more(p_msg)) {
                            more = false;
                        }
                    }

                    psa_zmq_processMsg(receiver, msg_list);
                    for (int i = 0; i < celix_arrayList_size(msg_list); ++i) {
                        complete_zmq_msg_t *msg = celix_arrayList_get(msg_list, i);
                        zframe_destroy(&msg->header);
                        zframe_destroy(&msg->payload);
                        free(msg);
                    }
                    celix_arrayList_destroy(msg_list);
                }

            } else {
                free(headerMsg);
                L_WARN("PSA_ZMQ_TS: received message %u for topic %s without payload!\n", hdr->type, hdr->topic);
            }

        }

        celixThreadMutex_lock(&receiver->recvThread.mutex);
        running = receiver->recvThread.running;
        celixThreadMutex_unlock(&receiver->recvThread.mutex);
    } // while

    return NULL;
}

static int psa_zmq_getMultipart(void *handle, unsigned int msgTypeId, bool retain, void **part){

    if(handle==NULL){
        *part = NULL;
        return -1;
    }

    mp_handle_t *mp_handle = handle;
    msg_map_entry_t *entry = hashMap_get(mp_handle->rcv_msg_map, (void*)(uintptr_t) msgTypeId);
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

static void psa_zmq_destroyMultipartHandle(mp_handle_t *mp_handle) {

    hash_map_iterator_pt iter = hashMapIterator_create(mp_handle->rcv_msg_map);
    while(hashMapIterator_hasNext(iter)){
        hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
        unsigned int msgId = (unsigned int)(uintptr_t)hashMapEntry_getKey(entry);
        msg_map_entry_t *msgEntry = hashMapEntry_getValue(entry);
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

static mp_handle_t* psa_zmq_createMultipartHandle(hash_map_pt svc_msg_db,array_list_pt rcv_msg_list){

    if(arrayList_size(rcv_msg_list)==1){ //Means it's not a multipart message
        return NULL;
    }

    mp_handle_t *mp_handle = calloc(1,sizeof(struct mp_handle));
    mp_handle->svc_msg_db = svc_msg_db;
    mp_handle->rcv_msg_map = hashMap_create(NULL, NULL, NULL, NULL);

    int i=1; //We skip the first message, it will be handle differently
    for(;i<arrayList_size(rcv_msg_list);i++){
        complete_zmq_msg_t *c_msg = celix_arrayList_get(rcv_msg_list,i);
        pubsub_msg_header_t *header = (pubsub_msg_header_t*)zframe_data(c_msg->header);

        pubsub_msg_serializer_t* msgSer = hashMap_get(svc_msg_db, (void*)(uintptr_t)(header->type));

        if (msgSer!= NULL) {
            void *msgInst = NULL;

            bool validVersion = psa_zmq_checkVersion(msgSer->msgVersion,header);

            if(validVersion){
                celix_status_t status = msgSer->deserialize(msgSer, (const void*)zframe_data(c_msg->payload), 0, &msgInst);

                if(status == CELIX_SUCCESS){
                    msg_map_entry_t *entry = calloc(1,sizeof(struct msg_map_entry));
                    entry->msgInst = msgInst;
                    hashMap_put(mp_handle->rcv_msg_map, (void*)(uintptr_t)header->type,entry);
                }
            }
        }
    }

    return mp_handle;

}


static void psa_zmq_processMsg(pubsub_zmq_topic_receiver_t *receiver, celix_array_list_t *messages) {

    pubsub_msg_header_t *first_msg_hdr = (pubsub_msg_header_t*)zframe_data(((complete_zmq_msg_t*)celix_arrayList_get(messages,0))->header);

    celixThreadMutex_lock(&receiver->subscribers.mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(receiver->subscribers.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_zmq_subscriber_entry_t *entry = hashMapIterator_nextValue(&iter);

        pubsub_msg_serializer_t *msgSer = NULL;
        if (entry->msgTypes != NULL) {
            msgSer = hashMap_get(entry->msgTypes, (void *) (uintptr_t) first_msg_hdr->type);
        }
        if (msgSer == NULL) {
            printf("[PSA_ZMQ] Serializer not available for message %d.\n", first_msg_hdr->type);
        } else {
            void *msgInst = NULL;
            bool validVersion = psa_zmq_checkVersion(msgSer->msgVersion,first_msg_hdr);

            if (validVersion) {

                celix_status_t status = msgSer->deserialize(msgSer, (const void *) zframe_data(((complete_zmq_msg_t*)celix_arrayList_get(messages,0))->payload), 0, &msgInst);
                if (status == CELIX_SUCCESS) {
                    bool release = true;
                    mp_handle_t *mp_handle = psa_zmq_createMultipartHandle(entry->msgTypes, messages);
                    pubsub_multipart_callbacks_t mp_callbacks;
                    mp_callbacks.handle = mp_handle;
                    mp_callbacks.localMsgTypeIdForMsgType = psa_zmq_localMsgTypeIdForMsgType;
                    mp_callbacks.getMultipart = psa_zmq_getMultipart;
                    entry->svc->receive(entry->svc->handle, msgSer->msgName, first_msg_hdr->type, msgInst, &mp_callbacks, &release);

                    if (release) {
                        msgSer->freeMsg(msgSer,msgInst); // pubsubSerializer_freeMsg(msgType, msgInst);
                    }
                    if (mp_handle!=NULL) {
                        psa_zmq_destroyMultipartHandle(mp_handle);
                    }
                }
                else{
                    L_WARN("[PSA_ZMQ] TopicReceiver Cannot deserialize msgType %s.\n",msgSer->msgName);
                }

            } else {
                int major=0,minor=0;
                version_getMajor(msgSer->msgVersion,&major);
                version_getMinor(msgSer->msgVersion,&minor);
                L_WARN("[PSA_ZMQ] TopicReceiver: Version mismatch for primary message '%s' (have %d.%d, received %u.%u). NOT sending any part of the whole message.\n",
                       msgSer->msgName,major,minor,first_msg_hdr->major,first_msg_hdr->minor);
            }
        }
    }
    celixThreadMutex_unlock(&receiver->subscribers.mutex);
}
