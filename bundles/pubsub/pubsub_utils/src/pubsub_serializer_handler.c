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


#include "pubsub_serializer_handler.h"

#include <string.h>

#include "celix_version.h"
#include "pubsub_message_serialization_service.h"
#include "celix_log_helper.h"

#define L_DEBUG(...) \
    celix_logHelper_debug(handler->logHelper, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_info(handler->logHelper, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_warning(handler->logHelper, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_error(handler->logHelper, __VA_ARGS__)

typedef struct pubsub_serialization_service_entry {
    long svcId;
    const celix_properties_t *properties;
    uint32_t msgId;
    celix_version_t* msgVersion;
    char* msgFqn;
    pubsub_message_serialization_service_t* svc;
} pubsub_serialization_service_entry_t;

struct pubsub_serializer_handler {
    celix_bundle_context_t* ctx;
    bool backwardCompatible;
    long serializationSvcTrackerId;
    celix_log_helper_t *logHelper;

    celix_thread_mutex_t mutex;
    hash_map_t *serializationServices; //key = msg id, value = sorted array list with pubsub_serialization_service_entry_t*
};

static void addSvc(void *handle, void* svc, const celix_properties_t *props) {
    pubsub_serializer_handler_t* handler = handle;
    pubsub_message_serialization_service_t* serSvc = svc;
    pubsub_serializerHandler_addSerializationService(handler, serSvc, props);
}

static void remSvc(void *handle, void* svc, const celix_properties_t *props) {
    pubsub_serializer_handler_t* handler = handle;
    pubsub_message_serialization_service_t* serSvc = svc;
    pubsub_serializerHandler_removeSerializationService(handler, serSvc, props);
}

int compareEntries(const void *a, const void *b) {
    const pubsub_serialization_service_entry_t* aEntry = a;
    const pubsub_serialization_service_entry_t* bEntry = b;

    long servIdA = celix_properties_getAsLong(aEntry->properties, OSGI_FRAMEWORK_SERVICE_ID, 0);
    long servIdB = celix_properties_getAsLong(bEntry->properties, OSGI_FRAMEWORK_SERVICE_ID, 0);

    long servRankingA = celix_properties_getAsLong(aEntry->properties, OSGI_FRAMEWORK_SERVICE_RANKING, 0);
    long servRankingB = celix_properties_getAsLong(bEntry->properties, OSGI_FRAMEWORK_SERVICE_RANKING, 0);

    return utils_compareServiceIdsAndRanking(servIdA, servRankingA, servIdB, servRankingB);
}

static pubsub_serialization_service_entry_t* findEntry(pubsub_serializer_handler_t* handler, uint32_t msgId) {
    //NOTE assumes mutex is locked
    celix_array_list_t* entries = hashMap_get(handler->serializationServices, (void*)(uintptr_t)msgId);
    if (entries != NULL) {
        return celix_arrayList_get(entries, 0); //NOTE if entries not null, always at least 1 entry
    }
    return NULL;
}

static bool isCompatible(pubsub_serializer_handler_t* handler, pubsub_serialization_service_entry_t* entry, int serializedMajorVersion, int serializedMinorVersion) {
    bool compatible = false;
    if (handler->backwardCompatible) {
        compatible = celix_version_isUserCompatible(entry->msgVersion, serializedMajorVersion, serializedMinorVersion);
    } else {
        int major = celix_version_getMajor(entry->msgVersion);
        int minor = celix_version_getMinor(entry->msgVersion);
        compatible = major == serializedMajorVersion && minor == serializedMinorVersion;
    }
    return compatible;
}

static const char* getMsgFqn(pubsub_serializer_handler_t* handler, uint32_t msgId) {
    //NOTE assumes mutex is locked
    const char *result = NULL;
    celix_array_list_t* entries = hashMap_get(handler->serializationServices, (void*)(uintptr_t)msgId);
    if (entries != NULL) {
        pubsub_serialization_service_entry_t *entry = celix_arrayList_get(entries, 0); //NOTE if an entries exists, there is at least 1 entry.
        result = entry->msgFqn;
    }
    return result;
}

pubsub_serializer_handler_t* pubsub_serializerHandler_create(celix_bundle_context_t* ctx, const char* serializerType, bool backwardCompatible) {
    pubsub_serializer_handler_t* handler = calloc(1, sizeof(*handler));
    handler->ctx = ctx;
    handler->backwardCompatible = backwardCompatible;

    handler->logHelper = celix_logHelper_create(ctx, "celix_pubsub_serialization_handler");

    celixThreadMutex_create(&handler->mutex, NULL);
    handler->serializationServices = hashMap_create(NULL, NULL, NULL, NULL);

    char *filter = NULL;
    asprintf(&filter, "(%s=%s)", PUBSUB_MESSAGE_SERIALIZATION_SERVICE_SERIALIZATION_TYPE_PROPERTY, serializerType);
    celix_service_tracking_options_t opts = CELIX_EMPTY_SERVICE_TRACKING_OPTIONS;
    opts.filter.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
    opts.filter.versionRange = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_RANGE;
    opts.filter.filter = filter;
    opts.callbackHandle = handler;
    opts.addWithProperties = addSvc;
    opts.removeWithProperties = remSvc;
    handler->serializationSvcTrackerId = celix_bundleContext_trackServicesWithOptions(ctx, &opts);
    free(filter);

    return handler;
}


void pubsub_serializerHandler_destroy(pubsub_serializer_handler_t* handler) {
    if (handler != NULL) {
        celix_bundleContext_stopTracker(handler->ctx, handler->serializationSvcTrackerId);
        celixThreadMutex_destroy(&handler->mutex);
        hash_map_iterator_t iter = hashMapIterator_construct(handler->serializationServices);
        while (hashMapIterator_hasNext(&iter)) {
            celix_array_list_t *entries = hashMapIterator_nextValue(&iter);
            for (int i = 0; i < celix_arrayList_size(entries); ++i) {
                pubsub_serialization_service_entry_t* entry = celix_arrayList_get(entries, i);
                free(entry->msgFqn);
                celix_version_destroy(entry->msgVersion);
                free(entry);
            }
            celix_arrayList_destroy(entries);
        }
        hashMap_destroy(handler->serializationServices, false, false);
        celix_logHelper_destroy(handler->logHelper);
        free(handler);
    }
}

void pubsub_serializerHandler_addSerializationService(pubsub_serializer_handler_t* handler, pubsub_message_serialization_service_t* svc, const celix_properties_t* svcProperties) {
    long svcId = celix_properties_getAsLong(svcProperties, OSGI_FRAMEWORK_SERVICE_ID, -1L);
    const char *msgFqn = celix_properties_get(svcProperties, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY, NULL);
    const char *version = celix_properties_get(svcProperties, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY, "0.0.0");
    uint32_t msgId = (uint32_t)celix_properties_getAsLong(svcProperties, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY, 0L);

    if (msgId == 0) {
        msgId = celix_utils_stringHash(msgFqn);
    }

    celix_version_t* msgVersion = celix_version_createVersionFromString(version);
    if (msgVersion == NULL) {
        L_ERROR("%s service has an invalid %s property. value is '%s'", PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY, msgVersion);
        return;
    }

    celixThreadMutex_lock(&handler->mutex);

    pubsub_serialization_service_entry_t* existingEntry = findEntry(handler, msgId);

    bool valid = true;
    if (existingEntry != NULL && strncmp(existingEntry->msgFqn, msgFqn, 1024*1024) != 0) {
        L_ERROR("Msg id clash. Registered serialization service with msg id %d and msg fqn '%s' clashes with an existing serialization service using the same msg id and msg fqn '%s'. Ignoring serialization service.", msgId, msgFqn, existingEntry->msgFqn);
        valid = false;
    }

    if (existingEntry != NULL && celix_version_compareTo(existingEntry->msgVersion, msgVersion) != 0) {
        char* existingVersion = celix_version_toString(existingEntry->msgVersion);
        L_ERROR("Mismatched message versions. Registered serialization service with msg '%s' with version %s, has a different version than an existing serialization service with version '%s'. Ignoring serialization service.", msgFqn, version, existingVersion);
        free(existingVersion);
        valid = false;
    }

    if (valid) {
        celix_array_list_t *entries = hashMap_get(handler->serializationServices, (void *) (uintptr_t) msgId);
        if (entries == NULL) {
            entries = celix_arrayList_create();
        }
        pubsub_serialization_service_entry_t *entry = calloc(1, sizeof(*entry));
        entry->svcId = svcId;
        entry->properties = svcProperties;
        entry->msgFqn = celix_utils_strdup(msgFqn);
        entry->msgId = msgId;
        entry->msgVersion = msgVersion;
        entry->svc = svc;
        celix_arrayList_add(entries, entry);
        celix_arrayList_sort(entries, compareEntries);

        hashMap_put(handler->serializationServices, (void *) (uintptr_t) msgId, entries);
    } else {
        celix_version_destroy(msgVersion);
    }
    celixThreadMutex_unlock(&handler->mutex);
}

void pubsub_serializerHandler_removeSerializationService(pubsub_serializer_handler_t* handler, pubsub_message_serialization_service_t* svc, const celix_properties_t* svcProperties) {
    long svcId = celix_properties_getAsLong(svcProperties, OSGI_FRAMEWORK_SERVICE_ID, -1L);
    const char *msgFqn = celix_properties_get(svcProperties, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY, NULL);
    uint32_t msgId = (uint32_t)celix_properties_getAsLong(svcProperties, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY, 0L);
    if (msgId == 0) {
        msgId = celix_utils_stringHash(msgFqn);
    }

    celixThreadMutex_lock(&handler->mutex);
    celix_array_list_t* entries = hashMap_get(handler->serializationServices, (void*)(uintptr_t)msgId);
    if (entries != NULL) {
        pubsub_serialization_service_entry_t *found = NULL;
        for (int i = 0; i < celix_arrayList_size(entries); ++i) {
            pubsub_serialization_service_entry_t *entry = celix_arrayList_get(entries, i);
            if (entry->svcId == svcId) {
                found = entry;
                celix_arrayList_removeAt(entries, i);
                celix_arrayList_sort(entries, compareEntries);
                break;
            }
        }
        if (found != NULL) {
            free(found->msgFqn);
            celix_version_destroy(found->msgVersion);
            free(found);
        }
        if (celix_arrayList_size(entries) == 0) {
            hashMap_remove(handler->serializationServices, (void*)(uintptr_t)msgId);
            celix_arrayList_destroy(entries);
        }
    }
    celixThreadMutex_unlock(&handler->mutex);
}

celix_status_t pubsub_serializerHandler_serialize(pubsub_serializer_handler_t* handler, uint32_t msgId, const void* input, struct iovec** output, size_t* outputIovLen) {
    celix_status_t status;
    celixThreadMutex_lock(&handler->mutex);
    pubsub_serialization_service_entry_t* entry = findEntry(handler, msgId);
    if (entry != NULL) {
        status = entry->svc->serialize(entry->svc->handle, input, output, outputIovLen);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
        L_ERROR("Cannot find message serialization service for msg id %u.", msgId);
    }
    celixThreadMutex_unlock(&handler->mutex);
    return status;
}

celix_status_t pubsub_serializerHandler_freeSerializedMsg(pubsub_serializer_handler_t* handler, uint32_t msgId, struct iovec* input, size_t inputIovLen) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadMutex_lock(&handler->mutex);
    pubsub_serialization_service_entry_t* entry = findEntry(handler, msgId);
    if (entry != NULL) {
        entry->svc->freeSerializedMsg(entry->svc->handle, input, inputIovLen);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
        L_ERROR("Cannot find message serialization service for msg id %u.", msgId);
    }
    celixThreadMutex_unlock(&handler->mutex);
    return status;

}

celix_status_t pubsub_serializerHandler_deserialize(pubsub_serializer_handler_t* handler, uint32_t msgId, int serializedMajorVersion, int serializedMinorVersion, const struct iovec* input, size_t inputIovLen, void** out) {
    celix_status_t status;
    celixThreadMutex_lock(&handler->mutex);
    pubsub_serialization_service_entry_t* entry = findEntry(handler, msgId);
    bool compatible = false;
    if (entry != NULL) {
        compatible = isCompatible(handler, entry, serializedMajorVersion, serializedMinorVersion);
        if (compatible) {
            status = entry->svc->deserialize(entry->svc->handle, input, inputIovLen, out);
        } else {
            status = CELIX_ILLEGAL_ARGUMENT;
            char *version = celix_version_toString(entry->msgVersion);
            L_ERROR("Cannot deserialize for message %s version %s. The serialized input has a version of %d.%d.x and this is incompatible.", entry->msgFqn, version, serializedMajorVersion, serializedMinorVersion);
            free(version);
        }
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
        L_ERROR("Cannot find message serialization service for msg id %u.", msgId);
    }
    celixThreadMutex_unlock(&handler->mutex);
    return status;
}

celix_status_t pubsub_serializerHandler_freeDeserializedMsg(pubsub_serializer_handler_t* handler, uint32_t msgId, void* msg) {
    celix_status_t status = CELIX_SUCCESS;
    celixThreadMutex_lock(&handler->mutex);
    pubsub_serialization_service_entry_t* entry = findEntry(handler, msgId);
    if (entry != NULL) {
        entry->svc->freeDeserializedMsg(entry->svc->handle, msg);
    } else {
        status = CELIX_ILLEGAL_ARGUMENT;
        L_ERROR("Cannot find message serialization service for msg id %u.", msgId);
    }
    celixThreadMutex_unlock(&handler->mutex);
    return status;
}

bool pubsub_serializerHandler_supportMsg(pubsub_serializer_handler_t* handler, uint32_t msgId, int serializedMajorVersion, int serializedMinorVersion) {
    celixThreadMutex_lock(&handler->mutex);
    bool compatible = false;
    pubsub_serialization_service_entry_t* entry = findEntry(handler, msgId);
    if (entry != NULL) {
        compatible = isCompatible(handler, entry, serializedMajorVersion, serializedMinorVersion);
    }
    celixThreadMutex_unlock(&handler->mutex);
    return compatible;
}

char* pubsub_serializerHandler_getMsgFqn(pubsub_serializer_handler_t* handler, uint32_t msgId) {
    celixThreadMutex_lock(&handler->mutex);
    char *msgFqn = celix_utils_strdup(getMsgFqn(handler, msgId));
    celixThreadMutex_unlock(&handler->mutex);
    return msgFqn;

}

uint32_t pubsub_serializerHandler_getMsgId(pubsub_serializer_handler_t* handler, const char* msgFqn) {
    uint32_t result = 0;
    celixThreadMutex_lock(&handler->mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(handler->serializationServices);
    while (hashMapIterator_hasNext(&iter) && result == 0) {
        celix_array_list_t *entries = hashMapIterator_nextValue(&iter);
        pubsub_serialization_service_entry_t* entry = celix_arrayList_get(entries, 0); //NOTE if an entries exists, there is at least 1 entry.
        if (strncmp(entry->msgFqn, msgFqn, 1024*1024) == 0) {
            result = entry->msgId;
        }
    }
    celixThreadMutex_unlock(&handler->mutex);
    return result;
}

size_t pubsub_serializerHandler_messageSerializationServiceCount(pubsub_serializer_handler_t* handler) {
    size_t count = 0;
    celixThreadMutex_lock(&handler->mutex);
    hash_map_iterator_t iter = hashMapIterator_construct(handler->serializationServices);
    while (hashMapIterator_hasNext(&iter)) {
        celix_array_list_t *entries = hashMapIterator_nextValue(&iter);
        count += celix_arrayList_size(entries);
    }
    celixThreadMutex_unlock(&handler->mutex);
    return count;
}