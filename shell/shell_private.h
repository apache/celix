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
 *  Created on: Aug 13, 2010
 *      Author: alexanderb
 */

#ifndef SHELL_PRIVATE_H_
#define SHELL_PRIVATE_H_

#include "headers.h"
#include "shell.h"
#include "hash_map.h"
#include "command.h"

struct shell {
	apr_pool_t *pool;
	BUNDLE_CONTEXT bundleContext;
	HASH_MAP commandReferenceMap;
	HASH_MAP commandNameMap;
};

SHELL shell_create();
char * shell_getCommandUsage(SHELL shell, char * commandName);
char * shell_getCommandDescription(SHELL shell, char * commandName);
SERVICE_REFERENCE shell_getCommandReference(SHELL shell, char * command);
void shell_executeCommand(SHELL shell, char * commandLine, void (*out)(char *), void (*error)(char *));

COMMAND shell_getCommand(SHELL shell, char * commandName);


#endif /* SHELL_PRIVATE_H_ */
