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
 * shell.h
 *
 *  \date       Aug 12, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SHELL_H_
#define SHELL_H_

#include "array_list.h"
#include "service_reference.h"

static const char * const OSGI_SHELL_SERVICE_NAME = "shellService";

typedef struct shell * shell_pt;

struct shellService {
	shell_pt shell;

	celix_status_t (*getCommands)(shell_pt shell_ptr, array_list_pt *commands_ptr);
	celix_status_t (*getCommandUsage)(shell_pt shell_ptr, char *command_name_str, char **usage_str);
	celix_status_t (*getCommandDescription)(shell_pt shell_ptr, char *command_name_str, char **command_description_str);
	celix_status_t (*getCommandReference)(shell_pt shell_ptr, char *command_name_str, service_reference_pt *command_reference_ptr);
	celix_status_t (*executeCommand)(shell_pt shell_ptr, char * command_line_str, FILE *out, FILE *err);
};

typedef struct shellService * shell_service_pt;

#endif /* SHELL_H_ */
