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
#include "dfi_ei.h"
#include "dyn_function.h"
#include "dyn_interface.h"

extern "C" {
int __real_dynFunction_createClosure(dyn_function_type *dynFunction, void (*bind)(void *, void **, void*), void *userData, void(**fn)(void));
CELIX_EI_DEFINE(dynFunction_createClosure, int)
int __wrap_dynFunction_createClosure(dyn_function_type *dynFunction, void (*bind)(void *, void **, void*), void *userData, void(**fn)(void)) {
    CELIX_EI_IMPL(dynFunction_createClosure);
    return __real_dynFunction_createClosure(dynFunction, bind, userData, fn);
}

int __real_jsonRpc_prepareInvokeRequest(dyn_function_type *func, const char *id, void *args[], char **out);
CELIX_EI_DEFINE(jsonRpc_prepareInvokeRequest, int)
int __wrap_jsonRpc_prepareInvokeRequest(dyn_function_type *func, const char *id, void *args[], char **out) {
    CELIX_EI_IMPL(jsonRpc_prepareInvokeRequest);
    return __real_jsonRpc_prepareInvokeRequest(func, id, args, out);
}

int __real_jsonRpc_call(const dyn_interface_type* intf, void* service, const char* request, char** out);
CELIX_EI_DEFINE(jsonRpc_call, int)
int __wrap_jsonRpc_call(const dyn_interface_type* intf, void* service, const char* request, char** out) {
    CELIX_EI_IMPL(jsonRpc_call);
    return __real_jsonRpc_call(intf, service, request, out);
}

}