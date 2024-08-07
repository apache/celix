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
#include <mosquitto.h>
#include <mqtt_protocol.h>
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
#include "celix_framework.h"
#include "celix_event_constants.h"
#include "celix_event_remote_provider_service.h"
#include "celix_earpm_mosquitto_cleanup.h"
#include "celix_earpm_event_deliverer.h"
#include "celix_earpm_client.h"
#include "celix_earpm_constants.h"
#include "celix_earpm_broker_discovery.h"


typedef struct celix_earpm_event_handler {
    celix_array_list_t* topics;
    char* filter;
    celix_earpm_qos_e qos;
    long serviceId;
}celix_earpm_event_handler_t;

typedef struct celix_earpm_event_subscription {
    celix_array_list_t* handlerServiceIdList;
    celix_earpm_qos_e curQos;
}celix_earpm_event_subscription_t;

typedef struct celix_earpm_remote_handler_info {
    celix_array_list_t* topics;
    celix_filter_t* filter;
}celix_earpm_remote_handler_info_t;

typedef struct celix_earpm_remote_framework_info {
    int expiryIntervalInSeconds;
    struct timespec expiryTime;
    celix_long_hash_map_t* handlerInfoMap;//key = serviceId, value = celix_earpm_remote_handler_info_t*
    celix_long_hash_map_t* eventAckSeqNrMap;//key = seqNr, value = celix_array_list_t* of serviceIds
    int continuousNoAckCount;
}celix_earpm_remote_framework_info_t;

struct celix_earpm_sync_event_correlation_data {
    long ackSeqNr;
};

struct celix_earpm_send_event_done_callback_data {
    celix_event_admin_remote_provider_mqtt_t* earpm;
    char* ackTopic;
    mosquitto_property* ackProps;
};

struct celix_event_admin_remote_provider_mqtt {
    celix_bundle_context_t* ctx;
    celix_log_helper_t* logHelper;
    const char* fwUUID;
    celix_earpm_qos_e defaultQos;
    char* syncEventAckTopic;
    celix_earpm_event_deliverer_t* deliverer;
    celix_earpm_client_t* mqttClient;
    celix_thread_mutex_t mutex;//protects belows
    celix_thread_cond_t ackCond;
    celix_long_hash_map_t* eventHandlers;//key = serviceId, value = celix_earpm_event_handler_t*
    celix_string_hash_map_t* eventSubscriptions;//key = topic, value = celix_earpm_event_subscription_t*
    celix_string_hash_map_t* remoteFrameworks;// key = frameworkUUID of remote frameworks, value = celix_remote_framework_info_t*
    long lastAckSeqNr;
    long remoteFwExpiryEventId;
};

/**
 * @todo
 * 1. 本地客户端断开重连了如何处理，复用原来的远程订阅者信息还是重新获取，如果复用，可能对端也不在了，如果不复用，重新获取阶段消息如何处理---重连后重新刷新远程订阅者信息并设置过期时间，重连前复用已有信息
 * 2. 同步消息处理，如果对端不在了，如何处理---等待WILL消息，同时使用断路机制
 * 3. 对端重连期间会漏远程消息，该情况是否需要处理----可以考虑使用retain标记，对端重连后可以收到消息,需要验证后再实施(这种情况应该考虑使用QOS1或QOS2订阅/发布消息)--不需要处理，同步发送接口报错以及QOS机制
 * 4. 客户端是否需要设置will消息----不需要，异常断开一般会自动重连，如果一段时间内没重连，使用2中的机制处理，如果使用will,处理流程上和2有点重复-----需要,使用2中的机制需要有事件产生，如果没有事件产生，状态会更新不及时
 * 5. 添加调试日志
 * 6. 添加段路机制
 * 7. 修改文件名
 */

static void celix_earpm_eventHandlerDestroy(celix_earpm_event_handler_t* handler);
static void celix_earpm_subscriptionDestroy(celix_earpm_event_subscription_t* subscription);
static void celix_earpm_remoteFrameworkInfoDestroy(celix_earpm_remote_framework_info_t* info);
static void celix_earpm_receiveMsgCallback(void* handle, const char* topic, const char* payload, size_t payloadSize, const mosquitto_property *mqttProps);
static void celix_earpm_connectedCallback(void* handle);
static void celix_earpm_subOrUnsubRemoteEventsForHandler(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_event_handler_t* handler, bool subscribe);
static void celix_earpm_addHandlerInfoToRemote(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_event_handler_t* handler);
static void celix_earpm_removeHandlerInfoFromRemote(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_event_handler_t* handler);
static celix_status_t celix_earpm_publishEvent(celix_event_admin_remote_provider_mqtt_t* earpm, const char *topic,
                                               const celix_properties_t *eventProps, bool async);
static void celix_earpm_scheduleRemoteFwExpiryEvent(celix_event_admin_remote_provider_mqtt_t* earpm, int delayInSeconds);


celix_event_admin_remote_provider_mqtt_t* celix_earpm_create(celix_bundle_context_t* ctx) {
    celix_autofree celix_event_admin_remote_provider_mqtt_t* earpm = calloc(1, sizeof(*earpm));
    if (earpm == NULL) {
        return NULL;
    }
    earpm->ctx = ctx;
    earpm->lastAckSeqNr = 0;
    earpm->remoteFwExpiryEventId = -1;
    celix_autoptr(celix_log_helper_t) logHelper = earpm->logHelper = celix_logHelper_create(ctx, "CELIX_EARPM");
    if (logHelper == NULL) {
        return NULL;
    }
    earpm->fwUUID = celix_bundleContext_getProperty(ctx, CELIX_FRAMEWORK_UUID, NULL);
    if (earpm->fwUUID == NULL) {
        celix_logHelper_error(logHelper, "Failed to get framework UUID.");
        return NULL;
    }
    earpm->defaultQos = (celix_earpm_qos_e)celix_bundleContext_getPropertyAsLong(ctx, CELIX_EARPM_EVENT_QOS, CELIX_EARPM_EVENT_QOS_DEFAULT);
    if (earpm->defaultQos <= CELIX_EARPM_QOS_UNKNOWN || earpm->defaultQos > CELIX_EARPM_QOS_MAX) {
        celix_logHelper_error(logHelper, "Invalid default QOS(%d) value.", (int)earpm->defaultQos);
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
    celix_earpmc_create_options_t opts;
    memset(&opts, 0, sizeof(opts));
    opts.ctx = ctx;
    opts.logHelper = logHelper;
    opts.sessionEndTopic = CELIX_EARPM_SESSION_END_TOPIC;
    celix_autoptr(mosquitto_property) sessionEndProps = NULL;
    if (mosquitto_property_add_string_pair(&sessionEndProps, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, earpm->fwUUID) != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to add framework uuid to session end message.");
        return NULL;
    }
    opts.sessionEndProps = celix_steal_ptr(sessionEndProps);
    opts.callbackHandle = earpm;
    opts.receiveMsgCallback = celix_earpm_receiveMsgCallback;
    opts.connectedCallback = celix_earpm_connectedCallback;
    celix_autoptr(celix_earpm_client_t) mqttClient = earpm->mqttClient = celix_earpmc_create(&opts);
    if (mqttClient == NULL) {
        celix_logHelper_error(logHelper, "Failed to create mqtt client.");
        return NULL;
    }
    status = celix_earpmc_subscribe(earpm->mqttClient, CELIX_EARPM_TOPIC_PATTERN, CELIX_EARPM_QOS_AT_LEAST_ONCE);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to subscribe %s. %d.", CELIX_EARPM_TOPIC_PATTERN, status);
        return NULL;
    }
    celix_autoptr(celix_earpm_event_deliverer_t) deliverer = earpm->deliverer = celix_earpmd_create(ctx, logHelper);
    if (deliverer == NULL) {
        celix_logHelper_error(logHelper, "Failed to create event deliverer.");
        return NULL;
    }

    celix_steal_ptr(deliverer);
    celix_steal_ptr(mqttClient);
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
    celix_earpmd_destroy(earpm->deliverer);
    celix_earpmc_destroy(earpm->mqttClient);
    if (celix_framework_isCurrentThreadTheEventLoop(celix_bundleContext_getFramework(earpm->ctx))) {
        celix_bundleContext_removeScheduledEventAsync(earpm->ctx, earpm->remoteFwExpiryEventId);
    } else {
        celix_bundleContext_removeScheduledEvent(earpm->ctx, earpm->remoteFwExpiryEventId);
    }
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

celix_status_t celix_earpm_endpointAdded(void* handle, endpoint_description_t* endpoint, char* matchedFilter) {
    assert(handle != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;
    return celix_earpmc_endpointAdded(earpm->mqttClient , endpoint, matchedFilter);
}

celix_status_t celix_earpm_endpointRemoved(void* handle, endpoint_description_t* endpoint, char* matchedFilter) {
    assert(handle != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;
    return celix_earpmc_endpointRemoved(earpm->mqttClient , endpoint, matchedFilter);
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
        celix_logHelper_error(earpm->logHelper, "Failed to attach handler service id to subscription info for %s. %d.", topic, status);
        return;
    }
    if (subscription->curQos < qos) {
        status = celix_earpmc_subscribe(earpm->mqttClient, topic, qos);
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
    celix_arrayList_removeLong(subscription->handlerServiceIdList, handlerServiceId);
    int size = celix_arrayList_size(subscription->handlerServiceIdList);
    if (size == 0) {
        celix_status_t status = celix_earpmc_unsubscribe(earpm->mqttClient, topic);
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
            celix_status_t status = celix_earpmc_subscribe(earpm->mqttClient, topic, maxQos);
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
        if (topic == NULL) {
            continue;
        }
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
        if (topic == NULL) {
            continue;
        }
        if (json_array_append_new(topics, json_string(topic)) != 0) {
            celix_logHelper_error(earpm->logHelper, "Failed to add topic to json array.");
            return NULL;
        }
    }
    json_auto_t* handlerInfo = json_pack("{s:O, s:i}", "topics", topics, "handlerId", handler->serviceId);
    if (handlerInfo == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to pack handler information.");
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
    json_auto_t* handlerInfo = celix_earpm_genHandlerInformation(earpm, handler);
    if (handlerInfo == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to create handler information for handler %li.", handler->serviceId);
        return;
    }
    if (json_object_set_new(handlerInfo, "fwExpiryInterval", json_integer(CELIX_EARPM_SESSION_EXPIRY_INTERVAL_DEFAULT)) != 0) {
        celix_logHelper_error(earpm->logHelper, "Failed to add framework expiry interval to handler information message.");
        return;
    }
    celix_autofree char* payload = json_dumps(handlerInfo, JSON_COMPACT | JSON_ENCODE_ANY);
    if (payload == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to dump handler information message payload for handler %li.", handler->serviceId);
        return;
    }
    celix_autoptr(mosquitto_property) mqttProps = NULL;
    if (mosquitto_property_add_string_pair(&mqttProps, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, earpm->fwUUID) != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to add framework uuid to message %s mqtt properties.", topic);
        return;
    }
    //If the mqtt connection is disconnected, we will resend the handler information
    // when the connection is re-established in celix_earpm_connectedCallback,
    // so we use CELIX_EARPM_QOS_AT_MOST_ONCE qos here.
    celix_status_t status = celix_earpmc_publishAsync(earpm->mqttClient, topic, payload, strlen(payload),
                                                      CELIX_EARPM_QOS_AT_MOST_ONCE, mqttProps, CELIX_EARPMC_MSG_PRI_HIGH);
    if (status != CELIX_SUCCESS && status != ENOTCONN) {
        celix_logHelper_error(earpm->logHelper, "Failed to publish %s. payload:%s. %d.", topic, payload, status);
    }
    return;
}

static void celix_earpm_removeHandlerInfoFromRemote(celix_event_admin_remote_provider_mqtt_t* earpm, const celix_earpm_event_handler_t* handler) {
    const char* topic = CELIX_EARPM_HANDLER_INFO_REMOVE_TOPIC;
    celix_autoptr(mosquitto_property) mqttProps = NULL;
    if (mosquitto_property_add_string_pair(&mqttProps, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, earpm->fwUUID) != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to add framework uuid to message %s mqtt properties.", topic);
        return;
    }
    char payload[128] = {0};
    (void)snprintf(payload, sizeof(payload), "{\"handlerId\":%ld}", handler->serviceId);
    //If the mqtt connection is disconnected, we will resend the handler information
    // when the connection is re-established in celix_earpm_connectedCallback,
    // so we use CELIX_EARPM_QOS_AT_MOST_ONCE qos here.
    celix_status_t status = celix_earpmc_publishAsync(earpm->mqttClient, topic, payload, strlen(payload),
                                                      CELIX_EARPM_QOS_AT_MOST_ONCE, mqttProps, CELIX_EARPMC_MSG_PRI_HIGH);
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
    return celix_earpmd_setEventAdminSvc(earpm->deliverer, (celix_event_admin_service_t*)eventAdminSvc);
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
    int32_t expiryInterval = (int32_t)celix_properties_getAsLong(eventProps, CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, -1);
    celix_autoptr(mosquitto_property) mqttProps = NULL;
    if (expiryInterval > 0) {
        int rc = mosquitto_property_add_int32(&mqttProps, MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, expiryInterval);
        if (rc != MOSQ_ERR_SUCCESS) {
            celix_logHelper_warning(earpm->logHelper, "Failed to set message expiry interval property for %s.", topic);
        }
    }
    celix_status_t status = celix_earpmc_publishAsync(earpm->mqttClient, topic, payload, payloadSize, qos, mqttProps,
                                                      CELIX_EARPMC_MSG_PRI_LOW);
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
        if (fwInfo->continuousNoAckCount > CELIX_EARPM_CONTINUOUS_ACK_TIMEOUT_COUNT_MAX) {
            celix_logHelper_warning(earpm->logHelper, "Continuous ack timeout for remote framework %s. No waiting for the ack of event %s.", iter.key, topic);
            continue;
        }
        celix_autoptr(celix_array_list_t) matchedHandlerServiceIdList = celix_arrayList_createLongArray();
        if (matchedHandlerServiceIdList == NULL) {
            celix_logHelper_logTssErrors(earpm->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(earpm->logHelper, "Failed to create matched handler service id list.");
            celix_earpm_clearAckSeqNr(earpm, ackSeqNr);
            return CELIX_ENOMEM;
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
    celix_status_t status = CELIX_SUCCESS;
    int32_t expiryInterval = (int32_t)celix_properties_getAsLong(eventProps, CELIX_EVENT_REMOTE_EXPIRY_INTERVAL, CELIX_EARPM_SYNC_EVENT_TIMEOUT_DEFAULT);
    if (expiryInterval <= 0) {
        celix_logHelper_error(earpm->logHelper, "Invalid expiry interval for %s.", topic);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_autoptr(mosquitto_property) mqttProps = NULL;
    int rc = mosquitto_property_add_int32(&mqttProps, MQTT_PROP_MESSAGE_EXPIRY_INTERVAL, expiryInterval);
    if (rc != MOSQ_ERR_SUCCESS) {
        celix_logHelper_warning(earpm->logHelper, "Failed to set message expiry interval property for %s, %d.", topic, rc);
    }
    rc = mosquitto_property_add_string(&mqttProps, MQTT_PROP_RESPONSE_TOPIC, earpm->syncEventAckTopic);
    if (rc != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to set message response topic property for %s, %d.", topic, rc);
        return CELIX_ENOMEM;
    }
    long ackSeqNr = __atomic_add_fetch(&earpm->lastAckSeqNr, 1, __ATOMIC_RELAXED);
    struct celix_earpm_sync_event_correlation_data data;
    memset(&data, 0, sizeof(data));
    data.ackSeqNr = ackSeqNr;
    rc = mosquitto_property_add_binary(&mqttProps, MQTT_PROP_CORRELATION_DATA, &data, sizeof(data));
    if (rc != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to set message correlation data property for %s, %d.", topic, rc);
        return CELIX_ENOMEM;
    }

    status = celix_earpm_associateAckSeqNrWithRemoteHandler(earpm, topic, eventProps, ackSeqNr);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to associate ack seq nr with remote handler for %s. %d.", topic, status);
        return status;
    }

    struct timespec expiryTime = celixThreadCondition_getDelayedTime(expiryInterval);
    status = celix_earpmc_publishSync(earpm->mqttClient, topic, payload, payloadSize, qos, mqttProps,&expiryTime);
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

static void celix_earpm_processSyncEventAckMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const mosquitto_property* mqttProps, const char* remoteFwUUID) {
    celix_autofree void* correlationData = NULL;
    uint16_t len = 0;
    if (mosquitto_property_read_binary(mqttProps, MQTT_PROP_CORRELATION_DATA, &correlationData, &len, false) == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to read correlation data from %s message.", topic);
        return;
    }
    if (len != sizeof(struct celix_earpm_sync_event_correlation_data)) {
        celix_logHelper_error(earpm->logHelper, "Correlation data size is invalid for %s message.", topic);
        return;
    }

    bool wakeupWaitAckThread = false;
    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
        celix_earpm_remote_framework_info_t* fwInfo = celix_stringHashMap_get(earpm->remoteFrameworks, remoteFwUUID);
        if (fwInfo == NULL) {
            celix_logHelper_debug(earpm->logHelper, "No remote framework info for %s.", remoteFwUUID);
            return;
        }
        struct celix_earpm_sync_event_correlation_data data;
        memcpy(&data, correlationData, sizeof(data));
        if (celix_longHashMap_remove(fwInfo->eventAckSeqNrMap, data.ackSeqNr)) {
            wakeupWaitAckThread = true;
        }
    }
    if (wakeupWaitAckThread) {
        celixThreadCondition_broadcast(&earpm->ackCond);
    }
    return;
}

static char* celix_earpm_getFrameworkUUIDFromMqttProperties(const mosquitto_property* mqttProps) {
    char* strName = NULL;
    char* strValue = NULL;
    if (mqttProps == NULL) {
        return NULL;
    }
    mqttProps = mosquitto_property_read_string_pair(mqttProps, MQTT_PROP_USER_PROPERTY, &strName, &strValue, false);
    while (mqttProps != NULL) {
        if (strcmp(strName, CELIX_FRAMEWORK_UUID) == 0) {
            free(strName);
            return strValue;
        }
        free(strName);
        free(strValue);
        mqttProps = mosquitto_property_read_string_pair(mqttProps, MQTT_PROP_USER_PROPERTY, &strName, &strValue, true);
    }
    return NULL;
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
            return NULL;
        }
        if (celix_arrayList_addString(topics, (void*)topic)) {
            celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_WARNING);
            celix_logHelper_warning(logHelper, "Failed to add topic %s to handler information.", topic);
        }
    }
    if (filter !=  NULL) {
        info->filter = celix_filter_create(filter);//If return NULL, then only let event admin does event filter
        if (info->filter == NULL) {
            celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_WARNING);
            celix_logHelper_warning(logHelper, "Failed to create filter for handler information.");
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
                                                                                        const char* remoteFwUUID, int expiryInterval) {
    celix_autofree celix_earpm_remote_framework_info_t* fwInfo = calloc(1, sizeof(*fwInfo));
    if (fwInfo == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to allocate memory for remote framework %s.", remoteFwUUID);
        return NULL;
    }
    fwInfo->expiryIntervalInSeconds = expiryInterval;
    fwInfo->continuousNoAckCount = 0;
    fwInfo->expiryTime.tv_sec = INT32_MAX;
    fwInfo->expiryTime.tv_nsec = 0;
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
        return CELIX_ENOMEM;
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
    json_t* fwExpiry = json_object_get(root, "fwExpiryInterval");
    if (!json_is_integer(fwExpiry)) {
        celix_logHelper_error(earpm->logHelper, "No expiry interval for remote framework %s.", remoteFwUUID);
        return;
    }
    int fwExpiryInterval = (int)json_integer_value(fwExpiry);
    if (fwExpiryInterval < 0) {
        celix_logHelper_error(earpm->logHelper, "Invalid expiry interval(%d) for remote framework %s.", fwExpiryInterval, remoteFwUUID);
        return;
    }
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    celix_earpm_remote_framework_info_t* fwInfo = celix_stringHashMap_get(earpm->remoteFrameworks, remoteFwUUID);
    if (fwInfo == NULL) {
        fwInfo = celix_earpm_createAndAddRemoteFrameworkInfo(earpm, remoteFwUUID, fwExpiryInterval);
        if (fwInfo == NULL) {
            celix_logHelper_error(earpm->logHelper, "Failed to create remote framework info for %s.", remoteFwUUID);
            return;
        }
    }
    celix_status_t status = celix_earpm_addRemoteHandlerInfo(earpm, root, fwInfo);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to add remote handler information with %s which from %s.", topic, remoteFwUUID);
        return;
    }
    return;
}

static bool celix_earpm_disassociateRemoteHandlerWithPendingAck(celix_earpm_remote_framework_info_t *fwInfo, long removedHandlerServiceId) {
    bool seqNrRemoved = false;
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
            seqNrRemoved = true;
        } else {
            celix_longHashMapIterator_next(&iter);
        }
    }
    return seqNrRemoved;
}

static void celix_earpm_processHandlerInfoRemoveMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize, const char* remoteFwUUID) {
    json_error_t error;
    json_auto_t* root = json_loadb(payload, payloadSize, 0, &error);
    if (root == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to parse message %s which from %s. %s.", topic, remoteFwUUID, error.text);
        return;
    }
    json_t* id = json_object_get(root, "handlerId");
    if (id == NULL) {
        celix_logHelper_error(earpm->logHelper, "Handler id is lost for topic %s which from %s.", topic, remoteFwUUID);
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
    json_t* fwExpiry = json_object_get(root, "fwExpiryInterval");
    if (!json_is_integer(fwExpiry)) {
        celix_logHelper_error(earpm->logHelper, "No expiry interval for remote framework %s.", remoteFwUUID);
        return;
    }
    int fwExpiryInterval = (int)json_integer_value(fwExpiry);
    if (fwExpiryInterval < 0) {
        celix_logHelper_error(earpm->logHelper, "Invalid expiry interval(%d) for remote framework %s.", fwExpiryInterval, remoteFwUUID);
        return;
    }
    json_t* handlers = json_object_get(root, "handlers");
    if (!json_is_array(handlers)) {
        celix_logHelper_error(earpm->logHelper, "Failed to get handler information from message %s.", topic);
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
            fwInfo = celix_earpm_createAndAddRemoteFrameworkInfo(earpm, remoteFwUUID, fwExpiryInterval);
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

    celix_autoptr(mosquitto_property) mqttProps = NULL;
    if (mosquitto_property_add_string_pair(&mqttProps, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, earpm->fwUUID) != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to set framework uuid to %s mqtt properties.", topic);
        return;
    }

    json_auto_t* payload = json_object();
    if (payload == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to create handler information message payload.");
        return;
    }
    if (json_object_set_new(payload, "fwExpiryInterval", json_integer(CELIX_EARPM_SESSION_EXPIRY_INTERVAL_DEFAULT)) != 0) {
        celix_logHelper_error(earpm->logHelper, "Failed to add framework expiry interval to handler information message.");
        return;
    }
    json_t* handlers = json_array();
    if (handlers == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to create handler information array.");
        return;
    }
    if (json_object_set_new(payload, "handlers", handlers) != 0) {
        celix_logHelper_error(earpm->logHelper, "Failed to add handler information to payload.");
        return;
    }

    //The mutex scope must end after celix_earpmc_publishAsync is called,
    // otherwise it will cause the conflict between CELIX_EARPM_HANDLER_INFO_UPDATE_TOPIC and CELIX_EARPM_HANDLER_INFO_ADD/REMOVE_TOPIC
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    CELIX_LONG_HASH_MAP_ITERATE(earpm->eventHandlers, iter) {
        celix_earpm_event_handler_t* handler = iter.value.ptrValue;
        json_t* handlerInfo = celix_earpm_genHandlerInformation(earpm, handler);
        if (handlerInfo == NULL) {
            celix_logHelper_error(earpm->logHelper, "Failed to create information json object of handler %li.", handler->serviceId);
            continue;
        }
        if (json_array_append_new(handlers, handlerInfo) != 0) {
            celix_logHelper_error(earpm->logHelper, "Failed to add information json object of handler %li .", handler->serviceId);
            continue;
        }
    }
    celix_autofree char* payloadStr = json_dumps(payload, JSON_COMPACT | JSON_ENCODE_ANY);
    if (payloadStr == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to dump handler information message payload.");
        return;
    }
    //If the mqtt connection is disconnected, we will resend the handler information
    // when the connection is re-established in celix_earpm_connectedCallback,
    // so we use CELIX_EARPM_QOS_AT_MOST_ONCE qos here.
    celix_status_t status = celix_earpmc_publishAsync(earpm->mqttClient, topic, payloadStr, strlen(payloadStr),
                                                      CELIX_EARPM_QOS_AT_MOST_ONCE, mqttProps,CELIX_EARPMC_MSG_PRI_HIGH);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to publish %s. payload:%s.", topic, payloadStr);
    }

    return;
}

static void celix_earpm_processHandlerInfoMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize, const char* remoteFwUUID) {
    const char* action = topic + sizeof(CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX) - 1;
    if (strcmp(action, "add") == 0) {
        celix_earpm_processHandlerInfoAddMessage(earpm, topic, payload, payloadSize, remoteFwUUID);
    } else if (strcmp(action, "remove") == 0) {
        celix_earpm_processHandlerInfoRemoveMessage(earpm, topic, payload, payloadSize, remoteFwUUID);
    } else if (strcmp(action, "update") == 0) {
        celix_earpm_processHandlerInfoUpdateMessage(earpm, topic, payload, payloadSize, remoteFwUUID);
    } else if (strcmp(action, "query") == 0) {
        celix_earpm_refreshAllHandlerInfoToRemote(earpm);
    } else {
        celix_logHelper_warning(earpm->logHelper, "Unknown action %s for handler information message.", topic);
    }
    return;
}

static void celix_earpm_processSessionEndMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic CELIX_UNUSED, const char* remoteFwUUID) {
    bool wakeupWaitAckThread = false;
    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
        wakeupWaitAckThread = celix_stringHashMap_remove(earpm->remoteFrameworks, remoteFwUUID);
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
        fwInfo->expiryTime.tv_sec = INT32_MAX;
        fwInfo->expiryTime.tv_nsec = 0;
        fwInfo->continuousNoAckCount = 0;
    }
    return;
}

static void celix_earpm_processSelfMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize, const mosquitto_property* mqttProps) {
    celix_autofree char* remoteFwUUID = celix_earpm_getFrameworkUUIDFromMqttProperties(mqttProps);
    if (remoteFwUUID == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to get remote framework UUID from %s message mqtt properties.", topic);
        return;
    }
    if (strcmp(topic, earpm->syncEventAckTopic) == 0) {
        celix_earpm_processSyncEventAckMessage(earpm, topic, mqttProps, remoteFwUUID);
    } else if (strncmp(topic, CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX, sizeof(CELIX_EARPM_HANDLER_INFO_TOPIC_PREFIX) - 1) == 0) {
        celix_earpm_processHandlerInfoMessage(earpm, topic, payload, payloadSize, remoteFwUUID);
    } else if (strcmp(topic, CELIX_EARPM_SESSION_END_TOPIC) == 0) {
        celix_earpm_processSessionEndMessage(earpm, topic, remoteFwUUID);
    } else {
        celix_logHelper_warning(earpm->logHelper, "Unknown message received on topic %s.", topic);
        return;
    }
    celix_earpm_markRemoteFrameworkActive(earpm, remoteFwUUID);
    return;
}

static bool celix_earpm_isSyncEvent(const mosquitto_property* mqttProps) {
    while (mqttProps) {
        if (mosquitto_property_identifier(mqttProps) == MQTT_PROP_RESPONSE_TOPIC) {
            return true;
        }
        mqttProps = mosquitto_property_next(mqttProps);
    }
    return false;
}

static void celix_earpm_sendEventDone(void* data, const char* topic, celix_status_t rc CELIX_UNUSED) {
    assert(data != NULL);
    assert(topic != NULL);
    struct celix_earpm_send_event_done_callback_data* callData = data;
    celix_status_t status = celix_earpmc_publishAsync(callData->earpm->mqttClient, callData->ackTopic, NULL, 0,
                                                      CELIX_EARPM_QOS_AT_LEAST_ONCE, callData->ackProps, CELIX_EARPMC_MSG_PRI_MIDDLE);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(callData->earpm->logHelper, "Failed to publish response for %s.", topic);
    }
    mosquitto_property_free_all(&callData->ackProps);
    free(callData->ackTopic);
    free(callData);
    return;
}

static celix_status_t celix_earpm_deliverSyncEvent(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize, char* ackTopic, mosquitto_property* ackProps) {
    celix_autoptr(celix_properties_t) eventProps = NULL;
    if (payload != NULL && payloadSize > 0) {
        celix_status_t status = celix_properties_loadFromString2(payload, 0, &eventProps);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(earpm->logHelper, "Failed to load event properties for %s.", topic);
            return status;
        }
    }
    celix_autofree struct celix_earpm_send_event_done_callback_data* sendDoneCbData = calloc(1, sizeof(*sendDoneCbData));
    if (sendDoneCbData == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to allocate memory for send done callback data.");
        return CELIX_ENOMEM;
    }
    sendDoneCbData->earpm = earpm;
    sendDoneCbData->ackTopic = ackTopic;
    sendDoneCbData->ackProps = ackProps;
    celix_status_t status = celix_earpmd_sendEvent(earpm->deliverer, topic, celix_steal_ptr(eventProps), celix_earpm_sendEventDone, sendDoneCbData);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to send event to local handler for %s. %d.", topic, status);
        return status;
    }
    celix_steal_ptr(sendDoneCbData);
    return CELIX_SUCCESS;
}

static void celix_earpm_processSyncEventMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize, const mosquitto_property* mqttProps) {
    celix_autofree char* ackTopic = NULL;
    if (mosquitto_property_read_string(mqttProps, MQTT_PROP_RESPONSE_TOPIC, &ackTopic, false) == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to get ack topic from sync event %s.", topic);
        return;
    }
    celix_autofree void* correlationData = NULL;
    uint16_t len = 0;
    if (mosquitto_property_read_binary(mqttProps, MQTT_PROP_CORRELATION_DATA, &correlationData, &len, false) == NULL) {
        celix_logHelper_error(earpm->logHelper, "Failed to get correlation data from sync event %s.", topic);
        return;
    }
    celix_autoptr(mosquitto_property) ackProps = NULL;
    if (mosquitto_property_add_binary(&ackProps, MQTT_PROP_CORRELATION_DATA, correlationData, len) != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to add correlation data to %s of sync event %s.", ackTopic, topic);
        return;
    }
    if (mosquitto_property_add_string_pair(&ackProps, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, earpm->fwUUID) != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to add framework uuid to %s of sync event %s.", ackTopic, topic);
        return;
    }
    celix_status_t status = celix_earpm_deliverSyncEvent(earpm, topic, payload, payloadSize, ackTopic, ackProps);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to deliver sync event %s.", topic);
        (void)celix_earpmc_publishAsync(earpm->mqttClient, ackTopic, NULL, 0, CELIX_EARPM_QOS_AT_LEAST_ONCE, ackProps, CELIX_EARPMC_MSG_PRI_MIDDLE);
        return;
    }
    celix_steal_ptr(ackProps);
    celix_steal_ptr(ackTopic);
    return;
}

static void celix_earpm_processAsyncEventMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize) {
    celix_autoptr(celix_properties_t) eventProps = NULL;
    if (payload != NULL && payloadSize > 0) {
        celix_status_t status = celix_properties_loadFromString2(payload, 0, &eventProps);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(earpm->logHelper, "Failed to load event properties for %s.", topic);
            return;
        }
    }
    celix_earpmd_postEvent(earpm->deliverer, topic, celix_steal_ptr(eventProps));
    return;
}

static void celix_earpm_processEventMessage(celix_event_admin_remote_provider_mqtt_t* earpm, const char* topic, const char* payload, size_t payloadSize, const mosquitto_property* mqttProps) {
    if (celix_earpm_isSyncEvent(mqttProps)) {
        celix_earpm_processSyncEventMessage(earpm, topic, payload, payloadSize, mqttProps);
    } else {
        celix_earpm_processAsyncEventMessage(earpm, topic, payload, payloadSize);
    }
    return;
}

static void celix_earpm_receiveMsgCallback(void* handle, const char* topic, const char* payload, size_t payloadSize, const mosquitto_property *mqttProps) {
    assert(handle != NULL);
    assert(topic != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;

    if (strncmp(topic,CELIX_EARPM_TOPIC_PREFIX, sizeof(CELIX_EARPM_TOPIC_PREFIX)-1) == 0) {
        celix_earpm_processSelfMessage(earpm, topic, payload, payloadSize, mqttProps);
    } else {// user topic
        celix_earpm_processEventMessage(earpm, topic, payload, payloadSize, mqttProps);
    }
    return;
}

static void celix_earpm_onRemoteFrameworkExpiryEvent(void* data) {
    assert(data != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)data;
    bool hasExpired = false;
    {
        celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
        int nextScheduleInterval = -1;
        celix_string_hash_map_iterator_t iter = celix_stringHashMap_begin(earpm->remoteFrameworks);
        while (!celix_stringHashMapIterator_isEnd(&iter)) {
            celix_earpm_remote_framework_info_t* fwInfo = (celix_earpm_remote_framework_info_t*)iter.value.ptrValue;
            double elapsedSecs = celix_elapsedtime(CLOCK_MONOTONIC, fwInfo->expiryTime);
            if (elapsedSecs >= 0) {
                celix_logHelper_warning(earpm->logHelper, "Remote framework %s expired.", iter.key);
                celix_stringHashMapIterator_remove(&iter);
                hasExpired = true;
            } else {
                if (fwInfo->expiryTime.tv_sec != INT32_MAX) {
                    int t = (int)(-elapsedSecs) + 1;
                    nextScheduleInterval = (nextScheduleInterval > 0 && nextScheduleInterval < t) ? nextScheduleInterval : t;
                }
                celix_stringHashMapIterator_next(&iter);
            }
        }
        earpm->remoteFwExpiryEventId = -1;
        if (nextScheduleInterval > 0) {
            celix_earpm_scheduleRemoteFwExpiryEvent(earpm, nextScheduleInterval);
        }
    }
    if (hasExpired) {
        celixThreadCondition_broadcast(&earpm->ackCond);
    }

    return;
}

static void celix_earpm_scheduleRemoteFwExpiryEvent(celix_event_admin_remote_provider_mqtt_t* earpm, int delayInSeconds) {
    if (earpm->remoteFwExpiryEventId < 0) {
        celix_scheduled_event_options_t opts = CELIX_EMPTY_SCHEDULED_EVENT_OPTIONS;
        opts.name = "CELIX_EARPM_REMOTE_FW_EXPIRY_EVENT";
        opts.initialDelayInSeconds = delayInSeconds;
        opts.callbackData = earpm;
        opts.callback = celix_earpm_onRemoteFrameworkExpiryEvent;
        earpm->remoteFwExpiryEventId = celix_bundleContext_scheduleEvent(earpm->ctx, &opts);
        if (earpm->remoteFwExpiryEventId < 0) {
            celix_logHelper_error(earpm->logHelper, "Failed to schedule remote framework expiry event.");
        }
    }
    return;
}

static void celix_earpm_queryAllRemoteHandlerInfo(celix_event_admin_remote_provider_mqtt_t* earpm) {
    const char* topic = CELIX_EARPM_HANDLER_INFO_QUERY_TOPIC;
    celix_autoptr(mosquitto_property) mqttProps = NULL;
    if (mosquitto_property_add_string_pair(&mqttProps, MQTT_PROP_USER_PROPERTY, CELIX_FRAMEWORK_UUID, earpm->fwUUID) != MOSQ_ERR_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to set framework uuid to %s mqtt properties.", topic);
        return;
    }
    //If the mqtt connection is disconnected, we will query the handler information again
    // when the connection is re-established in celix_earpm_connectedCallback,
    // so we use CELIX_EARPM_QOS_AT_MOST_ONCE qos here.
    celix_status_t status = celix_earpmc_publishAsync(earpm->mqttClient, topic, NULL, 0, CELIX_EARPM_QOS_AT_MOST_ONCE,
                                                      mqttProps, CELIX_EARPMC_MSG_PRI_HIGH);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(earpm->logHelper, "Failed to publish %s.", topic);
        return;
    }
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    struct timespec currentTime = celix_gettime(CLOCK_MONOTONIC);
    int scheduleInterval = -1;
    CELIX_STRING_HASH_MAP_ITERATE(earpm->remoteFrameworks, iter) {
        celix_earpm_remote_framework_info_t* info = iter.value.ptrValue;
        if (info->expiryTime.tv_sec == INT32_MAX) {
            info->expiryTime = currentTime;
            int expiryIntervalInSeconds = info->expiryIntervalInSeconds;
            info->expiryTime.tv_sec += expiryIntervalInSeconds;
            scheduleInterval = (scheduleInterval > 0 && scheduleInterval <= expiryIntervalInSeconds) ? scheduleInterval : expiryIntervalInSeconds;
        }
    }
    if (scheduleInterval >= 0) {
        celix_earpm_scheduleRemoteFwExpiryEvent(earpm, scheduleInterval);
    }
    return;
}

/**
 * @todo
 * 1.订阅远程主题消息
 * 2.订阅回应报文
 * 3.刷新订阅关系
 * 4.发布获取远程订阅者的消息
 * 5.发布缓存的异步消息----还没有获取到远程订阅信息如何处理
 * 6.唤醒同步等待
 * */
static void celix_earpm_connectedCallback(void* handle) {
    assert(handle != NULL);
    celix_event_admin_remote_provider_mqtt_t* earpm = (celix_event_admin_remote_provider_mqtt_t*)handle;

    celix_earpm_queryAllRemoteHandlerInfo(earpm);
    celix_earpm_refreshAllHandlerInfoToRemote(earpm);

    return;
}

size_t celix_earpm_currentRemoteFrameworkCnt(celix_event_admin_remote_provider_mqtt_t* earpm) {
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&earpm->mutex);
    size_t cnt = celix_stringHashMap_size(earpm->remoteFrameworks);
    return cnt;
}