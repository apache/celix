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

static const char * const OSGI_SHELL_COMMAND_SERVICE_NAME = "commandService";

typedef struct command * command_pt;

typedef struct commandService * command_service_pt;

struct commandService {
	command_pt command;
	char * (*getName)(command_pt command);
	char * (*getUsage)(command_pt command);
	char * (*getShortDescription)(command_pt command);
	void (*executeCommand)(command_pt command, char * commandLine, void (*out)(char *), void (*error)(char *));
};




#endif /* COMMAND_H_ */
