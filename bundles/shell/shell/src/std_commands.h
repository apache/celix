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

bool lbCommand_execute(void *handle, const char* commandLine, FILE *outhandle, FILE *errhandle);
bool queryCommand_execute(void *handle, const char* commandLine, FILE *outhandle, FILE *errhandle);
bool startCommand_execute(void *handle, const char* commandLine, FILE *outhandle, FILE *errhandle);
bool stopCommand_execute(void *handle, const char* commandLine, FILE *outStream, FILE *errStream);
bool installCommand_execute(void *handle, const char* commandLine, FILE *outStream, FILE *errStream);
bool uninstallCommand_execute(void *handle, const char* commandLine, FILE *outStream, FILE *errStream);
bool updateCommand_execute(void *handle, const char* commandLine, FILE *outStream, FILE *errStream);
bool helpCommand_execute(void *handle, const char* commandLine, FILE *outStream, FILE *errStream);
bool dmListCommand_execute(void* handle, const char* commandLine, FILE *out, FILE *err);
bool quitCommand_execute(void *handle, const char* commandLine, FILE *sout, FILE *serr);


#endif
