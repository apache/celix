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


#ifndef CELIX_SHELL_H_
#define CELIX_SHELL_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdio.h>
#include "celix_array_list.h"

#define CELIX_SHELL_SERVICE_NAME        "celix_shell"
#define CELIX_SHELL_SERVICE_VERSION     "2.0.0"

struct celix_shell {
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
	 * Try to execute a command using the provided command line.
	 */
	celix_status_t (*executeCommand)(void *handle, const char *commandLine, FILE *out, FILE *err);
};

typedef struct celix_shell celix_shell_t;

#ifdef __cplusplus
}
#endif

#endif /* CELIX_SHELL_H_ */
