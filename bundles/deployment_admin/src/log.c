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
 * log.c
 *
 *  \date       Apr 19, 2012
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stddef.h>
#include <stdlib.h>

#include "celix_errno.h"

#include "log.h"
#include "log_store.h"

struct log {
	log_store_pt logStore;
};

celix_status_t log_create(log_store_pt store, log_t **log) {
	celix_status_t status = CELIX_SUCCESS;

	*log = calloc(1, sizeof(**log));
	if (!*log) {
		status = CELIX_ENOMEM;
	} else {
		(*log)->logStore = store;
	}

	return status;
}

celix_status_t log_destroy(log_t **log) {
	free(*log);
	return CELIX_SUCCESS;
}

celix_status_t log_log(log_t *log, unsigned int type, properties_pt properties) {
	celix_status_t status;

	log_event_pt event = NULL;

	status = logStore_put(log->logStore, type, properties, &event);

	return status;
}

celix_status_t log_bundleChanged(void * listener, bundle_event_pt event) {
	return CELIX_SUCCESS;
}

celix_status_t log_frameworkEvent(void * listener, framework_event_pt event) {
	return CELIX_SUCCESS;
}
