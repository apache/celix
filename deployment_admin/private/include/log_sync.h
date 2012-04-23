/*
 * log_sync.h
 *
 *  Created on: Apr 19, 2012
 *      Author: alexander
 */

#ifndef LOG_SYNC_H_
#define LOG_SYNC_H_

#include "log_store.h"

typedef struct log_sync *log_sync_t;

celix_status_t logSync_create(apr_pool_t *pool, char *targetId, log_store_t store, log_sync_t *logSync);

#endif /* LOG_SYNC_H_ */
