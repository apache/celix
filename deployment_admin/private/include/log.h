/*
 * log.h
 *
 *  Created on: Apr 18, 2012
 *      Author: alexander
 */

#ifndef LOG_H_
#define LOG_H_

#include <apr_general.h>

#include "log_event.h"
#include "log_store.h"

typedef struct log *log_t;

celix_status_t log_create(apr_pool_t *pool, log_store_t store, log_t *log);
celix_status_t log_log(log_t log, unsigned int type, PROPERTIES properties);

#endif /* LOG_H_ */
