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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef SHELL_PRIVATE_H_
#define SHELL_PRIVATE_H_

#include <apr_general.h>

#include "shell.h"
#include "hash_map.h"
#include "command.h"

struct shell {
	apr_pool_t *pool;
	bundle_context_pt bundleContext;
	hash_map_pt commandReferenceMap;
	hash_map_pt commandNameMap;
};

shell_pt shell_create();
char * shell_getCommandUsage(shell_pt shell, char * commandName);
char * shell_getCommandDescription(shell_pt shell, char * commandName);
service_reference_pt shell_getCommandReference(shell_pt shell, char * command);
void shell_executeCommand(shell_pt shell, char * commandLine, void (*out)(char *), void (*error)(char *));

command_service_pt shell_getCommand(shell_pt shell, char * commandName);


#endif /* SHELL_PRIVATE_H_ */
