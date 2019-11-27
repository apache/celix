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
/**
 * log_store.c
 *
 *  \date       Apr 18, 2012
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include <time.h>

#include "celix_errno.h"
#include "array_list.h"

#include "log_store.h"
#include "log.h"

struct log_store {
	unsigned long storeId;

	array_list_pt logEvents;
};

static celix_status_t logStore_getNextID(log_store_pt store, unsigned long *id);

celix_status_t logStore_create(log_store_pt *store) {
	celix_status_t status = CELIX_SUCCESS;
	*store = calloc(1, sizeof(**store));
	if (!*store) {
		status = CELIX_ENOMEM;
	} else {
		(*store)->storeId = 1;
		arrayList_create(&(*store)->logEvents);
	}

	return status;
}

celix_status_t logStore_put(log_store_pt store, unsigned int type, properties_pt properties, log_event_pt *event) {
	celix_status_t status = CELIX_SUCCESS;

	*event = calloc(1, sizeof(**event));
	(*event)->targetId = NULL;
	(*event)->logId = store->storeId;
	(*event)->id = 0;
	(*event)->time = time(NULL);
	(*event)->type = type;
	(*event)->properties = properties;

	logStore_getNextID(store, &(*event)->id);

	arrayList_add(store->logEvents, *event);

	return status;
}

celix_status_t logStore_getLogId(log_store_pt store, unsigned long *id) {
	*id = store->storeId;
	return CELIX_SUCCESS;
}

celix_status_t logStore_getEvents(log_store_pt store, array_list_pt *events) {
	*events = store->logEvents;
	return CELIX_SUCCESS;
}

celix_status_t logStore_getHighestId(log_store_pt store, long *id) {
	*id = ((long) arrayList_size(store->logEvents)) - 1;
	return CELIX_SUCCESS;
}

static celix_status_t logStore_getNextID(log_store_pt store, unsigned long *id) {
	*id = arrayList_size(store->logEvents);
	return CELIX_SUCCESS;
}

