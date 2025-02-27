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
 * shell_mediator.h
 *
 *  \date       Nov 4, 2012
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef shellMediator_H_
#define shellMediator_H_


#include <bundle_context.h>
#include <service_tracker.h>
#include <celix_errno.h>

#include "celix_log_helper.h"
#include "celix_shell.h"
#include "celix_threads.h"

struct shell_mediator {
    celix_log_helper_t *loghelper;
	bundle_context_pt context;
	service_tracker_pt tracker;
	celix_thread_mutex_t mutex;

	//protected by mutex
	celix_shell_t *shellService;
};
typedef struct shell_mediator *shell_mediator_pt;

celix_status_t shellMediator_create(bundle_context_pt context, shell_mediator_pt *instance);
celix_status_t shellMediator_stop(shell_mediator_pt instance);
celix_status_t shellMediator_destroy(shell_mediator_pt instance);
celix_status_t shellMediator_executeCommand(shell_mediator_pt instance, char *command, FILE *out, FILE *err);

#endif /* shellMediator_H_ */
