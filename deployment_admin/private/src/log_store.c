/*
 * log_store.c
 *
 *  Created on: Apr 18, 2012
 *      Author: alexander
 */

#include <apr_general.h>

#include <time.h>

#include "celix_errno.h"
#include "array_list.h"

#include "log_store.h"
#include "log.h"

struct log_store {
	apr_pool_t *pool;

	unsigned long storeId;

	ARRAY_LIST logEvents;
};

static celix_status_t logStore_getNextID(log_store_t store, unsigned long *id);

celix_status_t logStore_create(apr_pool_t *pool, log_store_t *store) {
	celix_status_t status = CELIX_SUCCESS;
	*store = apr_palloc(pool, sizeof(**store));
	if (!*store) {
		status = CELIX_ENOMEM;
	} else {
		(*store)->pool = pool;
		(*store)->storeId = 1;
		arrayList_create(pool, &(*store)->logEvents);
	}

	return status;
}

celix_status_t logStore_put(log_store_t store, unsigned int type, PROPERTIES properties, log_event_t *event) {
	celix_status_t status = CELIX_SUCCESS;

	*event = apr_palloc(store->pool, sizeof(**event));
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

celix_status_t logStore_getLogId(log_store_t store, unsigned long *id) {
	*id = store->storeId;
	return CELIX_SUCCESS;
}

celix_status_t logStore_getEvents(log_store_t store, ARRAY_LIST *events) {
	*events = store->logEvents;
	return CELIX_SUCCESS;
}

celix_status_t logStore_getHighestId(log_store_t store, long *id) {
	*id = ((long) arrayList_size(store->logEvents)) - 1;
	return CELIX_SUCCESS;
}

static celix_status_t logStore_getNextID(log_store_t store, unsigned long *id) {
	*id = arrayList_size(store->logEvents);
	return CELIX_SUCCESS;
}

