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
 * remote_shell.h
 *
 *  \date       Nov 4, 2012
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_SHELL_H_
#define REMOTE_SHELL_H_

#include <bundle_context.h>
#include <celix_errno.h>

#include "shell_mediator.h"

struct remote_shell {
	celix_log_helper_t **loghelper;
	shell_mediator_pt mediator;
	celix_thread_mutex_t mutex;
	int maximumConnections;

	celix_array_list_t* connections;
};
typedef struct remote_shell *remote_shell_pt;

celix_status_t remoteShell_create(shell_mediator_pt mediator, int maximumConnections, remote_shell_pt *instance);
celix_status_t remoteShell_destroy(remote_shell_pt instance);
celix_status_t remoteShell_addConnection(remote_shell_pt instance, int socket);
celix_status_t remoteShell_stopConnections(remote_shell_pt instance);

#endif /* REMOTE_SHELL_H_ */
