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
static const char * const OSGI_SHELL_SERVICE_VERSION = "2.0.0";

struct shellService {
	void *handle;

	/**
	 * List the registered command names. Caller is owner of the commands.
	 * @return A celix array list with char*.
	 */
	celix_status_t (*getCommands)(void *handle, celix_array_list_t **commands);

	/**
	 * Gets the usage info for the provided command str. Caller is owner.
	 */
	celix_status_t (*getCommandUsage)(void *handle, const char *commandName, char **UsageStr);

    /**
     * Gets the usage info for the provided command str. Caller is owner.
     */
	celix_status_t (*getCommandDescription)(void *handle, const char *commandName, char **commandDescription);

	/**
	 * Try to execute a commmand using the provided command line.
	 */
	celix_status_t (*executeCommand)(void *handle, const char *commandLine, FILE *out, FILE *err);
};

typedef struct shellService shell_service_t;
typedef shell_service_t* shell_service_pt;

#endif /* SHELL_H_ */
