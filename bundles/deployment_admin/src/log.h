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
 *  \date       Apr 18, 2012
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef LOG_H_
#define LOG_H_

#include "log_event.h"
#include "log_store.h"

#include "bundle_event.h"
#include "framework_event.h"

typedef struct log log_t;

celix_status_t log_create(log_store_pt store, log_t **log);
celix_status_t log_log(log_t *log, unsigned int type, properties_pt properties);

celix_status_t log_bundleChanged(void * listener, bundle_event_pt event);
celix_status_t log_frameworkEvent(void * listener, framework_event_pt event);

#endif /* LOG_H_ */
