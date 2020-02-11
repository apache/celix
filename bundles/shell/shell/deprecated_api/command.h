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

#ifndef COMMAND_H_
#define COMMAND_H_

#include "celix_errno.h"
#include <stdio.h>

#define OSGI_SHELL_COMMAND_NAME "command.name"
#define OSGI_SHELL_COMMAND_USAGE "command.usage"
#define OSGI_SHELL_COMMAND_DESCRIPTION "command.description"

#define OSGI_SHELL_COMMAND_SERVICE_NAME "commandService"
#define OSGI_SHELL_COMMAND_SERVICE_VERSION "1.0.0"

typedef struct commandService command_service_t;
typedef command_service_t * command_service_pt;

#ifdef CELIX_ADD_DEPRECATED_ATTRIBUTES
#define DEP_ATTRIBUTE __attribute__((deprecated("command_service_t is replaced by celix_shell_command_t in celix_shell_command.h")))
#else
#define DEP_ATTRIBUTE
#endif

/**
 * The command service can be used to register additional shell commands.
 * The service should be register with the following properties:
 *  - command.name: mandatory, name of the command e.g. 'lb'
 *  - command.usage: optional, string describing how tu use the command e.g. 'lb [-l | -s | -u]'
 *  - command.description: optional, string describing the command e.g. 'list bundles.'
 *
 *  \deprecated Replaced by celix_shell_command_t
 */
struct commandService {
    void *handle;

    celix_status_t (*executeCommand)(void *handle, char * commandLine, FILE *outStream, FILE *errorStream) DEP_ATTRIBUTE;
};


#endif /* COMMAND_H_ */
