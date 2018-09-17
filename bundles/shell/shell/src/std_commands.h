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
 * std_commands.h
 *
 *  \date       March 27, 2014
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef __STD_COMMANDS_H_
#define __STD_COMMANDS_H_

#include "celix_errno.h"

#define OSGI_SHELL_COMMAND_SEPARATOR " "

celix_status_t lbCommand_execute(void *_ptr, char *command_line_str, FILE *out_ptr, FILE *err_ptr);
celix_status_t startCommand_execute(void *_ptr, char *command_line_str, FILE *out_ptr, FILE *err_ptr);
celix_status_t stopCommand_execute(void *handle, char * commandline, FILE *outStream, FILE *errStream);
celix_status_t installCommand_execute(void *handle, char * commandline, FILE *outStream, FILE *errStream);
celix_status_t uninstallCommand_execute(void *handle, char * commandline, FILE *outStream, FILE *errStream);
celix_status_t updateCommand_execute(void *handle, char * commandline, FILE *outStream, FILE *errStream);
celix_status_t logCommand_execute(void *handle, char * commandline, FILE *outStream, FILE *errStream);
celix_status_t inspectCommand_execute(void *handle, char * commandline, FILE *outStream, FILE *errStream);
celix_status_t helpCommand_execute(void *handle, char * commandline, FILE *outStream, FILE *errStream);
celix_status_t dmListCommand_execute(void* handle, char * line, FILE *out, FILE *err);


#endif
