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
/*
 * event_admin_impl.h
 *
 *  \Created on: Jul 16, 2013
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef EVENT_ADMIN_IMPL_H_
#define EVENT_ADMIN_IMPL_H_
#include <string.h>
#include <apr.h>
#include <apr_pools.h>
#include "celix_errno.h"
#include "bundle_context.h"
#include "constants.h"
#include "event_constants.h"
#include "event_admin.h"
#include "event_handler.h"
#include "bundle_activator.h"
#include "bundle_context.h"
#include "service_tracker.h"
#include "service_listener.h"
#include "service_registration.h"
#include "listener_hook_service.h"
#include "event_admin.h"

struct event_admin {
        apr_pool_t *pool;
        hash_map_pt channels;
        array_list_pt event_handlers;
        bundle_context_pt context;
};
typedef struct channel *channel_t;
struct channel {
        char *topic;
        hash_map_pt eventHandlers;///array list containing all listeners subscribed to the channel
       // hash_map_pt channels;
        apr_thread_mutex_t *channelLock;

};
/**
 * @desc Create event an event admin and put it in the event_admin parameter.
 * @param apr_pool_t *pool. Pointer to the apr pool
 * @param bundle_context_pt context. Pointer to the bundle context.
 * @param event_admin_pt *event_admin. The event admin result.
 */
celix_status_t eventAdmin_create(apr_pool_t *pool, bundle_context_pt context, event_admin_pt *event_admin);
/**
 * @desc Post event. sends the event to the handlers in async.
 * @param event_admin_pt event_admin. the event admin instance
 * @param event_pt event. the event to be send.
 *
 */
celix_status_t eventAdmin_postEvent(event_admin_pt event_admin, event_pt event);// async event sending
/**
 * @desc send event. sends the event to the handlers in sync.
 * @param event_admin_pt event_admin. the event admin instance
 * @param event_pt event. the event to be send.
 *
 */
celix_status_t eventAdmin_sendEvent(event_admin_pt event_admin, event_pt event);// sync event sending
/**
 * @desc functions for the service tracker
 * @param void * handle.  Pointer to the event admin.
 * @param service_reference_pt ref. Pointer to the service reference. Needed to get the service
 * @param void **service Pointer to the service. Needed to use the service.
 */
celix_status_t eventAdmin_addingService(void * handle, service_reference_pt ref, void **service);
celix_status_t eventAdmin_addedService(void * handle, service_reference_pt ref, void * service);
celix_status_t eventAdmin_modifiedService(void * handle, service_reference_pt ref, void * service);
celix_status_t eventAdmin_removedService(void * handle, service_reference_pt ref, void * service) ;
/*
 * end functions for service tracker
 */

/**
 * @desc finds the handlers interested in the topic.
 * @param hash_map_pt channels. hashmap contains the channels, key string based
 * @param char *topic, the topic string.
 * @param array_list_pt event_handlers. The array list to contain the interested handlers.
 */
celix_status_t eventAdmin_findHandlersByTopic(hash_map_pt channels, char *topic , array_list_pt event_handlers);
/**
 * @desc create the needed event channels for an event handler.
 * @desc apr_pool_t *pool. a memory pool pointer.
 * @desc event_handler_service_pt event_handler_service. The handler
 * @desc char *topic the topic
 * @desc channel_t *channel. the top level channel.
 */
celix_status_t eventAdmin_createEventChannelsByEventHandler(apr_pool_t *pool,event_handler_service_pt event_handler_service, char *topic, channel_t *channel);
/**
 * @desc mutex functions for the channels
 * @param event_admin_pt event_admin. the event admin instance.
 * @param char *topic. the topic for which the channels need to be locked or unlocked
 */
celix_status_t eventAdmin_lockHandlersList(event_admin_pt event_admin, char *topic );
celix_status_t eventAdmin_releaseHandersList(event_admin_pt event_admin, char *topic );

/**
 * @desc create an event
 * @param char *topic. String containing the topic
 * @param properties_pt properties.
 */
celix_status_t eventAdmin_createEvent(event_admin_pt *event_admin, char *topic, properties_pt properties, event_pt *event);
/**
 * @desc checks if an event contains the property
 * @param event_pt *event. the event to check
 * @param char *property. the key for the property to check
 * @param bool *result. the result.
 */
celix_status_t eventAdmin_containsProperty( event_pt *event, char *property, bool *result);
/**
 * @desc checks if an event is equal to the second event.
 * @param event_pt *event, event to compare to
 * @param event_pt *event, the event to be compared
 * @param bool *result. the result true if equal.
 */
celix_status_t eventAdmin_event_equals( event_pt *event, event_pt *compare, bool *result);
/**
 * @desc gets a property from the event.
 * @param event_pt *event. the event to get the property from
 * @param char *propertyKey the key of the property to get
 * @param char **propertyValue. the result param will contain the property if it exists in the event.
 */
celix_status_t eventAdmin_getProperty( event_pt *event, char *propertyKey, char **propertyValue);
/**
 * @desc gets all property names from the event
 * @param event_pt *event. the event to get the property names from
 * @param array_list_pt *names. will contain the keys
 */
celix_status_t eventAdmin_getPropertyNames( event_pt *event, array_list_pt *names);
/**
 * @desc gets the topic from an event
 * @param event_pt *event. the event to get the topic from
 * @param char **topic, result pointer will contain the topic.
 */
celix_status_t eventAdmin_getTopic( event_pt *event, char **topic);
celix_status_t eventAdmin_hashCode(event_pt *event, int *hashCode);
celix_status_t eventAdmin_matches( event_pt *event);
celix_status_t eventAdmin_toString( event_pt *event, char *eventString);


#endif /* EVENT_ADMIN_IMPL_H_ */
