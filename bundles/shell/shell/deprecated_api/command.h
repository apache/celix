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

#ifndef COMMAND_H_
#define COMMAND_H_

#include "celix_shell_command.h"

#define OSGI_SHELL_COMMAND_NAME             CELIX_SHELL_COMMAND_NAME
#define OSGI_SHELL_COMMAND_USAGE            CELIX_SHELL_COMMAND_USAGE
#define OSGI_SHELL_COMMAND_DESCRIPTION      CELIX_SHELL_COMMAND_DESCRIPTION

#define OSGI_SHELL_COMMAND_SERVICE_NAME     CELIX_SHELL_COMMAND_SERVICE_NAME
#define OSGI_SHELL_COMMAND_SERVICE_VERSION  CELIX_SHELL_COMMAND_SERVICE_VERSION

typedef celix_shell_command_t command_service_t; //use celix_command.h instead
typedef celix_shell_command_t* command_service_pt; //use celix_command.h instead


#endif /* COMMAND_H_ */
