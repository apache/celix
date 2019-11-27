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
 * log.h
 *
 *  \date       Jun 26, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LOG_H_
#define LOG_H_

#include "linked_list.h"
#include "log_entry.h"
#include "log_listener.h"

typedef struct log log_t;

celix_status_t log_create(int max_size, bool store_debug, log_t **logger);
celix_status_t log_destroy(log_t *logger);
celix_status_t log_addEntry(log_t *log, log_entry_t *entry);
celix_status_t log_getEntries(log_t *log, linked_list_pt *list);

celix_status_t log_bundleChanged(void *listener, celix_bundle_event_t *event);
celix_status_t log_frameworkEvent(void *listener, framework_event_pt event);

celix_status_t log_addLogListener(log_t *logger, log_listener_t *listener);
celix_status_t log_removeLogListener(log_t *logger, log_listener_t *listener);
celix_status_t log_removeAllLogListener(log_t *logger);

#endif /* LOG_H_ */
