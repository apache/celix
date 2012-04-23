/*
 * log.c
 *
 *  Created on: Apr 19, 2012
 *      Author: alexander
 */
#include <apr_general.h>

#include "celix_errno.h"

#include "log.h"
#include "log_store.h"

struct log {
	log_store_t logStore;
};

celix_status_t log_create(apr_pool_t *pool, log_store_t store, log_t *log) {
	celix_status_t status = CELIX_SUCCESS;

	*log = apr_palloc(pool, sizeof(**log));
	if (!*log) {
		status = CELIX_ENOMEM;
	} else {
		(*log)->logStore = store;
	}

	return status;
}

celix_status_t log_log(log_t log, unsigned int type, PROPERTIES properties) {
	celix_status_t status = CELIX_SUCCESS;

	log_event_t event = NULL;

	status = logStore_put(log->logStore, type, properties, &event);

	return status;
}
