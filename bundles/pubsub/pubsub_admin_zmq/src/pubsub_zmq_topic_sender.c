/*
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
#include <pubsub_protocol.h>
#include <stdlib.h>
#include <memory.h>
#include <arpa/inet.h>
#include <czmq.h>
#include <uuid/uuid.h>

#include "celix_utils.h"
#include "pubsub_constants.h"
#include "pubsub/publisher.h"
#include "celix_log_helper.h"
#include "pubsub_zmq_topic_sender.h"
#include "pubsub_psa_zmq_constants.h"
#include "celix_version.h"
#include "pubsub_serializer_handler.h"
#include "celix_constants.h"
#include "pubsub_interceptors_handler.h"
#include "pubsub_zmq_admin.h"

#define FIRST_SEND_DELAY_IN_SECONDS             2
#define ZMQ_BIND_MAX_RETRY                      10

#define L_DEBUG(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_INFO, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_log(sender->logHelper, CELIX_LOG_LEVEL_ERROR, __VA_ARGS__)

struct pubsub_zmq_topic_sender {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    void *admin;
    long protocolSvcId;
    pubsub_protocol_service_t *protocol;
    uuid_t fwUUID;
    bool zeroCopyEnabled;

    pubsub_serializer_handler_t* serializerHandler;
    pubsub_interceptors_handler_t *interceptorsHandler;

    char *scope;
    char *topic;
    char *url;
    bool isStatic;

    long seqNr; //atomic

    struct {
        bool dataLock; //atomic, protects below and protect zmq internal data
        void *headerBuffer;
        size_t headerBufferSize;
        void *metadataBuffer;
        size_t metadataBufferSize;
        void *footerBuffer;
        size_t footerBufferSize;
    } zmqBuffers;

    struct {
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
    int getCount;
} psa_zmq_bounded_service_entry_t;

typedef struct psa_zmq_zerocopy_free_entry {
    uint32_t msgId;
    pubsub_serializer_handler_t *serHandler;
    struct iovec *serializedOutput;
    size_t serializedOutputLen;
} psa_zmq_zerocopy_free_entry;


static void* psa_zmq_getPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static void psa_zmq_ungetPublisherService(void *handle, const celix_bundle_t *requestingBundle, const celix_properties_t *svcProperties);
static unsigned int rand_range(unsigned int min, unsigned int max);
static void delay_first_send_for_late_joiners(pubsub_zmq_topic_sender_t *sender);

static int psa_zmq_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *msg, celix_properties_t *metadata);

pubsub_zmq_topic_sender_t* pubsub_zmqTopicSender_create(
        celix_bundle_context_t *ctx,
        celix_log_helper_t *logHelper,
        const char *scope,
        const char *topic,
        pubsub_serializer_handler_t* serializerHandler,
        void *admin,
        long protocolSvcId,
        pubsub_protocol_service_t *prot,
        const char *bindIP,
        const char *staticBindUrl,
        unsigned int basePort,
        unsigned int maxPort) {
    pubsub_zmq_topic_sender_t *sender = calloc(1, sizeof(*sender));
    sender->ctx = ctx;
    sender->logHelper = logHelper;
    sender->serializerHandler = serializerHandler;
    sender->admin = admin;
    sender->protocolSvcId = protocolSvcId;
    sender->protocol = prot;
    const char* uuid = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
    if (uuid != NULL) {
        uuid_parse(uuid, sender->fwUUID);
    }
    sender->zeroCopyEnabled = celix_bundleContext_getPropertyAsBool(ctx, PSA_ZMQ_ZEROCOPY_ENABLED, PSA_ZMQ_DEFAULT_ZEROCOPY_ENABLED);

    sender->interceptorsHandler = pubsubInterceptorsHandler_create(ctx, scope, topic, PUBSUB_ZMQ_ADMIN_TYPE,
                                                                   pubsub_serializerHandler_getSerializationType(serializerHandler));

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
                sender->url = celix_utils_strdup(staticBindUrl);
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
        sender->scope = scope == NULL ? NULL : celix_utils_strdup(scope);
        sender->topic = celix_utils_strdup(topic);

        celixThreadMutex_create(&sender->boundedServices.mutex, NULL);
        sender->boundedServices.map = hashMap_create(NULL, NULL, NULL, NULL);
    }

    //register publisher services using a service factory
    if (sender->url != NULL) {
        sender->publisher.factory.handle = sender;
        sender->publisher.factory.getService = psa_zmq_getPublisherService;
        sender->publisher.factory.ungetService = psa_zmq_ungetPublisherService;

        celix_properties_t *props = celix_properties_create();
        celix_properties_set(props, PUBSUB_PUBLISHER_TOPIC, sender->topic);
        if (sender->scope != NULL) {
            celix_properties_set(props, PUBSUB_PUBLISHER_SCOPE, sender->scope);
        }

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
        hashMap_destroy(sender->boundedServices.map, false, true);
        celixThreadMutex_unlock(&sender->boundedServices.mutex);

        celixThreadMutex_destroy(&sender->boundedServices.mutex);

        pubsubInterceptorsHandler_destroy(sender->interceptorsHandler);

        if (sender->scope != NULL) {
            free(sender->scope);
        }
        free(sender->topic);
        free(sender->url);
        free(sender->zmqBuffers.headerBuffer);
        free(sender->zmqBuffers.metadataBuffer);
        free(sender->zmqBuffers.footerBuffer);
        free(sender);
    }
}

const char* pubsub_zmqTopicSender_serializerType(pubsub_zmq_topic_sender_t *sender) {
    return pubsub_serializerHandler_getSerializationType(sender->serializerHandler);
}

long pubsub_zmqTopicSender_protocolSvcId(pubsub_zmq_topic_sender_t *sender) {
    return sender->protocolSvcId;
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

static int psa_zmq_localMsgTypeIdForMsgType(void* handle, const char* msgType, unsigned int* msgTypeId) {
    psa_zmq_bounded_service_entry_t *entry = (psa_zmq_bounded_service_entry_t *) handle;
    *msgTypeId = pubsub_serializerHandler_getMsgId(entry->parent->serializerHandler, msgType);
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
        entry->service.handle = entry;
        entry->service.localMsgTypeIdForMsgType = psa_zmq_localMsgTypeIdForMsgType;
        entry->service.send = psa_zmq_topicPublicationSend;
        hashMap_put(sender->boundedServices.map, (void*)bndId, entry);
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
        free(entry);
    }
    celixThreadMutex_unlock(&sender->boundedServices.mutex);
}

static void psa_zmq_freeMsg(void *msg, void *hint) {
    psa_zmq_zerocopy_free_entry *entry = hint;
    pubsub_serializerHandler_freeSerializedMsg(entry->serHandler, entry->msgId, entry->serializedOutput, entry->serializedOutputLen);
    free(entry);
}

static int psa_zmq_topicPublicationSend(void* handle, unsigned int msgTypeId, const void *inMsg, celix_properties_t *metadata) {
    psa_zmq_bounded_service_entry_t *bound = handle;
    pubsub_zmq_topic_sender_t *sender = bound->parent;

    const char* msgFqn;
    int majorVersion;
    int minorversion;
    celix_status_t status = pubsub_serializerHandler_getMsgInfo(sender->serializerHandler, msgTypeId, &msgFqn, &majorVersion, &minorversion);
    if (status != CELIX_SUCCESS) {
        L_WARN("Cannot find serializer for msg id %u for serializer %s", msgTypeId, pubsub_serializerHandler_getSerializationType(sender->serializerHandler));
        celix_properties_destroy(metadata);
        return status;
    }

    bool cont = pubsubInterceptorHandler_invokePreSend(sender->interceptorsHandler, msgFqn, msgTypeId, inMsg, &metadata);
    if (!cont) {
        L_DEBUG("Cancel send based on pubsub interceptor cancel return");
        celix_properties_destroy(metadata);
        return status;
    }

    size_t serializedIoVecOutputLen = 0; //entry->serializedIoVecOutputLen;
    struct iovec *serializedIoVecOutput = NULL;
    status = pubsub_serializerHandler_serialize(sender->serializerHandler, msgTypeId, inMsg, &serializedIoVecOutput, &serializedIoVecOutputLen);

    if (status != CELIX_SUCCESS /*serialization not ok*/) {
        L_WARN("[PSA_ZMQ_TS] Error serialize message of type %s for scope/topic %s/%s", msgFqn, sender->scope == NULL ? "(null)" : sender->scope, sender->topic);
        celix_properties_destroy(metadata);
        return status;
    }

    delay_first_send_for_late_joiners(sender);

    // Some ZMQ functions are not thread-safe, but this atomic compare exchange ensures one access at a time.
    // Also protect sender->zmqBuffers (header, meta and footer)
    bool expected = false;
    while(!__atomic_compare_exchange_n(&sender->zmqBuffers.dataLock, &expected, true, false, __ATOMIC_ACQ_REL, __ATOMIC_ACQUIRE)) {
        expected = false;
        usleep(5);
    }

    pubsub_protocol_message_t message;
    message.payload.payload = serializedIoVecOutput->iov_base;
    message.payload.length = serializedIoVecOutput->iov_len;

    void *payloadData = NULL;
    size_t payloadLength = 0;
    sender->protocol->encodePayload(sender->protocol->handle, &message, &payloadData, &payloadLength);

    if (metadata != NULL) {
        message.metadata.metadata = metadata;
        sender->protocol->encodeMetadata(sender->protocol->handle, &message, &sender->zmqBuffers.metadataBuffer, &sender->zmqBuffers.metadataBufferSize);
    } else {
        message.metadata.metadata = NULL;
    }

    sender->protocol->encodeFooter(sender->protocol->handle, &message, &sender->zmqBuffers.footerBuffer, &sender->zmqBuffers.footerBufferSize);

    message.header.msgId = msgTypeId;
    message.header.seqNr = __atomic_fetch_add(&sender->seqNr, 1, __ATOMIC_RELAXED);
    message.header.msgMajorVersion = majorVersion;
    message.header.msgMinorVersion = minorversion;
    message.header.payloadSize = payloadLength;
    message.header.metadataSize = sender->zmqBuffers.metadataBufferSize;
    message.header.payloadPartSize = payloadLength;
    message.header.payloadOffset = 0;
    message.header.isLastSegment = 1;
    message.header.convertEndianess = 0;

    sender->protocol->encodeHeader(sender->protocol->handle, &message, &sender->zmqBuffers.headerBuffer, &sender->zmqBuffers.headerBufferSize);

    errno = 0;
    bool sendOk;
    if (bound->parent->zeroCopyEnabled) {
        zmq_msg_t msg1; // Header
        zmq_msg_t msg2; // Payload
        zmq_msg_t msg3; // Metadata
        zmq_msg_t msg4; // Footer
        void *socket = zsock_resolve(sender->zmq.socket);
        psa_zmq_zerocopy_free_entry *freeMsgEntry = malloc(sizeof(psa_zmq_zerocopy_free_entry)); //NOTE should be improved. Not really zero copy
        freeMsgEntry->serHandler = sender->serializerHandler;
        freeMsgEntry->msgId = msgTypeId;
        freeMsgEntry->serializedOutput = serializedIoVecOutput;
        freeMsgEntry->serializedOutputLen = serializedIoVecOutputLen;

        zmq_msg_init_data(&msg1, sender->zmqBuffers.headerBuffer, sender->zmqBuffers.headerBufferSize, NULL, NULL);
        //send header
        int rc = zmq_msg_send(&msg1, socket, ZMQ_SNDMORE);
        if (rc == -1) {
            L_WARN("Error sending header msg. %s", strerror(errno));
            zmq_msg_close(&msg1);
        }

        //send Payload
        if (rc > 0) {
            int flag = ((sender->zmqBuffers.metadataBufferSize > 0)  || (sender->zmqBuffers.footerBufferSize > 0)) ? ZMQ_SNDMORE : 0;
            zmq_msg_init_data(&msg2, payloadData, payloadLength, psa_zmq_freeMsg, freeMsgEntry);
            rc = zmq_msg_send(&msg2, socket, flag);
            if (rc == -1) {
                L_WARN("Error sending payload msg. %s", strerror(errno));
                zmq_msg_close(&msg2);
            }
        }

        //send MetaData
        if (rc > 0 && sender->zmqBuffers.metadataBufferSize > 0) {
            int flag = (sender->zmqBuffers.footerBufferSize > 0 ) ? ZMQ_SNDMORE : 0;
            zmq_msg_init_data(&msg3, sender->zmqBuffers.metadataBuffer, sender->zmqBuffers.metadataBufferSize, NULL, NULL);
            rc = zmq_msg_send(&msg3, socket, flag);
            if (rc == -1) {
                L_WARN("Error sending metadata msg. %s", strerror(errno));
                zmq_msg_close(&msg3);
            }
        }

        //send Footer
        if (rc > 0 && sender->zmqBuffers.footerBufferSize > 0) {
            zmq_msg_init_data(&msg4, sender->zmqBuffers.footerBuffer, sender->zmqBuffers.footerBufferSize, NULL, NULL);
            rc = zmq_msg_send(&msg4, socket, 0);
            if (rc == -1) {
                L_WARN("Error sending footer msg. %s", strerror(errno));
                zmq_msg_close(&msg4);
            }
        }
        sendOk = rc > 0;
    } else {
        //no zero copy
        zmsg_t *msg = zmsg_new();
        zmsg_addmem(msg, sender->zmqBuffers.headerBuffer, sender->zmqBuffers.headerBufferSize);
        zmsg_addmem(msg, payloadData, payloadLength);
        if (sender->zmqBuffers.metadataBufferSize > 0) {
            zmsg_addmem(msg, sender->zmqBuffers.metadataBuffer, sender->zmqBuffers.metadataBufferSize);
        }
        if (sender->zmqBuffers.footerBufferSize > 0) {
            zmsg_addmem(msg, sender->zmqBuffers.footerBuffer, sender->zmqBuffers.footerBufferSize);
        }
        int rc = zmsg_send(&msg, sender->zmq.socket);
        sendOk = rc == 0;

        if (!sendOk) {
            zmsg_destroy(&msg); //if send was not ok, no owner change -> destroy msg
        }

        // Note: serialized Payload is deleted by serializer
        if (payloadData && (payloadData != message.payload.payload)) {
            free(payloadData);
        }
    }
    __atomic_store_n(&sender->zmqBuffers.dataLock, false, __ATOMIC_RELEASE);
    pubsubInterceptorHandler_invokePostSend(sender->interceptorsHandler, msgFqn, msgTypeId, inMsg, metadata);

    if (!bound->parent->zeroCopyEnabled && serializedIoVecOutput) {
        pubsub_serializerHandler_freeSerializedMsg(sender->serializerHandler, msgTypeId, serializedIoVecOutput, serializedIoVecOutputLen);
    }

    if (!sendOk) {
        L_WARN("[PSA_ZMQ_TS] Error sending zmg. %s", strerror(errno));
    }

    celix_properties_destroy(metadata);
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
