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
 * logger.h
 *
 *  \date       Jun 26, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LOG_H_
#define LOG_H_

#include <apr_general.h>

#include "linked_list.h"
#include "log_entry.h"
#include "log_listener.h"

typedef struct log * log_pt;

celix_status_t log_create(apr_pool_t *pool, log_pt *logger);
celix_status_t log_addEntry(log_pt log, log_entry_pt entry);
celix_status_t log_getEntries(log_pt log, apr_pool_t *memory_pool, linked_list_pt *list);

celix_status_t log_bundleChanged(void *listener, bundle_event_pt event);
celix_status_t log_frameworkEvent(void *listener, framework_event_pt event);

celix_status_t log_addLogListener(log_pt logger, log_listener_pt listener);
celix_status_t log_removeLogListener(log_pt logger, log_listener_pt listener);
celix_status_t log_removeAllLogListener(log_pt logger);

#endif /* LOG_H_ */
