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
 * command_impl.h
 *
 *  \date       Aug 13, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef COMMAND_PRIVATE_H_
#define COMMAND_PRIVATE_H_

#include "command.h"
#include "bundle_context.h"

struct command {
	char * name;
	char * usage;
	char * shortDescription;

	bundle_context_pt bundleContext;
	void *handle;

	void (*executeCommand)(command_pt command, char * commandLine, void (*out)(char *), void (*error)(char *));
};

char *command_getName(command_pt command);
char *command_getUsage(command_pt command);
char *command_getShortDescription(command_pt command);

#endif /* COMMAND_PRIVATE_H_ */
