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

#ifndef CELIX_SHELL_COMMAND_H_
#define CELIX_SHELL_COMMAND_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdio.h>

#define CELIX_SHELL_COMMAND_NAME                "command.name"
#define CELIX_SHELL_COMMAND_USAGE               "command.usage"
#define CELIX_SHELL_COMMAND_DESCRIPTION         "command.description"

#define  CELIX_SHELL_COMMAND_SERVICE_NAME       "celix_shell_command"
#define  CELIX_SHELL_COMMAND_SERVICE_VERSION    "1.0.0"

typedef struct celix_shell_command celix_shell_command_t;

/**
 * The shell command can be used to register additional shell commands.
 * This service should be registered with the following properties:
 *  - command.name: mandatory, name of the command e.g. 'lb'
 *  - command.usage: optional, string describing how tu use the command e.g. 'lb [-l | -s | -u]'
 *  - command.description: optional, string describing the command e.g. 'list bundles.'
 */
struct celix_shell_command {
    void *handle;

    /**
     * Calls the shell command.
     * @param handle        The shell command handle.
     * @param commandLine   The complete provided cmd line (e.g. for a 'stop' command -> 'stop 42')
     * @param outStream     The output stream, to use for printing normal flow info.
     * @param errorStream   The error stream, to use for printing error flow info.
     * @return              Whether a command is successfully executed.
     */
    bool (*executeCommand)(void *handle, const char *commandLine, FILE *outStream, FILE *errorStream);
};

#ifdef __cplusplus
}
#endif


#endif /* CELIX_SHELL_COMMAND_H_ */
