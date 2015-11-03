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
 * command.h
 *
 *  \date       Aug 13, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef COMMAND_H_
#define COMMAND_H_

#include "celix_errno.h"
#include <stdio.h>

#define OSGI_SHELL_COMMAND_NAME "command.name"
#define OSGI_SHELL_COMMAND_USAGE "command.usage"
#define OSGI_SHELL_COMMAND_DESCRIPTION "command.description"

static const char * const OSGI_SHELL_COMMAND_SERVICE_NAME = "commandService";

typedef struct commandService * command_service_pt;

/**
 * The command service can be used to register additional shell commands.
 * The service should be register with the following properties:
 *  - command.name: mandatory, name of the command e.g. 'lb'
 *  - command.usage: optional, string describing how tu use the commmand e.g. 'lb [-l | -s | -u]'
 *  - command.descrription: optional, string describing the command e.g. 'list bundles.'
 */
struct commandService {
	void *handle;
	celix_status_t (*executeCommand)(void *handle, char * commandLine, FILE *outStream, FILE *errorStream);
};




#endif /* COMMAND_H_ */
