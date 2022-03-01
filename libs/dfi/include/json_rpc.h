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

#ifndef __JSON_RPC_H_
#define __JSON_RPC_H_

#include <jansson.h>
#include "dfi_log_util.h"
#include "dyn_type.h"
#include "dyn_function.h"
#include "dyn_interface.h"

#ifdef __cplusplus
extern "C" {
#endif

//logging
DFI_SETUP_LOG_HEADER(jsonRpc);

int jsonRpc_call(dyn_interface_type *intf, void *service, const char *request, char **out);


int jsonRpc_prepareInvokeRequest(dyn_function_type *func, const char *id, void *args[], char **out);
int jsonRpc_handleReply(dyn_function_type *func, const char *reply, void *args[], int *rsErrno);

#ifdef __cplusplus
}
#endif

#endif
