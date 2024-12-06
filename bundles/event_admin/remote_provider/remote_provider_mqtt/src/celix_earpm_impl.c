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
#include "celix_earpm_impl.h"

#include <assert.h>
#include <string.h>
#include <stdbool.h>
#include <errno.h>
#include <stdlib.h>
#include <stdio.h>
#include <jansson.h>

#include "celix_cleanup.h"
#include "celix_stdlib_cleanup.h"
#include "celix_utils.h"
#include "celix_string_hash_map.h"
#include "celix_long_hash_map.h"
#include "celix_array_list.h"
#include "celix_threads.h"
#include "celix_constants.h"
#include "celix_filter.h"
#include "celix_event_constants.h"
#include "celix_event_remote_provider_service.h"
#include "celix_earpm_event_deliverer.h"
#include "celix_earpm_client.h"
#include "celix_earpm_constants.h"
#include "celix_earpm_broker_discovery.h"

/**
 * @brief The remote sync event default timeout in seconds. Remote event can use CELIX_EVENT_REMOTE_EXPIRY_INTERVAL to override this value.
 * If the event remote provider does not receive an ack within this time, the event will be delivered failed.
 */
#define CELIX_EARPM_SYNC_EVENT_TIMEOUT_DEFAULT (5*60) //seconds

/**
 * @brief The version of the remote provider messages(It contains event messages and control messages).
 */
#define CELIX_EARPM_MSG_VERSION "1.0.0"

typedef struct celix_earpm_event_handler {
    celix_array_list_t* topics;
    char* filter;
    celix_earpm_qos_e qos;
    long serviceId;
} celix_earpm_event_handler_t;

typedef struct celix_earpm_event_subscription {
    celix_array_list_t* handlerServiceIdList;
    celix_earpm_qos_e curQos;
} celix_earpm_event_subscription_t;

typedef struct celix_earpm_remote_handler_info {
    celix_array_list_t* topics;
    celix_filter_t* filter;
} celix_earpm_remote_handler_info_t;

typedef struct celix_earpm_remote_framework_info {
    celix_long_hash_map_t* handlerInfoMap;//key = serviceId, value = celix_earpm_remote_handler_info_t*
    celix_long_hash_map_t* eventAckSeqNrMap;//key = seqNr, value = celix_array_list_t* of serviceIds
    int continuousNoAckCount;
} celix_earpm_remote_framework_info_t;

struct celix_earpm_sync_event_correlation_data {
    long ackSeqNr;
};

struct celix_earpm_send_event_done_callback_data {
    celix_event_admin_remote_provider_mqtt_t* earpm;
    char* responseTopic;
    void* correlationData;
    size_t correlationDataSize;
};

struct celix_event_admin_remote_provider_mqtt {
    celix_bundle_context_t* ctx;
    celix_log_helper_t* logHelper;
    const char* fwUUID;
    celix_earpm_qos_e defaultQos;
    int continuousNoAckThreshold;
    char* syncEventAckTopic;
    celix_earpm_event_deliverer_t* deliverer;
    celix_earpm_client_t* mqttClient;
    long lastAckSeqNr;
    celix_thread_mutex_t mutex;//protects belows
    celix_thread_cond_t ackCond;
    celix_long_hash_map_t* eventHandlers;//key = serviceId, value = celix_earpm_event_handler_t*
    celix_string_hash_map_t* eventSubscriptions;//key = topic, value = celix_earpm_event_subscription_t*
    celix_string_hash_map_t* remoteFrameworks;// key = frameworkUUID of remote frameworks, value = celix_earpm_remote_framework_info_t*
    bool destroying;
};

static void celix_earpm_eventHandlerDestroy(celix_earpm_event_handler_t* handler);
static void celix_earpm_subscriptionDestroy(celix_earpm_event_subscription_t* subscription);
static void celix_earpm_remoteFrameworkInfoDestroy(celix_earpm_remote_framework_info_t* info);
static void celix_earpm_receiveMsgCallback(void* handle, const celix_earpm_client_request_info_t* requestInfo);
static void celix_earpm_connectedCallback(void* handle);
static void celix_earpm_subOrUnsubRemoteEventsForHandler(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_event_handler_t* handler, bool subscribe);
static void celix_earpm_addHandlerInfoToRemote(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_event_handler_t* handler);
static void celix_earpm_removeHandlerInfoFromRemote(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_event_handler_t* handler);
static celix_status_t celix_earpm_publishEvent(celix_event_admin_remote_provider_mqtt_t* earpm, const char *topic,
                                               const celix_properties_t *eventProps, bool async);


celix_event_admin_remote_provider_mqtt_t* celix_earpm_create(celix_bundle_context_t* ctx) {
    assert(ctx != NULL);
    celix_autofree celix_event_admin_remote_provider_mqtt_t* earpm = calloc(1, sizeof(*earpm));
    if (earpm == NULL) {
        return NULL;
    }
    earpm->ctx = ctx;
    earpm->lastAckSeqNr = 0;
    earpm->destroying = false;
    celix_autoptr(celix_log_helper_t) logHelper = earpm->logHelper = celix_logHelper_create(ctx, "celix_earpm");
    if (logHelper == NULL) {
        return NULL;
    }
    earpm->fwUUID = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, NULL);
    if (earpm->fwUUID == NULL) {
        celix_logHelper_error(logHelper, "Failed to get framework UUID.");
        return NULL;
    }
    earpm->defaultQos = (celix_earpm_qos_e)celix_bundleContext_getPropertyAsLong(ctx, CELIX_EARPM_EVENT_DEFAULT_QOS, CELIX_EARPM_EVENT_DEFAULT_QOS_DEFAULT);
    if (earpm->defaultQos <= CELIX_EARPM_QOS_UNKNOWN || earpm->defaultQos >= CELIX_EARPM_QOS_MAX) {
        celix_logHelper_error(logHelper, "Invalid default QOS(%d) value.", (int)earpm->defaultQos);
        return NULL;
    }

    earpm->continuousNoAckThreshold = (int)celix_bundleContext_getPropertyAsLong(ctx, CELIX_EARPM_SYNC_EVENT_CONTINUOUS_NO_ACK_THRESHOLD, CELIX_EARPM_SYNC_EVENT_CONTINUOUS_NO_ACK_THRESHOLD_DEFAULT);
    if (earpm->continuousNoAckThreshold <= 0) {
        celix_logHelper_error(logHelper, "Invalid continuous no ack threshold(%d) value.", earpm->continuousNoAckThreshold);
        return NULL;
    }

    if (asprintf(&earpm->syncEventAckTopic, CELIX_EARPM_SYNC_EVENT_ACK_TOPIC_PREFIX"%s", earpm->fwUUID) < 0) {
        celix_logHelper_error(logHelper, "Failed to create sync event response topic.");
        return NULL;
    }
    celix_autofree char* syncEventAckTopic = earpm->syncEventAckTopic;
    celix_status_t status = celixThreadMutex_create(&earpm->mutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create mutex, %d.", status);
        return NULL;
    }
    celix_autoptr(celix_thread_mutex_t) mutex = &earpm->mutex;
    status = celixThreadCondition_init(&earpm->ackCond, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create condition for event ack, %d.", status);
        return NULL;
    }
    celix_autoptr(celix_thread_cond_t) ackCond = &earpm->ackCond;
    celix_autoptr(celix_long_hash_map_t) eventHandlers = NULL;
    {
        celix_long_hash_map_create_options_t opts = CELIX_EMPTY_LONG_HASH_MAP_CREATE_OPTIONS;
        opts.simpleRemovedCallback = (void *)celix_earpm_eventHandlerDestroy;
        eventHandlers = earpm->eventHandlers = celix_longHashMap_createWithOptions(&opts);
        if (eventHandlers == NULL) {
            celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(logHelper, "Failed to create local event handler map.");
            return NULL;
        }
    }
    celix_autoptr(celix_string_hash_map_t) eventSubscriptions = NULL;
    {
        celix_string_hash_map_create_options_t opts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
        opts.simpleRemovedCallback = (void *)celix_earpm_subscriptionDestroy;
        eventSubscriptions = earpm->eventSubscriptions = celix_stringHashMap_createWithOptions(&opts);
        if (eventSubscriptions == NULL) {
            celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(logHelper, "Failed to create event subscription map for local event handler.");
            return NULL;
        }
    }
    celix_autoptr(celix_string_hash_map_t) remoteFrameworks = NULL;
    {
        celix_string_hash_map_create_options_t opts = CELIX_EMPTY_STRING_HASH_MAP_CREATE_OPTIONS;
        opts.simpleRemovedCallback = (void*)celix_earpm_remoteFrameworkInfoDestroy;
        remoteFrameworks = earpm->remoteFrameworks = celix_stringHashMap_createWithOptions(&opts);
        if (remoteFrameworks == NULL) {
            celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(logHelper, "Failed to create remote framework information map.");
            return NULL;
        }
    }
    celix_autoptr(celix_earpm_event_deliverer_t) deliverer = earpm->deliverer = celix_earpmDeliverer_create(ctx, logHelper);
    if (deliverer == NULL) {
        celix_logHelper_error(logHelper, "Failed to create event deliverer.");
        return NULL;
    }
    celix_earpm_client_create_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.ctx = ctx;
    opts.logHelper = logHelper;
    opts.sessionEndMsgTopic = CELIX_EARPM_SESSION_END_TOPIC;
    opts.sessionEndMsgSenderUUID = earpm->fwUUID;
    opts.sessionEndMsgVersion = CELIX_EARPM_MSG_VERSION;
    opts.callbackHandle = earpm;
    opts.receiveMsgCallback = celix_earpm_receiveMsgCallback;
    opts.connectedCallback = celix_earpm_connectedCallback;
    celix_autoptr(celix_earpm_client_t) mqttClient = earpm->mqttClient = celix_earpmClient_create(&opts);
    if (mqttClient == NULL) {
        celix_logHelper_error(logHelper, "Failed to create mqtt client.");
        return NULL;
    }
    status = celix_earpmClient_subscribe(earpm->mqttClient, CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX"*", CELIX_EARPM_QOS_AT_LEAST_ONCE);
    status = CELIX_DO_IF(status, celix_earpmClient_subscribe(earpm->mqttClient, CELIX_EARPM_SESSION_END_TOPIC, CELIX_EARPM_QOS_AT_LEAST_ONCE));
    status = CELIX_DO_IF(status, celix_earpmClient_subscribe(earpm->mqttClient, syncEventAckTopic, CELIX_EARPM_QOS_AT_LEAST_ONCE));
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to subscribe control message. %d.", status);
        return NULL;
    }

    celix_steal_ptr(mqttClient);
    celix_steal_ptr(deliverer);
    celix_steal_ptr(remoteFrameworks);
    celix_steal_ptr(eventSubscriptions);
    celix_steal_ptr(eventHandlers);
    celix_steal_ptr(ackCond);
    celix_steal_ptr(syncEventAckTopic);
    celix_steal_ptr(mutex);
    celix_steal_ptr(logHelper);

    return celix_steal_ptr(earpm);
}

void celix_earpm_destroy(celix_event_admin_remote_provider_mqtt_t* earpm) {
    assert(earpm != NULL);
    celixThreadMutex_lock(&earpm->mutex);
    earpm->destroying = true;
    celixThreadMutex_unlock(&earpm->mutex);
    celix_earpmClient_destroy(earpm->mqttClient);
    celix_earpmDeliverer_destroy(earpm->deliverer);
    celix_stringHashMap_destroy(earpm->remoteFrameworks);
    celix_stringHashMap_destroy(earpm->eventSubscriptions);
    celix_longHashMap_destroy(earpm->eventHandlers);
    celixThreadCondition_destroy(&earpm->ackCond);
    celixThreadMutex_destroy(&earpm->mutex);
    free(earpm->syncEventAckTopic);
    celix_logHelper_destroy(earpm->logHelper);
    free(earpm);
    return;
}

celix_status_t celix_earpm_mqttBrokerEndpointAdded(void* handle, endpoint_description_t* endpoint, char* matchedFilter) {
    assert(handle != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;
    return celix_earpmClient_mqttBrokerEndpointAdded(earpm->mqttClient, endpoint, matchedFilter);
}

celix_status_t celix_earpm_mqttBrokerEndpointRemoved(void* handle, endpoint_description_t* endpoint, char* matchedFilter) {
    assert(handle != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;
    return celix_earpmClient_mqttBrokerEndpointRemoved(earpm->mqttClient, endpoint, matchedFilter);
}

static celix_earpm_event_handler_t* celix_earpm_createEventHandler(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_properties_t* eventHandlerProperties) {
    long serviceId = celix_properties_getAsLong(eventHandlerProperties, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (serviceId < 0 ) {
        celix_logHelper_error(earpm->logHelper, "Failed to add event handler service, no service id found.");
        return NULL;
    }
    celix_earpm_qos_e qos = (celix_earpm_qos_e)celix_properties_getAsLong(eventHandlerProperties, CELIX_EVENT_REMOTE_QOS, earpm->defaultQos);
    if (qos <= CELIX_EARPM_QOS_UNKNOWN || qos >= CELIX_EARPM_QOS_MAX) {
        celix_logHelper_error(earpm->logHelper, "Failed to add event handler service, invalid qos %d.", (int)qos);
        return NULL;
    }
    celix_autoptr(celix_array_list_t) topics = NULL;
    celix_status_t status = celix_properties_getAsStringArrayList(eventHandlerProperties, CELIX_EVENT_TOPIC, NULL, &topics);
    if (status != CELIX_SUCCESS || topics == NULL || celix_arrayList_size(topics) == 0) {
        celix_logHelper_error(earpm->logHelper, "Failed to add event handler service, maybe topic pattern is invalid. %d.", status);
        return NULL;
    }
    celix_autofree char* filterCopy = NULL;
    const char* filter = celix_properties_get(eventHandlerProperties, CELIX_EVENT_FILTER, NULL);
    if (filter != NULL) {
        filterCopy = celix_utils_strdup(filter);
        if (filterCopy == NULL) {
            celix_logHelper_error(earpm->logHelper, "Failed to add event handler service, out of memory.");
            return NULL;
        }
    }
    celix_autofree celix_earpm_event_handler_t* handler = calloc(1, sizeof(*handler));
    if (handler == NULL) {
        return NULL;
    }
    handler->serviceId = serviceId;
    handler->topics = celix_steal_ptr(topics);
    handler->filter = celix_steal_ptr(filterCopy);
    handler->qos = qos;

    return celix_steal_ptr(handler);
}

static void celix_earpm_eventHandlerDestroy(celix_earpm_event_handler_t* handler) {
    celix_arrayList_destroy(handler->topics);
    free(handler->filter);
    free(handler);
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_event_handler_t, celix_earpm_eventHandlerDestroy)

static celix_earpm_event_subscription_t* celix_earpm_subscriptionCreate(void) {
    celix_autofree celix_earpm_event_subscription_t* subscription = calloc(1, sizeof(*subscription));
    if (subscription == NULL) {
        return NULL;
    }
    subscription->curQos = CELIX_EARPM_QOS_UNKNOWN;
    subscription->handlerServiceIdList = celix_arrayList_create();
    if (subscription->handlerServiceIdList == NULL) {
        return NULL;
    }
    return celix_steal_ptr(subscription);
}

static void celix_earpm_subscriptionDestroy(celix_earpm_event_subscription_t* subscription) {
    celix_arrayList_destroy(subscription->handlerServiceIdList);
    free(subscription);
    return;
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_event_subscription_t, celix_earpm_subscriptionDestroy)

static void celix_earpm_subscribeEvent(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, celix_earpm_qos_e qos, long handlerServiceId) {
    celix_status_t status = CELIX_SUCCESS;
    celix_earpm_event_subscription_t* subscription= celix_stringHashMap_get(earpm->eventSubscriptions, topic);
    if (subscription == NULL) {
        celix_autoptr(celix_earpm_event_subscription_t) _subscription = celix_earpm_subscriptionCreate();
        if (_subscription == NULL) {
            celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(earpm->logHelper, "Failed to create subscription info for %s.", topic);
            return;
        }
        status = celix_stringHashMap_put(earpm->eventSubscriptions, topic, _subscription);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(earpm->logHelper, "Failed to add subscription info for %s. %d.", topic, status);
            return;
        }
        subscription = celix_steal_ptr(_subscription);
    }
    status = celix_arrayList_addLong(subscription->handlerServiceIdList, handlerServiceId);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(earpm->logHelper, "Failed to attach handler service(%ld) to subscription info for %s. %d.",handlerServiceId, topic, status);
        if (celix_arrayList_size(subscription->handlerServiceIdList) == 0) {
            celix_stringHashMap_remove(earpm->eventSubscriptions, topic);
        }
        return;
    }
    if (subscription->curQos < qos) {
        status = celix_earpmClient_subscribe(earpm->mqttClient, topic, qos);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(earpm->logHelper, "Failed to subscribe %s with qos %d. %d.", topic, (int)qos, status);
            return;
        }
        subscription->curQos = qos;
    }
    return;
}

static void celix_earpm_unsubscribeEvent(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, celix_earpm_qos_e qos, long handlerServiceId) {
    celix_earpm_event_subscription_t* subscription= celix_stringHashMap_get(earpm->eventSubscriptions, topic);
    if (subscription == NULL) {
        celix_logHelper_debug(earpm->logHelper, "No subscription found for %s.", topic);
        return;
    }
    celix_array_list_entry_t entry;
    memset(&entry, 0, sizeof(entry));
    entry.longVal = handlerServiceId;
    int index = celix_arrayList_indexOf(subscription->handlerServiceIdList, entry);
    if (index < 0) {
        celix_logHelper_debug(earpm->logHelper, "Not found handler(%ld) in %s subscription.", handlerServiceId, topic);
        return;
    }
    celix_arrayList_removeAt(subscription->handlerServiceIdList, index);
    int size = celix_arrayList_size(subscription->handlerServiceIdList);
    if (size == 0) {
        celix_status_t status = celix_earpmClient_unsubscribe(earpm->mqttClient, topic);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_warning(earpm->logHelper, "Failed to unsubscribe %s.", topic);
        }
        celix_stringHashMap_remove(earpm->eventSubscriptions, topic);
    } else if (qos == subscription->curQos) {
        celix_earpm_qos_e maxQos = CELIX_EARPM_QOS_UNKNOWN;
        for (int i = 0; i < size; ++i) {
            long serviceId = celix_arrayList_getLong(subscription->handlerServiceIdList, i);
            celix_earpm_event_handler_t* handler = celix_longHashMap_get(earpm->eventHandlers, serviceId);
            if (handler != NULL && handler->qos > maxQos) {
                maxQos = handler->qos;
            }
        }
        if (maxQos != subscription->curQos) {
            celix_status_t status = celix_earpmClient_subscribe(earpm->mqttClient, topic, maxQos);
            if (status != CELIX_SUCCESS) {
                celix_logHelper_error(earpm->logHelper, "Failed to subscribe %s with qos %d.", topic, (int)maxQos);
                return;
            }
            subscription->curQos = maxQos;
        }
    }
    return;
}

static void celix_earpm_subOrUnsubRemoteEventsForHandler(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_event_handler_t* handler, bool subscribe) {
    void (*action)(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, celix_earpm_qos_e qos, long handlerServiceId) = celix_earpm_unsubscribeEvent;
    if (subscribe) {
        action = celix_earpm_subscribeEvent;
    }

    int size = celix_arrayList_size(handler->topics);
    for (int i = 0; i < size; ++i) {
        const char* topic = celix_arrayList_getString(handler->topics, i);
        assert(topic != NULL);
        action(earpm, topic, handler->qos, handler->serviceId);
    }
    return;
}

static json_t* celix_earpm_genHandlerInformation(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_event_handler_t* handler) {
    json_auto_t* topics = json_array();
    if (topics == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to create topic json array.");
        return NULL;
    }
    int size = celix_arrayList_size(handler->topics);
    for (int i = 0; i < size; ++i) {
        const char* topic = celix_arrayList_getString(handler->topics, i);
        assert(topic != NULL);
        if (json_array_append_new(topics, json_string(topic)) != 0) {
            celix_logHelper_error(earpm->logHelper, "Failed to add topic to json array.");
            return NULL;
        }
    }
    json_error_t jsonError;
    memset(&jsonError, 0, sizeof(jsonError));
    json_auto_t* handlerInfo = json_pack_ex(&jsonError, 0, "{sOsi}", "topics", topics, "handlerId", handler->serviceId);
    if (handlerInfo == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to pack handler information. %s", jsonError.text);
        return NULL;
    }
    if (handler->filter != NULL) {
        if (json_object_set_new(handlerInfo, "filter", json_string(handler->filter)) != 0) {
            celix_logHelper_error(earpm->logHelper, "Failed to add filter to handler information.");
            return NULL;
        }
    }

    return json_incref(handlerInfo);
}

static void celix_earpm_addHandlerInfoToRemote(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_event_handler_t* handler) {
    const char* topic = CELIX_EARPM_HANDLER_INFO_ADD_TOPIC;
    json_auto_t* root = json_object();
    if (root == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to create adding handler info message payload.");
        return;
    }
    json_t* handlerInfo = celix_earpm_genHandlerInformation(earpm, handler);
    if (handlerInfo == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to create handler information for handler %li.", handler->serviceId);
        return;
    }
    if (json_object_set_new(root, "handler", handlerInfo) != 0) {
        celix_logHelper_error(earpm->logHelper, "Failed to add handler information to adding handler information message.");
        return;
    }

    celix_autofree char* payload = json_dumps(root, JSON_COMPACT | JSON_ENCODE_ANY);
    if (payload == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to dump adding handler information message payload for handler %li.", handler->serviceId);
        return;
    }
    //If the mqtt connection is disconnected, we will resend the handler information
    // when the connection is re-established in celix_earpm_connectedCallback,
    // so we use CELIX_EARPM_QOS_AT_MOST_ONCE qos here.
    celix_earpm_client_request_info_t requestInfo;
    memset(&requestInfo, 0, sizeof(requestInfo));
    requestInfo.topic = topic;
    requestInfo.payload = payload;
    requestInfo.payloadSize = strlen(payload);
    requestInfo.qos = CELIX_EARPM_QOS_AT_MOST_ONCE;
    requestInfo.pri = CELIX_EARPM_MSG_PRI_HIGH;
    requestInfo.senderUUID = earpm->fwUUID;
    requestInfo.version = CELIX_EARPM_MSG_VERSION;
    celix_status_t status = celix_earpmClient_publishAsync(earpm->mqttClient, &requestInfo);
    if (status != CELIX_SUCCESS && status != ENOTCONN) {
        celix_logHelper_error(earpm->logHelper, "Failed to publish %s. payload:%s. %d.", topic, payload, status);
    }
    return;
}

static void celix_earpm_removeHandlerInfoFromRemote(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_event_handler_t* handler) {
    const char* topic = CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC;
    json_auto_t* root = json_object();
    if (root == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to create removing handler info message payload.");
        return;
    }
    if (json_object_set_new(root, "handlerId", json_integer(handler->serviceId)) != 0) {
        celix_logHelper_error(earpm->logHelper, "Failed to add handler id to removing handler info message.");
        return;
    }
    celix_autofree char* payload = json_dumps(root, JSON_COMPACT | JSON_ENCODE_ANY);
    if (payload == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to dump removing handler information message payload for handler %li.", handler->serviceId);
        return;
    }
    //If the mqtt connection is disconnected, we will resend the handler information
    // when the connection is re-established in celix_earpm_connectedCallback,
    // so we use CELIX_EARPM_QOS_AT_MOST_ONCE qos here.
    celix_earpm_client_request_info_t requestInfo;
    memset(&requestInfo, 0, sizeof(requestInfo));
    requestInfo.topic = topic;
    requestInfo.payload = payload;
    requestInfo.payloadSize = strlen(payload);
    requestInfo.qos = CELIX_EARPM_QOS_AT_MOST_ONCE;
    requestInfo.pri = CELIX_EARPM_MSG_PRI_HIGH;
    requestInfo.senderUUID = earpm->fwUUID;
    requestInfo.version = CELIX_EARPM_MSG_VERSION;
    celix_status_t status = celix_earpmClient_publishAsync(earpm->mqttClient, &requestInfo);
    if (status != CELIX_SUCCESS && status != ENOTCONN) {
        celix_logHelper_error(earpm->logHelper, "Failed to publish %s. payload:%s. %d.", topic, payload, status);
    }
    return;
}

celix_status_t celix_earpm_addEventHandlerService(void* handle , void* service CELIX_UNUSED, const celix_properties_t* properties) {
    assert(handle != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;

    celix_autoptr(celix_earpm_event_handler_t) handler = celix_earpm_createEventHandler(earpm, properties);
    if (handler == NULL) {
        return CELIX_SERVICE_EXCEPTION;
    }

    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    celix_status_t status = celix_longHashMap_put(earpm->eventHandlers, handler->serviceId, handler);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(earpm->logHelper, "Failed to add event handler service, %d.", status);
        return status;
    }

    celix_earpm_subOrUnsubRemoteEventsForHandler(earpm, handler, true);
    celix_earpm_addHandlerInfoToRemote(earpm, handler);

    celix_steal_ptr(handler);
    return CELIX_SUCCESS;
}

celix_status_t celix_earpm_removeEventHandlerService(void* handle , void* service CELIX_UNUSED, const celix_properties_t* properties) {
    assert(handle != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;

    long serviceId = celix_properties_getAsLong(properties, CELIX_FRAMEWORK_SERVICE_ID, -1);
    if (serviceId < 0 ) {
        celix_logHelper_error(earpm->logHelper, "Failed to remove event handler service, no service id found.");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    celix_earpm_event_handler_t* handler = (celix_earpm_event_handler_t*)celix_longHashMap_get(earpm->eventHandlers, serviceId);
    if (handler == NULL) {
        celix_logHelper_debug(earpm->logHelper, "No handler found for service id %li.", serviceId);
        return CELIX_SUCCESS;
    }

    celix_earpm_removeHandlerInfoFromRemote(earpm, handler);
    celix_earpm_subOrUnsubRemoteEventsForHandler(earpm, handler, false);

    celix_longHashMap_remove(earpm->eventHandlers, serviceId);

    return CELIX_SUCCESS;
}

celix_status_t celix_earpm_setEventAdminSvc(void* handle, void* eventAdminSvc) {
    assert(handle != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;
    return celix_earpmDeliverer_setEventAdminSvc(earpm->deliverer, (celix_event_admin_service_t *) eventAdminSvc);
}

static bool celix_event_matchRemoteHandler(const char* topic, const celix_properties_t* eventProps, const celix_earpm_remote_handler_info_t* info) {
    assert(info->topics != NULL);

    if (!celix_filter_match(info->filter, eventProps)) {
        return false;
    }
    int size = celix_arrayList_size(info->topics);
    for (int i = 0; i < size; ++i) {
        const char* topicPattern = celix_arrayList_getString(info->topics, i);
        if (celix_utils_stringEquals(topicPattern, "*")) {
            return true;
        }
        if (celix_utils_stringEquals(topicPattern, topic)) {
            return true;
        }
        if (topicPattern[strlen(topicPattern) - 1] == '*') {
            if (strncmp(topicPattern, topic, strlen(topicPattern) - 1) == 0) {
                return true;
            }
        }
    }
    return false;
}

static bool celix_earpm_filterEvent(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const celix_properties_t* eventProps) {
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    CELIX_STRING_HASH_MAP_ITERATE(earpm->remoteFrameworks, iter) {
        celix_earpm_remote_framework_info_t* fwInfo = iter.value.ptrValue;
        CELIX_LONG_HASH_MAP_ITERATE(fwInfo->handlerInfoMap, handlerIter) {
            celix_earpm_remote_handler_info_t* info = handlerIter.value.ptrValue;
            if (celix_event_matchRemoteHandler(topic, eventProps, info)) {
                return false;
            }
        }
    }
    return true;
}

static celix_status_t celix_earpm_publishEventAsync(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize, const celix_properties_t* eventProps, celix_earpm_qos_e qos) {
    celix_earpm_client_request_info_t requestInfo;
    memset(&requestInfo, 0, sizeof(requestInfo));
    requestInfo.topic = topic;
    requestInfo.payload = payload;
    requestInfo.payloadSize = payloadSize;
    requestInfo.qos = qos;
    requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
    requestInfo.expiryInterval = (int32_t)celix_properties_getAsLong(eventProps, CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, -1);
    requestInfo.version = CELIX_EARPM_MSG_VERSION;
    celix_status_t status = celix_earpmClient_publishAsync(earpm->mqttClient, &requestInfo);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to publish async event %s with qos %d. %d.", topic, (int)qos, status);
        return status;
    }
    return CELIX_SUCCESS;
}

static void celix_earpm_clearAckSeqNr(celix_event_admin_remote_provider_mqtt_t* earpm, long ackSeqNr) {
    CELIX_STRING_HASH_MAP_ITERATE(earpm->remoteFrameworks, iter) {
        celix_earpm_remote_framework_info_t* fwInfo = (celix_earpm_remote_framework_info_t*)iter.value.ptrValue;
        celix_longHashMap_remove(fwInfo->eventAckSeqNrMap, ackSeqNr);
    }
}

static celix_status_t celix_earpm_associateAckSeqNrWithRemoteHandler(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const celix_properties_t* eventProps, long ackSeqNr) {
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    CELIX_STRING_HASH_MAP_ITERATE(earpm->remoteFrameworks, iter) {
        celix_earpm_remote_framework_info_t* fwInfo = (celix_earpm_remote_framework_info_t*)iter.value.ptrValue;
        if (fwInfo->continuousNoAckCount > earpm->continuousNoAckThreshold) {
            celix_logHelper_warning(earpm->logHelper, "Continuous ack timeout for remote framework %s. No waiting for the ack of event %s.", iter.key, topic);
            continue;
        }
        celix_autoptr(celix_array_list_t) matchedHandlerServiceIdList = celix_arrayList_createLongArray();
        if (matchedHandlerServiceIdList == NULL) {
            celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(earpm->logHelper, "Failed to create matched handler service id list.");
            celix_earpm_clearAckSeqNr(earpm, ackSeqNr);
            return ENOMEM;
        }
        CELIX_LONG_HASH_MAP_ITERATE(fwInfo->handlerInfoMap, handlerIter) {
            celix_earpm_remote_handler_info_t* info = handlerIter.value.ptrValue;
            if (celix_event_matchRemoteHandler(topic, eventProps, info)) {
                celix_status_t status = celix_arrayList_addLong(matchedHandlerServiceIdList, handlerIter.key);
                if (status != CELIX_SUCCESS) {
                    celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
                    celix_logHelper_error(earpm->logHelper, "Failed to add matched handler service id to list.");
                    celix_earpm_clearAckSeqNr(earpm, ackSeqNr);
                    return status;
                }
            }
        }
        celix_status_t status = celix_longHashMap_put(fwInfo->eventAckSeqNrMap, ackSeqNr, matchedHandlerServiceIdList);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(earpm->logHelper, "Error adding event(%s,%ld) to list.", topic, ackSeqNr);
            celix_earpm_clearAckSeqNr(earpm, ackSeqNr);
            return status;
        }
        celix_steal_ptr(matchedHandlerServiceIdList);
    }
    return CELIX_SUCCESS;
}

static bool celix_earpm_hasPendingAckFor(celix_event_admin_remote_provider_mqtt_t* earpm, long ackSeqNr) {
    celix_array_list_entry_t listEntry;
    memset(&listEntry, 0, sizeof(listEntry));
    CELIX_STRING_HASH_MAP_ITERATE(earpm->remoteFrameworks, iter) {
        celix_earpm_remote_framework_info_t* fwInfo = (celix_earpm_remote_framework_info_t*)iter.value.ptrValue;
        if (celix_longHashMap_hasKey(fwInfo->eventAckSeqNrMap, ackSeqNr)) {
            return true;
        }
    }
    return false;
}

static void celix_earpm_handleNoAckEvent(celix_event_admin_remote_provider_mqtt_t* earpm, long ackSeqNr) {
    CELIX_STRING_HASH_MAP_ITERATE(earpm->remoteFrameworks, iter) {
        celix_earpm_remote_framework_info_t* fwInfo = (celix_earpm_remote_framework_info_t*)iter.value.ptrValue;
        if (celix_longHashMap_hasKey(fwInfo->eventAckSeqNrMap, ackSeqNr)) {
            fwInfo->continuousNoAckCount++;
        }
    }
    return;
}

static celix_status_t celix_earpm_waitForEventAck(celix_event_admin_remote_provider_mqtt_t* earpm, long ackSeqNr, const struct timespec* expiryTime) {
    celix_status_t status = CELIX_SUCCESS;
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    while (celix_earpm_hasPendingAckFor(earpm, ackSeqNr)) {
        status = celixThreadCondition_waitUntil(&earpm->ackCond, &earpm->mutex, expiryTime);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(earpm->logHelper, "Failed to wait for ack of event %ld.", ackSeqNr);
            if (status == CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO, ETIMEDOUT)) {
                celix_earpm_handleNoAckEvent(earpm, ackSeqNr);
            }
            celix_earpm_clearAckSeqNr(earpm, ackSeqNr);
            return status;
        }
    }
    return CELIX_SUCCESS;
}

static celix_status_t celix_earpm_publishEventSync(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize, const celix_properties_t* eventProps, celix_earpm_qos_e qos) {
    long ackSeqNr = __atomic_add_fetch(&earpm->lastAckSeqNr, 1, __ATOMIC_RELAXED);

    celix_status_t status = celix_earpm_associateAckSeqNrWithRemoteHandler(earpm, topic, eventProps, ackSeqNr);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to associate ack seq nr with remote handler for %s. %d.", topic, status);
        return status;
    }
    int32_t expiryInterval = (int32_t)celix_properties_getAsLong(eventProps, CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, CELIX_EARPM_SYNC_EVENT_TIMEOUT_DEFAULT);
    celix_earpm_client_request_info_t requestInfo;
    memset(&requestInfo, 0, sizeof(requestInfo));
    requestInfo.topic = (char*)topic;
    requestInfo.payload = (char*)payload;
    requestInfo.payloadSize = payloadSize;
    requestInfo.qos = qos;
    requestInfo.pri = CELIX_EARPM_MSG_PRI_LOW;
    requestInfo.expiryInterval = expiryInterval;
    requestInfo.version = CELIX_EARPM_MSG_VERSION;
    requestInfo.responseTopic = earpm->syncEventAckTopic;
    struct celix_earpm_sync_event_correlation_data correlationData;
    memset(&correlationData, 0, sizeof(correlationData));
    correlationData.ackSeqNr = ackSeqNr;
    requestInfo.correlationData = &correlationData;
    requestInfo.correlationDataSize = sizeof(correlationData);

    expiryInterval = (expiryInterval > 0 && expiryInterval < (INT32_MAX/2)) ? expiryInterval : (INT32_MAX/2);
    struct timespec expiryTime = celixThreadCondition_getDelayedTime(expiryInterval);

    status = celix_earpmClient_publishSync(earpm->mqttClient, &requestInfo);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to publish sync event %s with qos %d. %d.", topic, (int)qos, status);
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
        celix_earpm_clearAckSeqNr(earpm, ackSeqNr);
        return status;
    }

    return celix_earpm_waitForEventAck(earpm, ackSeqNr, &expiryTime);
}

static celix_status_t celix_earpm_publishEvent(celix_event_admin_remote_provider_mqtt_t* earpm, const char *topic,
                                               const celix_properties_t *eventProps, bool async) {
    celix_earpm_qos_e qos = (celix_earpm_qos_e)celix_properties_getAsLong(eventProps, CELIX_EVENT_REMOTE_QOS, earpm->defaultQos);
    if (qos <= CELIX_EARPM_QOS_UNKNOWN || qos >= CELIX_EARPM_QOS_MAX) {
        celix_logHelper_error(earpm->logHelper, "Qos(%d) is invalid for event %s.", qos, topic);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    if (celix_earpm_filterEvent(earpm, topic, eventProps)) {
        celix_logHelper_trace(earpm->logHelper, "No remote handler subscribe %s", topic);
        return CELIX_SUCCESS;
    }
    celix_autofree char* payload = NULL;
    size_t payloadSize = 0;
    if (eventProps != NULL) {
        celix_status_t status = celix_properties_saveToString(eventProps, 0, &payload);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(earpm->logHelper, "Failed to dump event properties to string for %s. %d.", topic, status);
            return status;
        }
        payloadSize = strlen(payload);
    }
    if (async) {
        return celix_earpm_publishEventAsync(earpm, topic, payload, payloadSize, eventProps, qos);
    }
    return celix_earpm_publishEventSync(earpm, topic, payload, payloadSize, eventProps, qos);
}

celix_status_t celix_earpm_postEvent(void* handle , const char *topic, const celix_properties_t *eventProps) {
    if (handle == NULL || topic == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;

    celix_logHelper_trace(earpm->logHelper, "Post event %s", topic);

    return celix_earpm_publishEvent(earpm, topic, eventProps, true);
}

celix_status_t celix_earpm_sendEvent(void* handle, const char *topic, const celix_properties_t *eventProps) {
    if (handle == NULL || topic == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;

    celix_logHelper_trace(earpm->logHelper, "Send event %s.", topic);

    return celix_earpm_publishEvent(earpm, topic, eventProps, false);
}

static void celix_earpm_processSyncEventAckMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_client_request_info_t* requestInfo) {
    if (requestInfo->correlationData == NULL
        || requestInfo->correlationDataSize != sizeof(struct celix_earpm_sync_event_correlation_data)) {
        celix_logHelper_error(earpm->logHelper, "Correlation data size is invalid for %s message.", requestInfo->topic);
        return;
    }

    bool wakeupWaitAckThread = false;
    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
        celix_earpm_remote_framework_info_t* fwInfo = celix_stringHashMap_get(earpm->remoteFrameworks, requestInfo->senderUUID);
        if (fwInfo == NULL) {
            celix_logHelper_debug(earpm->logHelper, "No remote framework info for %s.", requestInfo->senderUUID);
            return;
        }
        struct celix_earpm_sync_event_correlation_data data;
        memcpy(&data, requestInfo->correlationData, sizeof(data));
        if (celix_longHashMap_remove(fwInfo->eventAckSeqNrMap, data.ackSeqNr)) {
            wakeupWaitAckThread = true;
        }
    }
    if (wakeupWaitAckThread) {
        celixThreadCondition_broadcast(&earpm->ackCond);
    }
    return;
}

static celix_earpm_remote_handler_info_t* celix_earpm_remoteHandlerInfoCreate(const json_t* topicsJson, const char* filter, celix_log_helper_t* logHelper) {
    celix_autofree celix_earpm_remote_handler_info_t* info = calloc(1, sizeof(*info));
    if (info == NULL) {
        celix_logHelper_error(logHelper, "Failed to allocate memory for handler information.");
        return NULL;
    }
    celix_autoptr(celix_array_list_t) topics = info->topics = celix_arrayList_createStringArray();
    if (topics == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Failed to create topics list for handler information.");
        return NULL;
    }

    size_t index;
    json_t* value;
    json_array_foreach(topicsJson, index, value) {
        const char* topic = json_string_value(value);
        if (topic == NULL) {
            celix_logHelper_error(logHelper, "Topic is not string.");
            return NULL;
        }
        if (celix_arrayList_addString(topics, (void*)topic)) {
            celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_WARNING);
            celix_logHelper_warning(logHelper, "Failed to add topic(%s) to handler information.", topic);
        }
    }
    if (filter !=  NULL) {
        info->filter = celix_filter_create(filter);//If return NULL, then only let event admin does event filter
        if (info->filter == NULL) {
            celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_WARNING);
            celix_logHelper_warning(logHelper, "Failed to create filter(%s) for handler information.", filter);
        }
    }

    celix_steal_ptr(topics);

    return celix_steal_ptr(info);
}

static void celix_earpm_remoteHandlerInfoDestroy(celix_earpm_remote_handler_info_t* info) {
    celix_arrayList_destroy(info->topics);
    celix_filter_destroy(info->filter);
    free(info);
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_remote_handler_info_t, celix_earpm_remoteHandlerInfoDestroy)


static celix_earpm_remote_framework_info_t* celix_earpm_createAndAddRemoteFrameworkInfo(celix_event_admin_remote_provider_mqtt_t* earpm,
                                                                                        const char* remoteFwUUID) {
    celix_autofree celix_earpm_remote_framework_info_t* fwInfo = calloc(1, sizeof(*fwInfo));
    if (fwInfo == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to allocate memory for remote framework %s.", remoteFwUUID);
        return NULL;
    }
    fwInfo->continuousNoAckCount = 0;
    celix_autoptr(celix_long_hash_map_t) eventAckSeqNrMap = NULL;
    {
        celix_long_hash_map_create_options_t opts = CELIX_EMPTY_LONG_HASH_MAP_CREATE_OPTIONS;
        opts.simpleRemovedCallback = (void*) celix_arrayList_destroy;
        eventAckSeqNrMap = fwInfo->eventAckSeqNrMap = celix_longHashMap_createWithOptions(&opts);
        if (eventAckSeqNrMap == NULL) {
            celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(earpm->logHelper, "Failed to create event ack seq number map for remote framework %s.", remoteFwUUID);
            return NULL;
        }
    }
    celix_autoptr(celix_long_hash_map_t) handlerInfoMap = NULL;
    {
        celix_long_hash_map_create_options_t opts = CELIX_EMPTY_LONG_HASH_MAP_CREATE_OPTIONS;
        opts.simpleRemovedCallback = (void *) celix_earpm_remoteHandlerInfoDestroy;
        handlerInfoMap = fwInfo->handlerInfoMap = celix_longHashMap_createWithOptions(&opts);
        if (handlerInfoMap == NULL) {
            celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(earpm->logHelper, "Failed to create handler information map for remote framework %s.", remoteFwUUID);
            return NULL;
        }
    }

    if (celix_stringHashMap_put(earpm->remoteFrameworks, remoteFwUUID, fwInfo) != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(earpm->logHelper, "Failed to add remote framework info for %s.", remoteFwUUID);
        return NULL;
    }
    celix_steal_ptr(handlerInfoMap);
    celix_steal_ptr(eventAckSeqNrMap);
    return celix_steal_ptr(fwInfo);
}

static void celix_earpm_remoteFrameworkInfoDestroy(celix_earpm_remote_framework_info_t* info) {
    celix_longHashMap_destroy(info->handlerInfoMap);
    celix_longHashMap_destroy(info->eventAckSeqNrMap);
    free(info);
    return;
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(celix_earpm_remote_framework_info_t, celix_earpm_remoteFrameworkInfoDestroy)

static celix_status_t celix_earpm_addRemoteHandlerInfo(celix_event_admin_remote_provider_mqtt_t* earpm, const json_t* handlerInfoJson, celix_earpm_remote_framework_info_t* fwInfo) {
    json_t* id = json_object_get(handlerInfoJson, "handlerId");
    if (!json_is_integer(id)) {
        celix_logHelper_error(earpm->logHelper, "Handler id is lost or not integer.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    long handlerServiceId = json_integer_value(id);
    if (handlerServiceId < 0) {
        celix_logHelper_error(earpm->logHelper, "Handler id(%ld) is invalid.", handlerServiceId);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    json_t* topics = json_object_get(handlerInfoJson, "topics");
    if (!json_is_array(topics)) {
        celix_logHelper_error(earpm->logHelper, "Topics is lost or not array.");
        return CELIX_ILLEGAL_ARGUMENT;
    }
    const char* filter = json_string_value(json_object_get(handlerInfoJson, "filter"));
    celix_autoptr(celix_earpm_remote_handler_info_t) handlerInfo = celix_earpm_remoteHandlerInfoCreate(topics, filter, earpm->logHelper);
    if (handlerInfo == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to create remote handler information.");
        return ENOMEM;
    }
    celix_status_t status = celix_longHashMap_put(fwInfo->handlerInfoMap, handlerServiceId, handlerInfo);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(earpm->logHelper, "Failed to add remote handler information.");
        return status;
    }
    celix_steal_ptr(handlerInfo);
    return CELIX_SUCCESS;
}

static void celix_earpm_processHandlerInfoAddMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize, const char* remoteFwUUID) {
    json_error_t error;
    json_auto_t* root = json_loadb(payload, payloadSize, 0, &error);
    if (root == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to parse message %s which from %s. %s.", topic, remoteFwUUID,
                              error.text);
        return;
    }
    json_t* handlerInfo = json_object_get(root, "handler");
    if (!json_is_object(handlerInfo)) {
        celix_logHelper_error(earpm->logHelper, "No handler information for %s which from %s.", topic, remoteFwUUID);
        return;
    }
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    celix_earpm_remote_framework_info_t* fwInfo = celix_stringHashMap_get(earpm->remoteFrameworks, remoteFwUUID);
    if (fwInfo == NULL) {
        fwInfo = celix_earpm_createAndAddRemoteFrameworkInfo(earpm, remoteFwUUID);
        if (fwInfo == NULL) {
            celix_logHelper_error(earpm->logHelper, "Failed to create remote framework info for %s.", remoteFwUUID);
            return;
        }
    }
    celix_status_t status = celix_earpm_addRemoteHandlerInfo(earpm, handlerInfo, fwInfo);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to add remote handler information with %s which from %s.", topic, remoteFwUUID);
        return;
    }
    return;
}

static bool celix_earpm_disassociateRemoteHandlerWithPendingAck(celix_earpm_remote_framework_info_t *fwInfo, long removedHandlerServiceId) {
    bool pendingAckExpire = false;
    celix_long_hash_map_iterator_t iter = celix_longHashMap_begin(fwInfo->eventAckSeqNrMap);
    while (!celix_longHashMapIterator_isEnd(&iter)) {
        celix_array_list_t* handlerServiceIdList = iter.value.ptrValue;
        if (removedHandlerServiceId >= 0) {
            celix_arrayList_removeLong(handlerServiceIdList, removedHandlerServiceId);
        } else {
            for(int i = celix_arrayList_size(handlerServiceIdList); i > 0; --i) {
                long handlerServiceId = celix_arrayList_getLong(handlerServiceIdList, i - 1);
                if (!celix_longHashMap_hasKey(fwInfo->handlerInfoMap, handlerServiceId)) {
                    celix_arrayList_removeAt(handlerServiceIdList, i - 1);
                }
            }
        }
        if (celix_arrayList_size(handlerServiceIdList) == 0) {
            celix_longHashMapIterator_remove(&iter);
            pendingAckExpire = true;
        } else {
            celix_longHashMapIterator_next(&iter);
        }
    }
    return pendingAckExpire;
}

static void celix_earpm_processHandlerInfoRemoveMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize, const char* remoteFwUUID) {
    json_error_t error;
    json_auto_t* root = json_loadb(payload, payloadSize, 0, &error);
    if (root == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to parse message %s which from %s. %s.", topic, remoteFwUUID, error.text);
        return;
    }
    json_t* id = json_object_get(root, "handlerId");
    if (!json_is_integer(id)) {
        celix_logHelper_error(earpm->logHelper, "Handler id is lost or not integer for topic %s which from %s.", topic, remoteFwUUID);
        return;
    }
    long handlerServiceId = json_integer_value(id);
    if (handlerServiceId < 0) {
        celix_logHelper_error(earpm->logHelper, "Handler id(%ld) is invalid for topic %s which from %s.", handlerServiceId, topic, remoteFwUUID);
        return;
    }
    bool wakeupWaitAckThread = false;
    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
        celix_earpm_remote_framework_info_t *fwInfo = celix_stringHashMap_get(earpm->remoteFrameworks, remoteFwUUID);
        if (fwInfo == NULL) {
            celix_logHelper_warning(earpm->logHelper, "No remote framework info for %s.", remoteFwUUID);
            return;
        }
        celix_longHashMap_remove(fwInfo->handlerInfoMap, handlerServiceId);
        if (celix_longHashMap_size(fwInfo->handlerInfoMap) == 0) {
            celix_stringHashMap_remove(earpm->remoteFrameworks, remoteFwUUID);
            wakeupWaitAckThread = true;
        } else if (celix_earpm_disassociateRemoteHandlerWithPendingAck(fwInfo, handlerServiceId)){
            wakeupWaitAckThread = true;
        }
    }
    if (wakeupWaitAckThread) {
        celixThreadCondition_broadcast(&earpm->ackCond);
    }

    return;
}

static void celix_earpm_processHandlerInfoUpdateMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize, const char* remoteFwUUID) {
    json_error_t error;
    json_auto_t* root = json_loadb(payload, payloadSize, 0, &error);
    if (root == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to parse message %s. %s.", topic, error.text);
        return;
    }
    json_t* handlers = json_object_get(root, "handlers");
    if (!json_is_array(handlers)) {
        celix_logHelper_error(earpm->logHelper, "No handler information for %s which from %s.", topic, remoteFwUUID);
        return;
    }
    bool wakeupWaitAckThread = false;
    size_t handlerSize = json_array_size(handlers);
    if (handlerSize == 0) {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
        wakeupWaitAckThread = celix_stringHashMap_remove(earpm->remoteFrameworks, remoteFwUUID);
    } else {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
        bool handlerInfoRemoved = false;
        celix_earpm_remote_framework_info_t* fwInfo = celix_stringHashMap_get(earpm->remoteFrameworks, remoteFwUUID);
        if (fwInfo == NULL) {
            fwInfo = celix_earpm_createAndAddRemoteFrameworkInfo(earpm, remoteFwUUID);
            if (fwInfo == NULL) {
                celix_logHelper_error(earpm->logHelper, "Failed to create remote framework info for %s.", remoteFwUUID);
                return;
            }
        } else {
            celix_longHashMap_clear(fwInfo->handlerInfoMap);
            handlerInfoRemoved = true;
        }
        for (size_t i = 0; i < handlerSize; ++i) {
            json_t* handlerDesc = json_array_get(handlers, i);
            celix_status_t status = celix_earpm_addRemoteHandlerInfo(earpm, handlerDesc, fwInfo);
            if (status != CELIX_SUCCESS) {
                celix_logHelper_error(earpm->logHelper, "Failed to add handler information with %s.", topic);
                continue;
            }
        }
        if (celix_longHashMap_size(fwInfo->handlerInfoMap) == 0) {
            celix_stringHashMap_remove(earpm->remoteFrameworks, remoteFwUUID);
            wakeupWaitAckThread = handlerInfoRemoved;
        } else if (handlerInfoRemoved && celix_earpm_disassociateRemoteHandlerWithPendingAck(fwInfo, -1/*check all handlers*/)) {
            wakeupWaitAckThread = true;
        }
    }
    if (wakeupWaitAckThread) {
        celixThreadCondition_broadcast(&earpm->ackCond);
    }
    return;
}

static void celix_earpm_refreshAllHandlerInfoToRemote(celix_event_admin_remote_provider_mqtt_t* earpm) {
    const char* topic = CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC;
    json_auto_t* root = json_object();
    if (root == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to create updating handlers info message payload.");
        return;
    }
    json_t* handlers = json_array();
    if (handlers == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to create handler information array.");
        return;
    }
    if (json_object_set_new(root, "handlers", handlers) != 0) {
        celix_logHelper_error(earpm->logHelper, "Failed to add handlers information to updating handler info message.");
        return;
    }

    //The mutex scope must end after celix_earpmc_publishAsync is called,
    // otherwise it will cause the conflict between CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC and CELIX_EARPM_HANDLER_INFO_ADD/REMOVE_TOPIC
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    CELIX_LONG_HASH_MAP_ITERATE(earpm->eventHandlers, iter) {
        celix_earpm_event_handler_t* handler = iter.value.ptrValue;
        json_t* handlerInfo = celix_earpm_genHandlerInformation(earpm, handler);
        if (handlerInfo == NULL) {
            celix_logHelper_error(earpm->logHelper, "Failed to create handler information for handler %li.", handler->serviceId);
            continue;
        }
        if (json_array_append_new(handlers, handlerInfo) != 0) {
            celix_logHelper_error(earpm->logHelper, "Failed to add information of handler %li.", handler->serviceId);
            continue;
        }
    }
    celix_autofree char* payload = json_dumps(root, JSON_COMPACT | JSON_ENCODE_ANY);
    if (payload == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to dump updating handler information message payload.");
        return;
    }
    //If the mqtt connection is disconnected, we will resend the handler information
    // when the connection is re-established in celix_earpm_connectedCallback,
    // so we use CELIX_EARPM_QOS_AT_MOST_ONCE qos here.
    celix_earpm_client_request_info_t requestInfo;
    memset(&requestInfo, 0, sizeof(requestInfo));
    requestInfo.topic = topic;
    requestInfo.payload = payload;
    requestInfo.payloadSize = strlen(payload);
    requestInfo.qos = CELIX_EARPM_QOS_AT_MOST_ONCE;
    requestInfo.pri = CELIX_EARPM_MSG_PRI_HIGH;
    requestInfo.senderUUID = earpm->fwUUID;
    requestInfo.version = CELIX_EARPM_MSG_VERSION;
    celix_status_t status = celix_earpmClient_publishAsync(earpm->mqttClient, &requestInfo);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to publish %s. payload:%s.", topic, payload);
    }

    return;
}

static void celix_earpm_processHandlerInfoMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_client_request_info_t* requestInfo) {
    const char* action = requestInfo->topic + sizeof(CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX) - 1;
    if (strcmp(action, "add") == 0) {
        celix_earpm_processHandlerInfoAddMessage(earpm, requestInfo->topic, requestInfo->payload,
                                                 requestInfo->payloadSize, requestInfo->senderUUID);
    } else if (strcmp(action, "remove") == 0) {
        celix_earpm_processHandlerInfoRemoveMessage(earpm, requestInfo->topic, requestInfo->payload,
                                                    requestInfo->payloadSize, requestInfo->senderUUID);
    } else if (strcmp(action, "update") == 0) {
        celix_earpm_processHandlerInfoUpdateMessage(earpm, requestInfo->topic, requestInfo->payload,
                                                    requestInfo->payloadSize, requestInfo->senderUUID);
    } else if (strcmp(action, "query") == 0) {
        celix_earpm_refreshAllHandlerInfoToRemote(earpm);
    } else {
        celix_logHelper_warning(earpm->logHelper, "Unknown action %s for handler information message.", requestInfo->topic);
    }
    return;
}

static void celix_earpm_processSessionEndMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_client_request_info_t* requestInfo) {
    bool wakeupWaitAckThread = false;
    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
        wakeupWaitAckThread = celix_stringHashMap_remove(earpm->remoteFrameworks, requestInfo->senderUUID);
    }
    if(wakeupWaitAckThread) {
        celixThreadCondition_broadcast(&earpm->ackCond);
    }
    return;
}

static void celix_earpm_markRemoteFrameworkActive(celix_event_admin_remote_provider_mqtt_t* earpm, const char* fwUUID) {
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    celix_earpm_remote_framework_info_t* fwInfo = celix_stringHashMap_get(earpm->remoteFrameworks, fwUUID);
    if (fwInfo != NULL) {
        fwInfo->continuousNoAckCount = 0;
    }
    return;
}

static void celix_earpm_processControlMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_client_request_info_t* requestInfo) {
    const char* topic = requestInfo->topic;
    if (requestInfo->senderUUID == NULL) {
        celix_logHelper_error(earpm->logHelper, "No sender UUID for control message %s.", topic);
        return;
    }
    if (celix_utils_stringEquals(topic, earpm->syncEventAckTopic)) {
        celix_earpm_processSyncEventAckMessage(earpm, requestInfo);
    } else if (strncmp(topic, CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX, sizeof(CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX) - 1) == 0) {
        celix_earpm_processHandlerInfoMessage(earpm, requestInfo);
    } else if (celix_utils_stringEquals(topic, CELIX_EARPM_SESSION_END_TOPIC)) {
        celix_earpm_processSessionEndMessage(earpm, requestInfo);
    }

    celix_earpm_markRemoteFrameworkActive(earpm, requestInfo->senderUUID);
    return;
}

static celix_status_t celix_earpm_sendSyncEventAck(celix_event_admin_remote_provider_mqtt_t* earpm, const char* ackTopic, const void* correlationData, size_t correlationDataSize) {
    celix_earpm_client_request_info_t ack;
    memset(&ack, 0, sizeof(ack));
    ack.topic = ackTopic;
    ack.correlationData = correlationData;
    ack.correlationDataSize = correlationDataSize;
    ack.qos = CELIX_EARPM_QOS_AT_LEAST_ONCE;
    ack.pri = CELIX_EARPM_MSG_PRI_MIDDLE;
    ack.senderUUID = earpm->fwUUID;
    ack.version = CELIX_EARPM_MSG_VERSION;
    return celix_earpmClient_publishAsync(earpm->mqttClient, &ack);
}

static void celix_earpm_sendEventDone(void* data, const char* topic, celix_status_t rc CELIX_UNUSED) {
    assert(data != NULL);
    assert(topic != NULL);
    struct celix_earpm_send_event_done_callback_data* callData = data;
    celix_event_admin_remote_provider_mqtt_t* earpm = callData->earpm;
    assert(earpm != NULL);
    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
        if (!earpm->destroying) {//The mqtt client may be invalid when the earpm is destroying
            (void)celix_earpm_sendSyncEventAck(earpm, callData->responseTopic, callData->correlationData, callData->correlationDataSize);
        }
    }
    free(callData->correlationData);
    free(callData->responseTopic);
    free(callData);
    return;
}

static celix_status_t celix_earpm_deliverSyncEvent(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_client_request_info_t* requestInfo) {
    celix_autoptr(celix_properties_t) eventProps = NULL;
    if (requestInfo->payload != NULL && requestInfo->payloadSize > 0) {
        celix_status_t status = celix_properties_loadFromString(requestInfo->payload, 0, &eventProps);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(earpm->logHelper, "Failed to load event properties for %s.", requestInfo->topic);
            return status;
        }
    }
    celix_autofree struct celix_earpm_send_event_done_callback_data* sendDoneCbData = calloc(1, sizeof(*sendDoneCbData));
    if (sendDoneCbData == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to allocate memory for send done callback data.");
        return ENOMEM;
    }
    celix_autofree char* responseTopic = celix_utils_strdup(requestInfo->responseTopic);
    if (responseTopic == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to get response topic from sync event %s.", requestInfo->topic);
        return ENOMEM;
    }
    celix_autofree void* correlationData = NULL;
    size_t correlationDataSize = 0;
    if (requestInfo->correlationData != NULL  && requestInfo->correlationDataSize != 0) {
        correlationData = malloc(requestInfo->correlationDataSize);
        if (correlationData == NULL) {
            celix_logHelper_error(earpm->logHelper, "Failed to allocate memory for correlation data of sync event %s.", requestInfo->topic);
            return ENOMEM;
        }
        memcpy(correlationData, requestInfo->correlationData, requestInfo->correlationDataSize);
        correlationDataSize = requestInfo->correlationDataSize;
    }
    sendDoneCbData->earpm = earpm;
    sendDoneCbData->responseTopic = responseTopic;
    sendDoneCbData->correlationData = correlationData;
    sendDoneCbData->correlationDataSize = correlationDataSize;
    celix_status_t status = celix_earpmDeliverer_sendEvent(earpm->deliverer, requestInfo->topic, celix_steal_ptr(eventProps),
                                                           celix_earpm_sendEventDone, sendDoneCbData);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to send event to local handler for %s. %d.", requestInfo->topic, status);
        return status;
    }
    celix_steal_ptr(responseTopic);
    celix_steal_ptr(correlationData);
    celix_steal_ptr(sendDoneCbData);
    return CELIX_SUCCESS;
}

static void celix_earpm_processSyncEventMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_client_request_info_t* requestInfo) {
    celix_status_t status = celix_earpm_deliverSyncEvent(earpm, requestInfo);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to deliver sync event %s.", requestInfo->topic);
        (void)celix_earpm_sendSyncEventAck(earpm, requestInfo->responseTopic, requestInfo->correlationData,
                                            requestInfo->correlationDataSize);
        return;
    }
    return;
}

static void celix_earpm_processAsyncEventMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_client_request_info_t* requestInfo) {
    celix_autoptr(celix_properties_t) eventProps = NULL;
    if (requestInfo->payload != NULL && requestInfo->payloadSize > 0) {
        celix_status_t status = celix_properties_loadFromString(requestInfo->payload, 0, &eventProps);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(earpm->logHelper, "Failed to load event properties for %s.", requestInfo->topic);
            return;
        }
    }
    celix_earpmDeliverer_postEvent(earpm->deliverer, requestInfo->topic, celix_steal_ptr(eventProps));
    return;
}

static void celix_earpm_processEventMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_client_request_info_t* requestInfo) {
    if (requestInfo->responseTopic != NULL) {
        celix_earpm_processSyncEventMessage(earpm, requestInfo);
    } else {
        celix_earpm_processAsyncEventMessage(earpm, requestInfo);
    }
    return;
}

static bool celix_earpm_isMsgCompatible(const celix_earpm_client_request_info_t* requestInfo) {
    char actualVersion[16]= {0};
    if (requestInfo->version == NULL) {
        return false;
    }
    int ret = snprintf(actualVersion, sizeof(actualVersion), "%s", requestInfo->version);
    if (ret < 0 || ret >= (int)sizeof(actualVersion)) {
        return false;
    }
    char* endPtr = NULL;
    long actualMajor = strtol(actualVersion, &endPtr, 10);
    if (endPtr == NULL || endPtr[0] != '.') {
        return false;
    }
    long actualMinor = strtol(endPtr + 1, NULL, 10);
    long expectedMajor = strtol(CELIX_EARPM_MSG_VERSION, &endPtr, 10);
    assert(endPtr[0] == '.');
    long expectedMinor = strtol(endPtr + 1, NULL, 10);

    if (actualMajor == expectedMajor && actualMinor <= expectedMinor) {
        return true;
    }
    return false;
}

static void celix_earpm_receiveMsgCallback(void* handle, const celix_earpm_client_request_info_t* requestInfo) {
    assert(handle != NULL);
    assert(requestInfo != NULL);
    assert(requestInfo->topic != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;

    if (!celix_earpm_isMsgCompatible(requestInfo)) {
        celix_logHelper_warning(earpm->logHelper, "%s message version(%s) is incompatible.",requestInfo->topic, requestInfo->version == NULL ? "null" : requestInfo->version);
        return;
    }

    if (strncmp(requestInfo->topic,CELIX_EARPM_TOPIC_PREFIX, sizeof(CELIX_EARPM_TOPIC_PREFIX)-1) == 0) {
        celix_earpm_processControlMessage(earpm, requestInfo);
    } else {// user topic
        celix_earpm_processEventMessage(earpm, requestInfo);
    }
    return;
}

static void celix_earpm_queryRemoteHandlerInfo(celix_event_admin_remote_provider_mqtt_t* earpm) {
    const char* topic = CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC;
    //If the mqtt connection is disconnected, we will query the handler information again
    // when the connection is re-established in celix_earpm_connectedCallback,
    // so we use CELIX_EARPM_QOS_AT_MOST_ONCE qos here.
    celix_earpm_client_request_info_t requestInfo;
    memset(&requestInfo, 0, sizeof(requestInfo));
    requestInfo.topic = topic;
    requestInfo.qos = CELIX_EARPM_QOS_AT_MOST_ONCE;
    requestInfo.pri = CELIX_EARPM_MSG_PRI_HIGH;
    requestInfo.senderUUID = earpm->fwUUID;
    requestInfo.version = CELIX_EARPM_MSG_VERSION;
    celix_status_t status = celix_earpmClient_publishAsync(earpm->mqttClient, &requestInfo);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to publish %s, %d.", topic, status);
    }
    return;
}

static void celix_earpm_connectedCallback(void* handle) {
    assert(handle != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;

    celix_earpm_queryRemoteHandlerInfo(earpm);
    celix_earpm_refreshAllHandlerInfoToRemote(earpm);

    return;
}

static void celix_earpm_infoCmd(celix_event_admin_remote_provider_mqtt_t* earpm, FILE* outStream) {
    fprintf(outStream, "Event Admin Remote Provider Based On MQTT Info:\n");
    {
        fprintf(outStream, "\nLocal Subscriptions:\n");
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
        CELIX_STRING_HASH_MAP_ITERATE(earpm->eventSubscriptions, iter) {
            celix_earpm_event_subscription_t* subscription = iter.value.ptrValue;
            fprintf(outStream, "\t%s -> QOS:%d, SubCnt:%d\n", iter.key, subscription->curQos, celix_arrayList_size(subscription->handlerServiceIdList));
        }
        fprintf(outStream, "\nRemote Framework Info:\n");
        CELIX_STRING_HASH_MAP_ITERATE(earpm->remoteFrameworks, iter) {
            celix_earpm_remote_framework_info_t* fwInfo = iter.value.ptrValue;
            fprintf(outStream, "\t%s -> HandlerCnt:%zu, NoAckCnt:%d, PendingAckCnt:%zu\n", iter.key,
                    celix_longHashMap_size(fwInfo->handlerInfoMap), fwInfo->continuousNoAckCount, celix_longHashMap_size(fwInfo->eventAckSeqNrMap));
        }
    }

    celix_earpmClient_info(earpm->mqttClient, outStream);
}

bool celix_earpm_executeCommand(void* handle, const char* commandLine, FILE* outStream, FILE* errorStream) {
    assert(handle != NULL);
    assert(commandLine != NULL);
    assert(outStream != NULL);
    assert(errorStream != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;
    celix_autofree char* cmd = celix_utils_strdup(commandLine);
    if (cmd == NULL) {
        fprintf(errorStream, "Failed to process command line %s.\n", commandLine);
        return false;
    }
    const char* subCmd = NULL;
    char* savePtr = NULL;
    strtok_r(cmd, " ", &savePtr);
    subCmd = strtok_r(NULL, " ", &savePtr);
    if (subCmd == NULL) {
        celix_earpm_infoCmd(earpm, outStream);
    } else {
        fprintf(errorStream, "Unexpected sub command %s\n", subCmd);
        return false;
    }
    return true;
}

size_t celix_earpm_currentRemoteFrameworkCount(celix_event_admin_remote_provider_mqtt_t* earpm) {
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    size_t cnt = celix_stringHashMap_size(earpm->remoteFrameworks);
    return cnt;
}