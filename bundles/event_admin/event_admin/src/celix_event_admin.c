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
#include "celix_event_admin.h"

#include <string.h>
#include <stdbool.h>
#include <assert.h>
#include <errno.h>

#include "celix_event_handler_service.h"
#include "celix_event_constants.h"
#include "celix_event.h"
#include "celix_log_helper.h"
#include "celix_string_hash_map.h"
#include "celix_long_hash_map.h"
#include "celix_filter.h"
#include "celix_constants.h"
#include "celix_threads.h"
#include "celix_utils.h"
#include "celix_stdlib_cleanup.h"


#define CELIX_EVENT_ADMIN_MAX_HANDLER_THREADS 5
#define CELIX_EVENT_ADMIN_MAX_PARALLEL_EVENTS_OF_HANDLER (CELIX_EVENT_ADMIN_MAX_HANDLER_THREADS/3 + 1) //max parallel async event for a single handler
#define CELIX_EVENT_ADMIN_MAX_HANDLE_EVENT_TIME 60 //seconds
#define CELIX_EVENT_ADMIN_MAX_EVENT_QUEUE_SIZE 512 //events

typedef struct celix_event_handler {
    celix_event_handler_service_t* service;
    long serviceId;
    const char* serviceDescription;
    bool asyncOrdered;
    celix_filter_t* eventFilter;
    bool blackListed;//Blacklisted handlers must not be notified of any events.
    unsigned int handlingAsyncEventCnt;
}celix_event_handler_t;

typedef struct celix_event_channel {
    celix_array_list_t* eventHandlerSvcIdList;
}celix_event_channel_t;

typedef struct celix_event_entry {
    celix_event_t* event;
    celix_long_hash_map_t* eventHandlers;//key: event handler service id, value: null
}celix_event_entry_t;

struct celix_event_admin {
    celix_bundle_context_t* ctx;
    celix_log_helper_t* logHelper;
    celix_thread_rwlock_t lock;//projects: channels,eventHandlers
    celix_event_channel_t channelMatchingAllEvents;
    celix_string_hash_map_t* channelsMatchingTopic; //key: topic, value: celix_event_channel_t *
    celix_string_hash_map_t* channelsMatchingPrefixTopic;//key:prefix topic, value: celix_event_channel_t *
    celix_long_hash_map_t* eventHandlers;//key: event handler service id, value: celix_event_handler_t*
    celix_thread_mutex_t eventsMutex;// protect belows
    celix_thread_cond_t eventsTriggerCond;
    celix_array_list_t* asyncEventQueue;//array_list<celix_event_entry_t*>
    bool threadsRunning;
    celix_thread_t eventHandlerThreads[CELIX_EVENT_ADMIN_MAX_HANDLER_THREADS];
};

static void* celix_eventAdmin_deliverEventThread(void* data);

celix_event_admin_t* celix_eventAdmin_create(celix_bundle_context_t* ctx) {
    celix_autofree celix_event_admin_t* ea = calloc(1, sizeof(*ea));
    if (ea == NULL) {
        return NULL;
    }
    ea->ctx = ctx;
    ea->threadsRunning = false;

    celix_autoptr(celix_log_helper_t) logHelper = ea->logHelper = celix_logHelper_create(ctx, "CelixEventAdmin");
    if (logHelper == NULL) {
        return NULL;
    }

    celix_status_t status = celixThreadRwlock_create(&ea->lock, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create event admin lock.");
        return NULL;
    }
    celix_autoptr(celix_thread_rwlock_t) lock = &ea->lock;
    celix_autoptr(celix_array_list_t) channelMatchingAllEvents = ea->channelMatchingAllEvents.eventHandlerSvcIdList = celix_arrayList_create();
    if (channelMatchingAllEvents == NULL) {
        celix_logHelper_error(logHelper, "Failed to create event channel matching all events.");
        return NULL;
    }
    celix_autoptr(celix_string_hash_map_t) channelsMatchingTopic = ea->channelsMatchingTopic = celix_stringHashMap_create();
    if (channelsMatchingTopic == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Failed to create event channels matching topic.");
        return NULL;
    }
    celix_autoptr(celix_string_hash_map_t) channelsMatchingPrefixTopic = ea->channelsMatchingPrefixTopic = celix_stringHashMap_create();
    if (channelsMatchingPrefixTopic == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Failed to create event channels matching prefix topic.");
        return NULL;
    }
    celix_autoptr(celix_long_hash_map_t) eventHandlers = ea->eventHandlers = celix_longHashMap_create();
    if (eventHandlers == NULL) {
        celix_logHelper_logTssErrors(logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(logHelper, "Failed to create event handler map.");
        return NULL;
    }

    status = celixThreadMutex_create(&ea->eventsMutex, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create event admin events mutex.");
        return NULL;
    }
    celix_autoptr(celix_thread_mutex_t) mutex = &ea->eventsMutex;
    status = celixThreadCondition_init(&ea->eventsTriggerCond, NULL);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(logHelper, "Failed to create event admin events not empty condition.");
        return NULL;
    }
    celix_autoptr(celix_thread_cond_t) cond = &ea->eventsTriggerCond;

    celix_autoptr(celix_array_list_t) asyncEventQueue = ea->asyncEventQueue = celix_arrayList_create();
    if (asyncEventQueue == NULL) {
        celix_logHelper_error(logHelper, "Failed to create async event queue.");
        return NULL;
    }

    celix_steal_ptr(asyncEventQueue);
    celix_steal_ptr(cond);
    celix_steal_ptr(mutex);
    celix_steal_ptr(eventHandlers);
    celix_steal_ptr(channelsMatchingPrefixTopic);
    celix_steal_ptr(channelsMatchingTopic);
    celix_steal_ptr(channelMatchingAllEvents);
    celix_steal_ptr(lock);
    celix_steal_ptr(logHelper);

    return celix_steal_ptr(ea);
}

int celix_eventAdmin_start(celix_event_admin_t* ea) {
    celix_status_t status = CELIX_SUCCESS;
    ea->threadsRunning = true;
    for (int thCnt = 0; thCnt < CELIX_EVENT_ADMIN_MAX_HANDLER_THREADS; ++thCnt) {
        status = celixThread_create(&ea->eventHandlerThreads[thCnt], NULL, celix_eventAdmin_deliverEventThread, ea);
        if (status != CELIX_SUCCESS) {
            celix_logHelper_error(ea->logHelper, "Failed to create event handler thread.");
            celixThreadMutex_lock(&ea->eventsMutex);
            ea->threadsRunning = false;
            celixThreadMutex_unlock(&ea->eventsMutex);
            celixThreadCondition_broadcast(&ea->eventsTriggerCond);
            for (int i = 0; i < thCnt; i++) {
                celixThread_join(ea->eventHandlerThreads[i], NULL);
            }
            return status;
        }
        celixThread_setName(&ea->eventHandlerThreads[thCnt], "EventHandler");
    }

    return CELIX_SUCCESS;
}

int celix_eventAdmin_stop(celix_event_admin_t* ea) {
    celixThreadMutex_lock(&ea->eventsMutex);
    ea->threadsRunning = false;
    celixThreadMutex_unlock(&ea->eventsMutex);
    celixThreadCondition_broadcast(&ea->eventsTriggerCond);
    for (int i = 0; i < CELIX_EVENT_ADMIN_MAX_HANDLER_THREADS; ++i) {
        celixThread_join(ea->eventHandlerThreads[i], NULL);
    }
    return CELIX_SUCCESS;
}

void celix_eventAdmin_destroy(celix_event_admin_t* ea) {
    assert(ea != NULL);
    size_t size = celix_arrayList_size(ea->asyncEventQueue);
    for (int i = 0; i < size; ++i) {
        celix_event_entry_t *entry = celix_arrayList_get(ea->asyncEventQueue, i);
        celix_event_release(entry->event);
        celix_longHashMap_destroy(entry->eventHandlers);
        free(entry);
    }
    celix_arrayList_destroy(ea->asyncEventQueue);
    celixThreadCondition_destroy(&ea->eventsTriggerCond);
    celixThreadMutex_destroy(&ea->eventsMutex);
    assert(celix_longHashMap_size(ea->eventHandlers) == 0);
    celix_longHashMap_destroy(ea->eventHandlers);
    assert(celix_stringHashMap_size(ea->channelsMatchingPrefixTopic) == 0);
    celix_stringHashMap_destroy(ea->channelsMatchingPrefixTopic);
    assert(celix_stringHashMap_size(ea->channelsMatchingTopic) == 0);
    celix_stringHashMap_destroy(ea->channelsMatchingTopic);
    celix_arrayList_destroy(ea->channelMatchingAllEvents.eventHandlerSvcIdList);
    celixThreadRwlock_destroy(&ea->lock);
    celix_logHelper_destroy(ea->logHelper);
    free(ea);
    return;
}

static void celix_eventAdmin_addEventHandlerToChannels(celix_event_admin_t* ea, const char* topic,
                                                       celix_event_handler_t* eventHandler, celix_string_hash_map_t* channels) {
    celix_event_channel_t* channel = celix_stringHashMap_get(channels, topic);
    if (channel == NULL) {
        celix_autofree celix_event_channel_t *_channel = channel = calloc(1, sizeof(*channel));
        if (_channel == NULL) {
            celix_logHelper_error(ea->logHelper, "Failed to create event channel for topic %s", topic);
            return;
        }
        celix_autoptr(celix_array_list_t) eventHandlerSvcIdList = channel->eventHandlerSvcIdList = celix_arrayList_create();
        if (eventHandlerSvcIdList == NULL) {
            celix_logHelper_error(ea->logHelper, "Failed to create event handlers list for topic %s", topic);
            return;
        }
        if (celix_arrayList_addLong(channel->eventHandlerSvcIdList, eventHandler->serviceId) != CELIX_SUCCESS) {
            celix_logHelper_error(ea->logHelper, "Failed to subscribe %s for event handler(%s).", topic, eventHandler->serviceDescription);
            return;
        }
        if (celix_stringHashMap_put(channels, topic, channel) != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(ea->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(ea->logHelper, "Failed to add event channel for topic %s", topic);
            return;
        }
        celix_steal_ptr(eventHandlerSvcIdList);
        celix_steal_ptr(_channel);
    } else if (celix_arrayList_addLong(channel->eventHandlerSvcIdList, eventHandler->serviceId) != CELIX_SUCCESS) {
        celix_logHelper_error(ea->logHelper, "Failed to subscribe %s for event handler(%s).", topic, eventHandler->serviceDescription);
    }
    return;
}

static void celix_eventAdmin_subscribeTopicFor(celix_event_admin_t* ea, const char* topic, celix_event_handler_t* eventHandler) {
    size_t topicLen = celix_utils_strlen(topic);
    if (celix_utils_stringEquals(topic, "*")) {
        celix_event_channel_t *channel = &ea->channelMatchingAllEvents;
        if (celix_arrayList_addLong(channel->eventHandlerSvcIdList, eventHandler->serviceId) != CELIX_SUCCESS) {
            celix_logHelper_error(ea->logHelper, "Failed to subscribe %s for event handler(%s).", topic, eventHandler->serviceDescription);
            return;
        }
    } else if (topicLen > 2 && topic[topicLen-1] == '*' && topic[topicLen-2] == '/'){
        char prefixTopic[256] = {0};
        if (topicLen-2 >= sizeof(prefixTopic)) {
            celix_logHelper_error(ea->logHelper, "Prefix topic %s too long.", topic);
            return;
        }
        memcpy(prefixTopic, topic, topicLen - 2);
        prefixTopic[topicLen - 2] = '\0';
        celix_eventAdmin_addEventHandlerToChannels(ea, prefixTopic, eventHandler, ea->channelsMatchingPrefixTopic);
    } else {
        celix_eventAdmin_addEventHandlerToChannels(ea, topic, eventHandler, ea->channelsMatchingTopic);
    }
    return;
}

static void celix_eventAdmin_removeEventHandlerFromChannels(celix_event_admin_t* ea, celix_event_handler_t* eventHandler, celix_string_hash_map_t* channels) {
    (void)ea;//unused
    celix_string_hash_map_iterator_t iter = celix_stringHashMap_begin(channels);
    while (!celix_stringHashMapIterator_isEnd(&iter)) {
        celix_event_channel_t *channel = iter.value.ptrValue;
        celix_arrayList_removeLong(channel->eventHandlerSvcIdList, eventHandler->serviceId);
        if (celix_arrayList_size(channel->eventHandlerSvcIdList) == 0) {
            celix_stringHashMapIterator_remove(&iter);
            celix_arrayList_destroy(channel->eventHandlerSvcIdList);
            free(channel);
        } else {
            celix_stringHashMapIterator_next(&iter);
        }
    }
}

static void celix_eventAdmin_unsubscribeTopicFor(celix_event_admin_t* ea, celix_event_handler_t* eventHandler) {
    celix_arrayList_removeLong(ea->channelMatchingAllEvents.eventHandlerSvcIdList, eventHandler->serviceId);
    celix_eventAdmin_removeEventHandlerFromChannels(ea, eventHandler, ea->channelsMatchingPrefixTopic);
    celix_eventAdmin_removeEventHandlerFromChannels(ea, eventHandler, ea->channelsMatchingTopic);
    return;
}

int celix_eventAdmin_addEventHandlerWithProperties(void* handle, void* svc, const celix_properties_t* props) {
    assert(handle != NULL);
    assert(svc != NULL);
    assert(props != NULL);
    celix_event_admin_t* ea = (celix_event_admin_t*)handle;
    const char *topics = celix_properties_get(props, CELIX_EVENT_TOPIC, NULL);
    if (topics == NULL) {
        celix_logHelper_error(ea->logHelper, "Failed to add event handler. No %s property set", CELIX_EVENT_TOPIC);
        return CELIX_ILLEGAL_ARGUMENT;
    }
    long serviceId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
    if (serviceId < 0) {
        celix_logHelper_error(ea->logHelper, "Event handler service id is empty.");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_autofree celix_event_handler_t *handler = calloc(1, sizeof(*handler));
    if (handler == NULL) {
        celix_logHelper_error(ea->logHelper, "Failed to create event handler.");
        return CELIX_ENOMEM;
    }
    handler->service = svc;
    handler->serviceId = serviceId;
    handler->serviceDescription = celix_properties_get(props, CELIX_FRAMEWORK_SERVICE_DESCRIPTION, "Unknown");
    const char *deliver = celix_properties_get(props, CELIX_EVENT_DELIVERY, NULL);
    handler->asyncOrdered = (deliver == NULL || strstr(deliver,CELIX_EVENT_DELIVERY_ASYNC_ORDERED) != NULL);
    handler->eventFilter = NULL;
    handler->blackListed = false;
    handler->handlingAsyncEventCnt = 0;

    celix_autofree char* topicsCopy = celix_utils_strdup(topics);
    if (topicsCopy == NULL) {
        celix_logHelper_error(ea->logHelper, "Failed to dup topics %s.", topics);
        return CELIX_ENOMEM;
    }

    celix_autoptr(celix_string_hash_map_t) topicsMap = celix_stringHashMap_create();//avoid topic duplicates
    if (topicsMap == NULL) {
        celix_logHelper_logTssErrors(ea->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(ea->logHelper, "Failed to create topics map.");
        return CELIX_ENOMEM;
    }
    char* savePtr = NULL;
    char* token = strtok_r(topicsCopy, ",", &savePtr);
    while (token != NULL) {
        char *trimmedToken = celix_utils_trimInPlace(token);
        if (celix_stringHashMap_put(topicsMap, trimmedToken, NULL) != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(ea->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(ea->logHelper, "Failed to add topic %s to topics map.", trimmedToken);
            return CELIX_ENOMEM;
        }
        token = strtok_r(NULL, ",", &savePtr);
    }

    celix_autoptr(celix_filter_t) eventFilter = NULL;
    const char* eventFilterStr = celix_properties_get(props, CELIX_EVENT_FILTER, NULL);
    if (eventFilterStr) {
        eventFilter = handler->eventFilter = celix_filter_create(eventFilterStr);
    }

    celix_auto(celix_rwlock_wlock_guard_t) wLockGuard = celixRwlockWlockGuard_init(&ea->lock);
    celix_status_t status = celix_longHashMap_put(ea->eventHandlers, handler->serviceId, handler);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_logTssErrors(ea->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(ea->logHelper, "Failed to add event handler(%s).", handler->serviceDescription);
        return status;
    }

    CELIX_STRING_HASH_MAP_ITERATE(topicsMap, iter) {
        celix_eventAdmin_subscribeTopicFor(ea, iter.key, handler);
    }

    celix_logHelper_debug(ea->logHelper, "Added event handler(%s) for topics %s", handler->serviceDescription, topics);

    celix_steal_ptr(eventFilter);
    celix_steal_ptr(handler);

    return CELIX_SUCCESS;
}

int celix_eventAdmin_removeEventHandlerWithProperties(void* handle, void* svc, const celix_properties_t* props) {
    assert(handle != NULL);
    assert(svc != NULL);
    assert(props != NULL);
    celix_event_admin_t* ea = handle;

    long serviceId = celix_properties_getAsLong(props, CELIX_FRAMEWORK_SERVICE_ID, -1L);
    if (serviceId < 0) {
        celix_logHelper_error(ea->logHelper, "Event handler service id is empty.");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_auto(celix_rwlock_wlock_guard_t) wLockGuard = celixRwlockWlockGuard_init(&ea->lock);
    celix_event_handler_t *handler = celix_longHashMap_get(ea->eventHandlers, serviceId);
    if (handler != NULL) {
        celix_logHelper_debug(ea->logHelper, "Removing event handler(%s)", handler->serviceDescription);
        celix_eventAdmin_unsubscribeTopicFor(ea, handler);
        celix_longHashMap_remove(ea->eventHandlers, serviceId);
        celix_filter_destroy(handler->eventFilter);
        free(handler);
    }

    return CELIX_SUCCESS;
}

static void celix_eventAdmin_collectEventHandlers(celix_event_admin_t* ea, const char* eventTopic, const celix_properties_t* eventProperties,
                                                  celix_event_channel_t* channel, celix_long_hash_map_t* eventHandlers) {
    if (channel == NULL) {
        return;
    }
    size_t size = celix_arrayList_size(channel->eventHandlerSvcIdList);
    for (int i = 0; i < size; ++i) {
        long handlerSvcId = celix_arrayList_getLong(channel->eventHandlerSvcIdList, i);
        celix_event_handler_t* eventHandler = celix_longHashMap_get(ea->eventHandlers, handlerSvcId);
        if (eventHandler == NULL) {
            continue;
        }
        if (!celix_filter_match(eventHandler->eventFilter, eventProperties)) {
            celix_logHelper_debug(ea->logHelper, "Event %s is filtered by filter(%s)", eventTopic,
                                  celix_filter_getFilterString(eventHandler->eventFilter));
            continue;
        }
        if (__atomic_load_n(&eventHandler->blackListed, __ATOMIC_ACQUIRE)) {
            celix_logHelper_warning(ea->logHelper, "Skipping blacklisted event handler for topic %s, %s", eventTopic,
                                    eventHandler->serviceDescription);
            continue;
        }
        if (celix_longHashMap_put(eventHandlers, handlerSvcId, NULL) != CELIX_SUCCESS) {
            celix_logHelper_logTssErrors(ea->logHelper, CELIX_LOG_LEVEL_ERROR);
            celix_logHelper_error(ea->logHelper, "Failed to collect event handler for topic %s, %s", eventTopic,
                                  eventHandler->serviceDescription);
        }
    }
    return;
}

static int celix_eventAdmin_deliverEvent(celix_event_admin_t* ea, const char* eventTopic, const celix_properties_t* eventProperties,
                                          int (*deliverAction)(celix_event_admin_t* ea, const char* topic, const celix_properties_t* properties,
                                                  celix_long_hash_map_t* eventHandlers, bool* stealEventHandlers)) {
    celix_autoptr(celix_long_hash_map_t) eventHandlers = celix_longHashMap_create();//avoid duplicated event handler
    if (eventHandlers == NULL) {
        celix_logHelper_logTssErrors(ea->logHelper, CELIX_LOG_LEVEL_ERROR);
        celix_logHelper_error(ea->logHelper, "Failed to create event handlers list.");
        return CELIX_ENOMEM;
    }

    {
        celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&ea->lock);
        // Add all handlers for matching everything
        celix_eventAdmin_collectEventHandlers(ea, eventTopic, eventProperties, &ea->channelMatchingAllEvents, eventHandlers);

        // Add the handlers for prefix matches
        if (celix_stringHashMap_size(ea->channelsMatchingPrefixTopic) != 0) {
            char topicCopy[256] = {0};
            if (celix_utils_strlen(eventTopic) >= sizeof(topicCopy)) {
                celix_logHelper_error(ea->logHelper, "Topic %s too long.", eventTopic);
            } else {
                strcpy(topicCopy, eventTopic);
                char *pos;
                while ((pos = strrchr(topicCopy, '/')) != NULL) {
                    topicCopy[pos - topicCopy] = '\0';
                    celix_event_channel_t *channel = celix_stringHashMap_get(ea->channelsMatchingPrefixTopic, topicCopy);
                    celix_eventAdmin_collectEventHandlers(ea, eventTopic, eventProperties, channel, eventHandlers);
                }
            }
        }

        // Add the handlers for matching topic names
        celix_event_channel_t* channel = celix_stringHashMap_get(ea->channelsMatchingTopic, eventTopic);
        celix_eventAdmin_collectEventHandlers(ea, eventTopic, eventProperties, channel, eventHandlers);
    }

    if (celix_longHashMap_size(eventHandlers) == 0) {//no event handlers found, discard event
        celix_logHelper_trace(ea->logHelper, "No event handlers found for topic %s", eventTopic);
        return CELIX_SUCCESS;
    }
    bool stealEventHandlers = false;
    celix_status_t status = deliverAction(ea, eventTopic, eventProperties, eventHandlers, &stealEventHandlers);
    if (stealEventHandlers) {
        celix_steal_ptr(eventHandlers);
    }
    return status;
}

static void celix_eventAdmin_deliverEventToHandler(celix_event_admin_t* ea, const char* topic, const celix_properties_t* props,
                                                   celix_event_handler_t* eventHandler) {
    celix_logHelper_trace(ea->logHelper, "Delivering event %s to handler(%s)", topic, eventHandler->serviceDescription);
    celix_event_handler_service_t* svc = eventHandler->service;
    struct timespec startTime = celix_gettime(CLOCK_MONOTONIC);
    svc->handleEvent(svc->handle, topic, props);
    double elapsedTime = celix_elapsedtime(CLOCK_MONOTONIC, startTime);
    if (elapsedTime > CELIX_EVENT_ADMIN_MAX_HANDLE_EVENT_TIME) {
        celix_logHelper_error(ea->logHelper, "Event handler for topic %s took %f seconds, %s", topic, elapsedTime, eventHandler->serviceDescription);
        __atomic_store_n(&eventHandler->blackListed, true, __ATOMIC_RELEASE);
    }
    return;
}

static int celix_eventAdmin_deliverEventSyncDo(celix_event_admin_t* ea, const char* topic, const celix_properties_t* props,
                                               celix_long_hash_map_t* eventHandlers, bool* stealEventHandlers) {
    celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&ea->lock);
    CELIX_LONG_HASH_MAP_ITERATE(eventHandlers, iter) {
        celix_event_handler_t* eventHandler = celix_longHashMap_get(ea->eventHandlers, iter.key);
        if (eventHandler) {
            celix_eventAdmin_deliverEventToHandler(ea, topic, props, eventHandler);
        }
    }
    *stealEventHandlers = false;
    return CELIX_SUCCESS;
}

celix_status_t celix_eventAdmin_sendEvent(void* handle, const char* topic, const celix_properties_t* props) {
    if (handle == NULL || topic == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_event_admin_t* ea = (celix_event_admin_t*)handle;
    return celix_eventAdmin_deliverEvent(ea, topic, props, celix_eventAdmin_deliverEventSyncDo);
}

static int celix_eventAdmin_deliverEventAsyncDo(celix_event_admin_t* ea, const char* topic, const celix_properties_t* props,
                                                 celix_long_hash_map_t* eventHandlers, bool* stealEventHandlers) {
    celix_autofree celix_event_entry_t* entry = calloc(1, sizeof(*entry));
    if (entry == NULL) {
        celix_logHelper_error(ea->logHelper, "Failed to alloc memory for event entry %s.", topic);
        return CELIX_ENOMEM;
    }
    celix_autoptr(celix_event_t) event = entry->event = celix_event_create(topic, props);
    if (event == NULL) {
        celix_logHelper_error(ea->logHelper, "Failed to alloc memory for event %s.", topic);
        return CELIX_ENOMEM;
    }
    entry->eventHandlers = eventHandlers;
    celix_auto(celix_mutex_lock_guard_t) mutexGuard = celixMutexLockGuard_init(&ea->eventsMutex);
    if (celix_arrayList_size(ea->asyncEventQueue) >= CELIX_EVENT_ADMIN_MAX_EVENT_QUEUE_SIZE) {
        celix_logHelper_error(ea->logHelper, "Event queue is full. Dropping event %s.", topic);
        return CELIX_ILLEGAL_STATE;
    }
    int status = celix_arrayList_add(ea->asyncEventQueue, entry);
    if (status != CELIX_SUCCESS) {
        celix_logHelper_error(ea->logHelper, "Failed to add event(%s) to event queue.", topic);
        return status;
    }
    *stealEventHandlers = true;
    celix_steal_ptr(event);
    celix_steal_ptr(entry);
    celixThreadCondition_signal(&ea->eventsTriggerCond);

    return CELIX_SUCCESS;
}

celix_status_t celix_eventAdmin_postEvent(void* handle, const char* topic, const celix_properties_t* props) {
    if (handle == NULL || topic == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    celix_event_admin_t* ea = (celix_event_admin_t*)handle;
    return celix_eventAdmin_deliverEvent(ea, topic, props, celix_eventAdmin_deliverEventAsyncDo);
}

static bool celix_eventAdmin_getPendingEvent(celix_event_admin_t* ea, celix_event_t** event, long* eventHandlerSvcId) {
    bool notFound = true;
    size_t size = celix_arrayList_size(ea->asyncEventQueue);
    for (int i = 0; (i < size) && notFound; ++i) {
        celix_event_entry_t* eventEntry = celix_arrayList_get(ea->asyncEventQueue, i);
        celix_long_hash_map_iterator_t iter = celix_longHashMap_begin(eventEntry->eventHandlers);
        while (!celix_longHashMapIterator_isEnd(&iter) && notFound) {
            celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&ea->lock);
            long handlerSvcId = iter.key;
            celix_event_handler_t* eventHandler = celix_longHashMap_get(ea->eventHandlers, handlerSvcId);
            if (eventHandler == NULL) {
                celix_longHashMapIterator_remove(&iter);
                continue;
            }
            unsigned int handlingEventCnt = __atomic_fetch_add(&eventHandler->handlingAsyncEventCnt, 1, __ATOMIC_SEQ_CST);
            if (handlingEventCnt == 0 || (!eventHandler->asyncOrdered && handlingEventCnt < CELIX_EVENT_ADMIN_MAX_PARALLEL_EVENTS_OF_HANDLER)) {
                *event = celix_event_retain(eventEntry->event);
                *eventHandlerSvcId = handlerSvcId;
                celix_longHashMapIterator_remove(&iter);
                notFound = false;
                continue;
            }
            __atomic_fetch_sub(&eventHandler->handlingAsyncEventCnt, 1, __ATOMIC_SEQ_CST);

            celix_longHashMapIterator_next(&iter);
        }
        if (celix_longHashMap_size(eventEntry->eventHandlers) == 0) {
            celix_arrayList_removeAt(ea->asyncEventQueue, i);
            i--;
            size--;
            celix_event_release(eventEntry->event);
            celix_longHashMap_destroy(eventEntry->eventHandlers);
            free(eventEntry);
        }
    }
    return !notFound;
}

static void celix_eventAdmin_deliverPendingEvent(celix_event_admin_t* ea, const char* topic, const celix_properties_t* props, long eventHandlerSvcId) {
    celix_auto(celix_rwlock_rlock_guard_t) rLockGuard = celixRwlockRlockGuard_init(&ea->lock);
    celix_event_handler_t* eventHandler = celix_longHashMap_get(ea->eventHandlers, eventHandlerSvcId);
    if(eventHandler == NULL) {
        return;
    }
    if (__atomic_load_n(&eventHandler->blackListed, __ATOMIC_ACQUIRE)) {
        celix_logHelper_warning(ea->logHelper, "Skipping blacklisted event handler for topic %s, %s", topic,
                                eventHandler->serviceDescription);
        return;
    }
    celix_eventAdmin_deliverEventToHandler(ea, topic, props, eventHandler);
    __atomic_fetch_sub(&eventHandler->handlingAsyncEventCnt, 1, __ATOMIC_SEQ_CST);
    return;
}

static void* celix_eventAdmin_deliverEventThread(void* data) {
    assert(data != NULL);
    celix_event_admin_t* ea = (celix_event_admin_t*)data;
    bool running = ea->threadsRunning;
    while (running) {
        celix_event_t *event = NULL;
        long eventHandlerSvcId = -1;
        celixThreadMutex_lock(&ea->eventsMutex);
        while (ea->threadsRunning && !celix_eventAdmin_getPendingEvent(ea, &event, &eventHandlerSvcId)) {
            celixThreadCondition_wait(&ea->eventsTriggerCond, &ea->eventsMutex);
        }
        running = ea->threadsRunning;
        celixThreadMutex_unlock(&ea->eventsMutex);

        if (event) {
            celix_eventAdmin_deliverPendingEvent(ea, celix_event_getTopic(event), celix_event_getProperties(event), eventHandlerSvcId);
            celix_event_release(event);
        }
    }
    return NULL;
}