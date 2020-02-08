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
#ifndef SHELL_H_
#define SHELL_H_

#include "celix_shell.h"

#define OSGI_SHELL_SERVICE_NAME     CELIX_SHELL_SERVICE_NAME
#define OSGI_SHELL_SERVICE_VERSION  CELIX_SHELL_SERVICE_VERSION

//NOTE celix_shell_t is a backwards compatible service for - the deprecated - shell_service_t.
typedef celix_shell_t shell_service_t; //use celix_shell.h instead
typedef celix_shell_t* shell_service_pt; //use celix_shell.h instead

#endif /* SHELL_H_ */
