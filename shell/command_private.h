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
 * command_private.h
 *
 *  Created on: Aug 13, 2010
 *      Author: alexanderb
 */

#ifndef COMMAND_PRIVATE_H_
#define COMMAND_PRIVATE_H_

#include "command.h"
#include "headers.h"

struct command {
	char * name;
	char * usage;
	char * shortDescription;

	BUNDLE_CONTEXT bundleContext;

	void (*executeCommand)(COMMAND command, char * commandLine, void (*out)(char *), void (*error)(char *));
};


#endif /* COMMAND_PRIVATE_H_ */
