/*
 * log_store.h
 *
 *  Created on: Apr 18, 2012
 *      Author: alexander
 */

#ifndef LOG_STORE_H_
#define LOG_STORE_H_

#include "log_event.h"

#include "properties.h"
#include "array_list.h"

typedef struct log_store *log_store_t;

celix_status_t logStore_create(apr_pool_t *pool, log_store_t *store);
celix_status_t logStore_put(log_store_t store, unsigned int type, PROPERTIES properties, log_event_t *event);

celix_status_t logStore_getLogId(log_store_t store, unsigned long *id);
celix_status_t logStore_getEvents(log_store_t store, ARRAY_LIST *events);

celix_status_t logStore_getHighestId(log_store_t store, long *id);

#endif /* LOG_STORE_H_ */
