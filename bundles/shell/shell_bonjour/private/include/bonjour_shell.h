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
 * bonjour_shell.h
 *
 *  \date       Oct 20, 2014
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BONJOUR_SHELL_H_
#define BONJOUR_SHELL_H_

#include <service_reference.h>

#include "celix_errno.h"

typedef struct bonjour_shell *bonjour_shell_pt;

celix_status_t bonjourShell_create(char *id, bonjour_shell_pt *shell);
celix_status_t bonjourShell_destroy(bonjour_shell_pt shell);

celix_status_t bonjourShell_addShellService(void * handle, service_reference_pt reference, void * service);
celix_status_t bonjourShell_removeShellService(void * handle, service_reference_pt reference, void * service);


#endif /* BONJOUR_SHELL_H_ */
