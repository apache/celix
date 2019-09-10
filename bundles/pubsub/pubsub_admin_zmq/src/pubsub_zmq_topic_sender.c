/**
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */

#include <pubsub_serializer.h>
#include <stdlib.h>
#include <memory.h>
#include <pubsub_constants.h>
#include <pubsub/publisher.h>
#include <utils.h>
#include <zconf.h>
#include <arpa/inet.h>
#include <czmq.h>
#include <log_helper.h>
#include "pubsub_zmq_topic_sender.h"
#include "pubsub_psa_zmq_constants.h"
#include "pubsub_zmq_common.h"
#include <uuid/uuid.h>
#include <constants.h>

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
    uuid_t fwUUID;
    bool metricsEnabled;
    bool zeroCopyEnabled;

    char *scope;
    char *topic;
    char scopeAndTopicFilter[5];
    char *url;
    bool isStatic;

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

typedef struct psa_zmq_send_msg_entry {
    pubsub_zmq_msg_header_t header; //partially filled header (only seqnr and time needs to be updated per send)
    pubsub_msg_serializer_t *msgSer;
    celix_thread_mutex_t sendLock; //protects send & Seqnr
    unsigned int seqNr;
    struct {
        celix_thread_mutex_t mutex; //protects entries in struct
        unsigned long nrOfMessagesSend;
        unsigned long nrOfMessagesSendFailed;
        unsigned long nrOfSerializationErrors;
        struct timespec lastMessageSend;
        double averageTimeBetweenMessagesInSeconds;
        double averageSerializationTimeInSeconds;
    } metrics;
} psa_zmq_send_msg_entry_t;

typedef struct psa_zmq_bounded_service_entry {
    pubsub_zmq_topic_sender_t *parent;
    pubsub_publisher_t service;
    long bndId;
    hash_map_t *msgTypes; //key = msg type id, value = pubsub_msg_serializer_t
    hash_map_t *msgTypeIds; //key = msg name, value = msg type id
    hash_map_t *msgEntries; //key = msg type id, value = psa_zmq_send_msg_entry_t
    int getCount;
} psa_zmq_bounded_service_entry_t;

static void* psa_zmq_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static void psa_zmq_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static unsigned int rand_range(unsigned int min, unsigned int max);
static void delay_first_send_for_late_joiners(pubsub_zmq_topic_sender_t *sender);

static int psa_zmq_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *msg);

pubsub_zmq_topic_sender_t* pubsub_zmqTopicSender_create(
        celix_bundle_context_t *ctx,
        log_helper_t *logHelper,
        const char *scope,
        const char *topic,
        long serializerSvcId,
        pubsub_serializer_service_t *ser,
        const char *bindIP,
        const char *staticBindUrl,
        unsigned int basePort,
        unsigned int maxPort) {
    pubsub_zmq_topic_sender_t *sender = calloc(1, sizeof(*sender));
    sender->ctx = ctx;
    sender->logHelper = logHelper;
    sender->serializerSvcId = serializerSvcId;
    sender->serializer = ser;
    psa_zmq_setScopeAndTopicFilter(scope, topic, sender->scopeAndTopicFilter);
    const char* uuid = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
    if (uuid != NULL) {
        uuid_parse(uuid, sender->fwUUID);
    }
    sender->metricsEnabled = celix_bundleContext_getPropertyAsBool(ctx, PSA_ZMQ_METRICS_ENABLED, PSA_ZMQ_DEFAULT_METRICS_ENABLED);
    sender->zeroCopyEnabled = celix_bundleContext_getPropertyAsBool(ctx, PSA_ZMQ_ZEROCOPY_ENABLED, PSA_ZMQ_DEFAULT_ZEROCOPY_ENABLED);

    //setting up zmq socket for ZMQ TopicSender
    {
#ifdef BUILD_WITH_ZMQ_SECURITY
        char *secure_topics = NULL;
        bundleContext_getProperty(bundle_context, "SECURE_TOPICS", (const char **) &secure_topics);

        if (secure_topics) {
            array_list_pt secure_topics_list = pubsub_getTopicsFromString(secure_topics);

            int i;
            int secure_topics_size = arrayList_size(secure_topics_list);
            for (i = 0; i < secure_topics_size; i++) {
                char* top = arrayList_get(secure_topics_list, i);
                if (strcmp(pubEP->topic, top) == 0) {
                    printf("PSA_ZMQ_TP: Secure topic: '%s'\n", top);
                    pubEP->is_secure = true;
                }
                free(top);
                top = NULL;
            }

            arrayList_destroy(secure_topics_list);
        }

        zcert_t* pub_cert = NULL;
        if (pubEP->is_secure) {
            char* keys_bundle_dir = pubsub_getKeysBundleDir(bundle_context);
            if (keys_bundle_dir == NULL) {
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
            if (pub_cert == NULL) {
                printf("PSA_ZMQ_TP: Cannot load key '%s'\n", cert_path);
                printf("PSA_ZMQ_TP: Topic '%s' NOT SECURED !\n", pubEP->topic);
                pubEP->is_secure = false;
            }
        }
#endif

        zsock_t* zmqSocket = zsock_new(ZMQ_PUB);
        if (zmqSocket==NULL) {
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

        if (zmqSocket != NULL && staticBindUrl != NULL) {
            int rv = zsock_bind (zmqSocket, "%s", staticBindUrl);
            if (rv == -1) {
                L_WARN("Error for zmq_bind using static bind url '%s'. %s", staticBindUrl, strerror(errno));
            } else {
                sender->url = strndup(staticBindUrl, 1024*1024);
                sender->isStatic = true;
            }
        } else if (zmqSocket != NULL) {

            int retry = 0;
            while (sender->url == NULL && retry < ZMQ_BIND_MAX_RETRY) {
                /* Randomized part due to same bundle publishing on different topics */
                unsigned int port = rand_range(basePort, maxPort);

                char *url = NULL;
                asprintf(&url, "tcp://%s:%u", bindIP, port);

                char *bindUrl = NULL;
                asprintf(&bindUrl, "tcp://0.0.0.0:%u", port);


                int rv = zsock_bind(zmqSocket, "%s", bindUrl);
                if (rv == -1) {
                    L_WARN("Error for zmq_bind using dynamic bind url '%s'. %s", bindUrl, strerror(errno));
                    free(url);
                } else {
                    sender->url = url;
                }
                retry++;
                free(bindUrl);
            }
        }

        if (sender->url == NULL)  {
            zsock_destroy(&zmqSocket);
        } else {
            sender->zmq.socket = zmqSocket;
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

                hash_map_iterator_t iter2 = hashMapIterator_construct(entry->msgEntries);
                while (hashMapIterator_hasNext(&iter2)) {
                    psa_zmq_send_msg_entry_t *msgEntry = hashMapIterator_nextValue(&iter2);
                    celixThreadMutex_destroy(&msgEntry->metrics.mutex);
                    free(msgEntry);

                }
                hashMap_destroy(entry->msgEntries, false, false);

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

bool pubsub_zmqTopicSender_isStatic(pubsub_zmq_topic_sender_t *sender) {
    return sender->isStatic;
}

void pubsub_zmqTopicSender_connectTo(pubsub_zmq_topic_sender_t *sender  __attribute__((unused)), const celix_properties_t *endpoint __attribute__((unused))) {
    /*nop*/
}

void pubsub_zmqTopicSender_disconnectFrom(pubsub_zmq_topic_sender_t *sender __attribute__((unused)), const celix_properties_t *endpoint __attribute__((unused))) {
    /*nop*/
}

static int psa_zmq_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId) {
    psa_zmq_bounded_service_entry_t *entry = (psa_zmq_bounded_service_entry_t *) handle;
    *msgTypeId = (unsigned int)(uintptr_t) hashMap_get(entry->msgTypeIds, msgType);
    return 0;
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
        entry->msgEntries = hashMap_create(NULL, NULL, NULL, NULL);
        entry->msgTypeIds = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);

        int rc = sender->serializer->createSerializerMap(sender->serializer->handle, (celix_bundle_t*)requestingBundle, &entry->msgTypes);
        if (rc == 0) {
            hash_map_iterator_t iter = hashMapIterator_construct(entry->msgTypes);
            while (hashMapIterator_hasNext(&iter)) {
                hash_map_entry_t *hashMapEntry = hashMapIterator_nextEntry(&iter);
                void *key = hashMapEntry_getKey(hashMapEntry);
                psa_zmq_send_msg_entry_t *sendEntry = calloc(1, sizeof(*sendEntry));
                sendEntry->msgSer = hashMapEntry_getValue(hashMapEntry);
                sendEntry->header.type = (int32_t)sendEntry->msgSer->msgId;
                int major;
                int minor;
                version_getMajor(sendEntry->msgSer->msgVersion, &major);
                version_getMinor(sendEntry->msgSer->msgVersion, &minor);
                sendEntry->header.major = (uint8_t)major;
                sendEntry->header.minor = (uint8_t)minor;
                uuid_copy(sendEntry->header.originUUID, sender->fwUUID);
                celixThreadMutex_create(&sendEntry->metrics.mutex, NULL);
                hashMap_put(entry->msgEntries, key, sendEntry);
                hashMap_put(entry->msgTypeIds, strndup(sendEntry->msgSer->msgName, 1024), (void *)(uintptr_t) sendEntry->msgSer->msgId);
            }
            entry->service.handle = entry;
            entry->service.localMsgTypeIdForMsgType = psa_zmq_localMsgTypeIdForMsgType;
            entry->service.send = psa_zmq_topicPublicationSend;
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

        hash_map_iterator_t iter = hashMapIterator_construct(entry->msgEntries);
        while (hashMapIterator_hasNext(&iter)) {
            psa_zmq_send_msg_entry_t *msgEntry = hashMapIterator_nextValue(&iter);
            celixThreadMutex_destroy(&msgEntry->metrics.mutex);
            free(msgEntry);
        }
        hashMap_destroy(entry->msgEntries, false, false);

        hashMap_destroy(entry->msgTypeIds, true, false);
        free(entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);
}

pubsub_admin_sender_metrics_t* pubsub_zmqTopicSender_metrics(pubsub_zmq_topic_sender_t *sender) {
    pubsub_admin_sender_metrics_t *result = calloc(1, sizeof(*result));
    snprintf(result->scope, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", sender->scope);
    snprintf(result->topic, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", sender->topic);
    celixThreadMutex_lock(&sender->boundedServices.mutex);
    size_t count = 0;
    hash_map_iterator_t iter = hashMapIterator_construct(sender->boundedServices.map);
    while (hashMapIterator_hasNext(&iter)) {
        psa_zmq_bounded_service_entry_t *entry = hashMapIterator_nextValue(&iter);
        hash_map_iterator_t iter2 = hashMapIterator_construct(entry->msgEntries);
        while (hashMapIterator_hasNext(&iter2)) {
            hashMapIterator_nextValue(&iter2);
            count += 1;
        }
    }

    result->msgMetrics = calloc(count, sizeof(*result));

    iter = hashMapIterator_construct(sender->boundedServices.map);
    int i = 0;
    while (hashMapIterator_hasNext(&iter)) {
        psa_zmq_bounded_service_entry_t *entry = hashMapIterator_nextValue(&iter);
        hash_map_iterator_t iter2 = hashMapIterator_construct(entry->msgEntries);
        while (hashMapIterator_hasNext(&iter2)) {
            psa_zmq_send_msg_entry_t *mEntry = hashMapIterator_nextValue(&iter2);
            celixThreadMutex_lock(&mEntry->metrics.mutex);
            result->msgMetrics[i].nrOfMessagesSend = mEntry->metrics.nrOfMessagesSend;
            result->msgMetrics[i].nrOfMessagesSendFailed = mEntry->metrics.nrOfMessagesSendFailed;
            result->msgMetrics[i].nrOfSerializationErrors = mEntry->metrics.nrOfSerializationErrors;
            result->msgMetrics[i].averageSerializationTimeInSeconds = mEntry->metrics.averageSerializationTimeInSeconds;
            result->msgMetrics[i].averageTimeBetweenMessagesInSeconds = mEntry->metrics.averageTimeBetweenMessagesInSeconds;
            result->msgMetrics[i].lastMessageSend = mEntry->metrics.lastMessageSend;
            result->msgMetrics[i].bndId = entry->bndId;
            result->msgMetrics[i].typeId = mEntry->header.type;
            snprintf(result->msgMetrics[i].typeFqn, PUBSUB_AMDIN_METRICS_NAME_MAX, "%s", mEntry->msgSer->msgName);
            i += 1;
            celixThreadMutex_unlock(&mEntry->metrics.mutex);
        }
    }

    celixThreadMutex_unlock(&sender->boundedServices.mutex);
    result->nrOfmsgMetrics = (int)count;
    return result;
}

static void psa_zmq_freeMsg(void *msg, void *hint __attribute__((unused))) {
    free(msg);
}

static int psa_zmq_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *inMsg) {
    int status = CELIX_SUCCESS;
    psa_zmq_bounded_service_entry_t *bound = handle;
    pubsub_zmq_topic_sender_t *sender = bound->parent;
    bool monitor = sender->metricsEnabled;

    psa_zmq_send_msg_entry_t *entry = hashMap_get(bound->msgEntries, (void*)(uintptr_t)(msgTypeId));

    //metrics updates
    struct timespec sendTime;
    struct timespec serializationStart;
    struct timespec serializationEnd;
    //int unknownMessageCountUpdate = 0;
    int sendErrorUpdate = 0;
    int serializationErrorUpdate = 0;
    int sendCountUpdate = 0;

    if (entry != NULL) {
        delay_first_send_for_late_joiners(sender);



        if (monitor) {
            clock_gettime(CLOCK_REALTIME, &serializationStart);
        }

        void *serializedOutput = NULL;
        size_t serializedOutputLen = 0;
        status = entry->msgSer->serialize(entry->msgSer->handle, inMsg, &serializedOutput, &serializedOutputLen);

        if (monitor) {
            clock_gettime(CLOCK_REALTIME, &serializationEnd);
        }

        if (status == CELIX_SUCCESS /*ser ok*/) {
            unsigned char *hdr = calloc(sizeof(pubsub_zmq_msg_header_t), sizeof(unsigned char));

            celixThreadMutex_lock(&entry->sendLock);

            pubsub_zmq_msg_header_t msg_hdr = entry->header;
            msg_hdr.seqNr = 0;
            msg_hdr.sendtimeSeconds = 0;
            msg_hdr.sendTimeNanoseconds = 0;
            if (monitor) {
                clock_gettime(CLOCK_REALTIME, &sendTime);
                msg_hdr.sendtimeSeconds = (uint64_t) sendTime.tv_sec;
                msg_hdr.sendTimeNanoseconds = (uint64_t) sendTime.tv_nsec;
                msg_hdr.seqNr = entry->seqNr++;
            }
            psa_zmq_encodeHeader(&msg_hdr, hdr, sizeof(pubsub_zmq_msg_header_t));

            errno = 0;
            bool sendOk;

            if (bound->parent->zeroCopyEnabled) {
                zmq_msg_t msg1; //filter
                zmq_msg_t msg2; //header
                zmq_msg_t msg3; //payload
                void *socket = zsock_resolve(sender->zmq.socket);

                zmq_msg_init_data(&msg1, sender->scopeAndTopicFilter, 4, NULL, bound);
                //send filter
                int rc = zmq_msg_send(&msg1, socket, ZMQ_SNDMORE);
                if (rc == -1) {
                    L_WARN("Error sending filter msg. %s", strerror(errno));
                    zmq_msg_close(&msg1);
                }

                //send header
                if (rc > 0) {
                    zmq_msg_init_data(&msg2, hdr, sizeof(pubsub_zmq_msg_header_t), psa_zmq_freeMsg, bound);
                    rc = zmq_msg_send(&msg2, socket, ZMQ_SNDMORE);
                    if (rc == -1) {
                        L_WARN("Error sending header msg. %s", strerror(errno));
                        zmq_msg_close(&msg2);
                    }
                }


                if (rc > 0) {
                    zmq_msg_init_data(&msg3, serializedOutput, serializedOutputLen, psa_zmq_freeMsg, bound);
                    rc = zmq_msg_send(&msg3, socket, 0);
                    if (rc == -1) {
                        L_WARN("Error sending payload msg. %s", strerror(errno));
                        zmq_msg_close(&msg3);
                    }
                }

                sendOk = rc > 0;
            } else {
                zmsg_t *msg = zmsg_new();
                zmsg_addstr(msg, sender->scopeAndTopicFilter);
                zmsg_addmem(msg, hdr, sizeof(pubsub_zmq_msg_header_t));
                zmsg_addmem(msg, serializedOutput, serializedOutputLen);
                int rc = zmsg_send(&msg, sender->zmq.socket);
                sendOk = rc == 0;
                free(serializedOutput);
                free(hdr);
                if (!sendOk) {
                    zmsg_destroy(&msg); //if send was not ok, no owner change -> destroy msg
                }
            }

            celixThreadMutex_unlock(&entry->sendLock);
            if (sendOk) {
                sendCountUpdate = 1;
            } else {
                sendErrorUpdate = 1;
                L_WARN("[PSA_ZMQ_TS] Error sending zmg. %s", strerror(errno));
            }
        } else {
            serializationErrorUpdate = 1;
            L_WARN("[PSA_ZMQ_TS] Error serialize message of type %s for scope/topic %s/%s", entry->msgSer->msgName, sender->scope, sender->topic);
        }
    } else {
        //unknownMessageCountUpdate = 1;
        status = CELIX_SERVICE_EXCEPTION;
        L_WARN("[PSA_ZMQ_TS] Error cannot serialize message with msg type id %i for scope/topic %s/%s", msgTypeId, sender->scope, sender->topic);
    }


    if (monitor && entry != NULL) {
        celixThreadMutex_lock(&entry->metrics.mutex);

        long n = entry->metrics.nrOfMessagesSend + entry->metrics.nrOfMessagesSendFailed;
        double diff = celix_difftime(&serializationStart, &serializationEnd);
        double average = (entry->metrics.averageSerializationTimeInSeconds * n + diff) / (n+1);
        entry->metrics.averageSerializationTimeInSeconds = average;

        if (entry->metrics.nrOfMessagesSend > 2) {
            diff = celix_difftime(&entry->metrics.lastMessageSend, &sendTime);
            n = entry->metrics.nrOfMessagesSend;
            average = (entry->metrics.averageTimeBetweenMessagesInSeconds * n + diff) / (n+1);
            entry->metrics.averageTimeBetweenMessagesInSeconds = average;
        }

        entry->metrics.lastMessageSend = sendTime;
        entry->metrics.nrOfMessagesSend += sendCountUpdate;
        entry->metrics.nrOfMessagesSendFailed += sendErrorUpdate;
        entry->metrics.nrOfSerializationErrors += serializationErrorUpdate;

        celixThreadMutex_unlock(&entry->metrics.mutex);
    }

    return status;
}

static void delay_first_send_for_late_joiners(pubsub_zmq_topic_sender_t *sender) {

    static bool firstSend = true;

    if (firstSend) {
        L_INFO("PSA_ZMQ_TP: Delaying first send for late joiners...\n");
        sleep(FIRST_SEND_DELAY_IN_SECONDS);
        firstSend = false;
    }
}

static unsigned int rand_range(unsigned int min, unsigned int max) {
    double scaled = ((double)random())/((double)RAND_MAX);
    return (unsigned int)((max-min+1)*scaled + min);
}
