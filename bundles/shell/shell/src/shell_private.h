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
 * shell_private.h
 *
 *  \date       Aug 13, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SHELL_PRIVATE_H_
#define SHELL_PRIVATE_H_

#include "bundle_context.h"
#include "shell.h"
#include "hash_map.h"
#include "command.h"
#include "log_helper.h"

struct shell {
	bundle_context_pt bundle_context_ptr;
	hash_map_pt command_reference_map_ptr;
	hash_map_pt command_name_map_ptr;
	log_helper_pt logHelper;
};

celix_status_t shell_create(bundle_context_pt context_ptr, shell_service_pt *shell_service_ptr);
celix_status_t shell_destroy(shell_service_pt *shell_service_ptr);
celix_status_t shell_addCommand(shell_pt shell_ptr, service_reference_pt reference_ptr, void *svc);
celix_status_t shell_removeCommand(shell_pt shell_ptr, service_reference_pt reference_ptr, void *svc);

celix_status_t shell_getCommandReference(shell_pt shell_ptr, char *command_name_str, service_reference_pt *command_reference_ptr);
celix_status_t shell_executeCommand(shell_pt shell_ptr, char *command_line_str, FILE *out, FILE *err);

#endif /* SHELL_PRIVATE_H_ */
