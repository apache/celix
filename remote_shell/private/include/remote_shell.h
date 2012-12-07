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
 * remote_shell.h
 *
 *  \date       Nov 4, 2012
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef REMOTE_SHELL_H_
#define REMOTE_SHELL_H_

#include "shell_mediator.h"

#include <apr_pools.h>

#include <bundle_context.h>
#include <celix_errno.h>


typedef struct remote_shell *remote_shell_t;

celix_status_t remoteShell_create(apr_pool_t *pool, shell_mediator_t mediator, apr_int64_t maximumConnections, remote_shell_t *instance);

celix_status_t remoteShell_addConnection(remote_shell_t instance, apr_socket_t *socket);

celix_status_t remoteShell_stopConnections(remote_shell_t instance);

#endif /* REMOTE_SHELL_H_ */
