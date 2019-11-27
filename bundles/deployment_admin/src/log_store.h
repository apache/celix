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
 * log_store.h
 *
 *  \date       Apr 18, 2012
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LOG_STORE_H_
#define LOG_STORE_H_

#include "log_event.h"

#include "properties.h"
#include "array_list.h"

typedef struct log_store *log_store_pt;

celix_status_t logStore_create(log_store_pt *store);
celix_status_t logStore_put(log_store_pt store, unsigned int type, properties_pt properties, log_event_pt *event);

celix_status_t logStore_getLogId(log_store_pt store, unsigned long *id);
celix_status_t logStore_getEvents(log_store_pt store, array_list_pt *events);

celix_status_t logStore_getHighestId(log_store_pt store, long *id);

#endif /* LOG_STORE_H_ */
