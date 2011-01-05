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
 *  Created on: Aug 12, 2010
 *      Author: alexanderb
 */

#ifndef SHELL_H_
#define SHELL_H_

#include "headers.h"
#include "array_list.h"

static const char * const SHELL_SERVICE_NAME = "shellService";

typedef struct shell * SHELL;

struct shellService {
	SHELL shell;
	ARRAY_LIST (*getCommands)(SHELL shell);
	char * (*getCommandUsage)(SHELL shell, char * commandName);
	char * (*getCommandDescription)(SHELL shell, char * commandName);
	SERVICE_REFERENCE (*getCommandReference)(SHELL shell, char * command);
	void (*executeCommand)(SHELL shell, char * commandLine, void (*out)(char *), void (*error)(char *));
};

typedef struct shellService * SHELL_SERVICE;

#endif /* SHELL_H_ */
