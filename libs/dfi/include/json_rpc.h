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
#include "dyn_type.h"
#include "dyn_function.h"
#include "dyn_interface.h"
#include "celix_dfi_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Call a remote service using JSON-RPC.
 *
 * Caller is the owner of the out parameter and should release it using free.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] intf The interface type of the service to call.
 * @param[in] service The service to call.
 * @param[in] request The JSON-RPC request to send.
 * @param[out] out The JSON-RPC reply.
 * @return 0 if successful, otherwise 1.
 *
 */
CELIX_DFI_EXPORT int jsonRpc_call(const dyn_interface_type* intf, void* service, const char* request, char** out);

/**
 * @brief Prepare a JSON-RPC request for a given function.
 *
 * Caller is the owner of the out parameter and should release it using free.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] func The function type to prepare the request for.
 * @param[in] id The function ID.
 * @param[in] args The arguments to use for the function.
 * @param[out] out The JSON-RPC request.
 * @return 0 if successful, otherwise 1.
 *
 */
CELIX_DFI_EXPORT int jsonRpc_prepareInvokeRequest(const dyn_function_type* func, const char* id, void* args[], char** out);

/**
 * @brief Handle a JSON-RPC reply for a given function.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] func The function type to handle the reply for.
 * @param[in] reply The JSON-RPC reply.
 * @param[out] args The arguments to use for the function.
 * @param[out] rsErrno The return status of the function.
 * @return 0 if successful, otherwise 1.
 *
 */
CELIX_DFI_EXPORT int jsonRpc_handleReply(const dyn_function_type* func, const char* reply, void* args[], int* rsErrno);

#ifdef __cplusplus
}
#endif

#endif
