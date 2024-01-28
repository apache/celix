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

#include "json_rpc.h"
#include "json_serializer.h"
#include "dyn_type.h"
#include "dyn_interface.h"
#include "dyn_type_common.h"
#include "celix_cleanup.h"
#include "celix_err.h"

#include <jansson.h>
#include <stdint.h>
#include <string.h>
#include <ffi.h>

#define CELIX_JSON_RPC_MAX_ARGS     16

static int OK = 0;
static int ERROR = 1;

typedef void (*gen_func_type)(void);

struct generic_service_layout {
    void* handle;
    gen_func_type methods[];
};

typedef struct celix_rpc_args {
    const struct dyn_function_arguments_head* dynArgs;
    void* args[CELIX_JSON_RPC_MAX_ARGS];
}celix_rpc_args_t;

static void celix_rpcArgs_cleanup(celix_rpc_args_t* args) {
    const struct dyn_function_arguments_head* dynArgs = args->dynArgs;
    if (dynArgs == NULL) {
        return;
    }
    dyn_function_argument_type* entry = NULL;
    TAILQ_FOREACH(entry, dynArgs, entries) {
        const dyn_type* argType = dynType_realType(entry->type);
        enum dyn_function_argument_meta meta = entry->argumentMeta;
        if (meta == DYN_FUNCTION_ARGUMENT_META__STD) {
            if (dynType_descriptorType(argType) == 't') {
                const char* isConst = dynType_getMetaInfo(entry->type, "const");
                if (isConst != NULL && strncmp("true", isConst, 5) == 0) {
                    dynType_free(argType, args->args[entry->index]);
                } else {
                    //char* -> callee is now owner, no free for char seq needed
                    //will free the actual pointer
                    free(args->args[entry->index]);
                }
            } else {
                dynType_free(argType, args->args[entry->index]);
            }
        } else if (meta == DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT) {
            const dyn_type* subType = dynType_typedPointer_getTypedType(argType);
            dynType_free(subType, *(void**)(args->args[entry->index]));
        } else if (meta == DYN_FUNCTION_ARGUMENT_META__OUTPUT) {
            const dyn_type* typedType = dynType_typedPointer_getTypedType(argType);
            if (dynType_descriptorType(typedType) == 't') {
                free(**(void***)args->args[entry->index]);
            } else {
                const dyn_type* typedTypedType = dynType_typedPointer_getTypedType(typedType);
                dynType_free(typedTypedType, **(void***)args->args[entry->index]);
            }
        }
    }
    args->dynArgs = NULL;
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_rpc_args_t, celix_rpcArgs_cleanup)

int jsonRpc_call(const dyn_interface_type* intf, void* service, const char* request, char** out) {
    int status = OK;

    json_error_t error;
    json_auto_t* js_request = json_loads(request, 0, &error);
    if (js_request == NULL) {
        celix_err_pushf("Got json error: %s", error.text);
        return ERROR;
    }
    json_t* arguments = NULL;
    const char* sig;
    if (json_unpack(js_request, "{s:s}", "m", &sig) != 0) {
        celix_err_push("Error getting method signature");
        return ERROR;
    }
    arguments = json_object_get(js_request, "a");
    if (arguments == NULL || !json_is_array(arguments)) {
        celix_err_pushf("Error getting arguments array for %s", sig);
        return ERROR;
    }

    const struct method_entry* method = dynInterface_findMethod(intf, sig);
    if (method == NULL) {
        celix_err_pushf("Cannot find method with sig '%s'", sig);
        return ERROR;
    }

    struct generic_service_layout* serv = service;
    const struct dyn_function_arguments_head* dynArgs = dynFunction_arguments(method->dynFunc);
    const dyn_function_argument_type* last = TAILQ_LAST(dynArgs, dyn_function_arguments_head);
    int nrOfArgs = dynFunction_nrOfArguments(method->dynFunc);
    if (nrOfArgs > CELIX_JSON_RPC_MAX_ARGS) {
        celix_err_pushf("Too many arguments for %s: %d > %d", sig, nrOfArgs, CELIX_JSON_RPC_MAX_ARGS);
        return ERROR;
    }
    void* ptr = NULL;
    void* ptrToPtr = &ptr;
    celix_auto(celix_rpc_args_t) rpcArgs = { dynArgs, {0} };

    rpcArgs.args[0] = &serv->handle;
    --nrOfArgs;
    if (last->argumentMeta == DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT) {
        const dyn_type *subType = dynType_typedPointer_getTypedType(dynType_realType(last->type));
        rpcArgs.args[last->index] = &ptr;
        if ((status = dynType_alloc(subType, &ptr)) != OK) {
            celix_err_pushf("Error allocating memory for pre-allocated output argument of %s", sig);
            return ERROR;
        }
        --nrOfArgs;
    } else if (last->argumentMeta == DYN_FUNCTION_ARGUMENT_META__OUTPUT) {
        rpcArgs.args[last->index] = &ptrToPtr;
        --nrOfArgs;
    }
    if ((size_t)nrOfArgs != json_array_size(arguments)) {
        celix_err_pushf("Wrong number of standard arguments for %s. Expected %d, got %zu",
                        sig, nrOfArgs, json_array_size(arguments));
        return ERROR;
    }
    //setup and deserialize input
    dyn_function_argument_type* entry = NULL;
    TAILQ_FOREACH(entry, dynArgs, entries) {
        if (entry->argumentMeta != DYN_FUNCTION_ARGUMENT_META__STD) {
            continue;
        }
        status = jsonSerializer_deserializeJson(entry->type, json_array_get(arguments, entry->index-1), &(rpcArgs.args[entry->index]));
        if (status != OK) {
            celix_err_pushf("Error deserializing argument %d for %s", entry->index, sig);
            return status;
        }
    }
    ffi_sarg returnVal = 1;
    (void)dynFunction_call(method->dynFunc, serv->methods[method->index], (void *) &returnVal, rpcArgs.args);

    int funcCallStatus = (int)returnVal;
    //serialize output
    json_auto_t* jsonResult = NULL;
    if (funcCallStatus == 0) {
        const dyn_type* argType = dynType_realType(last->type);
        if (last->argumentMeta == DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT) {
            status = jsonSerializer_serializeJson(argType, rpcArgs.args[last->index], &jsonResult);
        } else if (last->argumentMeta == DYN_FUNCTION_ARGUMENT_META__OUTPUT) {
            status = jsonSerializer_serializeJson(dynType_typedPointer_getTypedType(argType), (void*) &ptr, &jsonResult);
        }
        if (status != OK) {
            celix_err_pushf("Error serializing result for %s", sig);
            return status;
        }
    }
    celix_rpcArgs_cleanup(&rpcArgs);

    json_auto_t* payload = json_object();
    if (funcCallStatus == 0) {
        if (jsonResult != NULL) {
            status = json_object_set_new_nocheck(payload, "r", celix_steal_ptr(jsonResult));
        }
    } else {
        status = json_object_set_new_nocheck(payload, "e", json_integer(funcCallStatus));
    }
    if (status != 0)  {
        celix_err_pushf("Error generating response payload for %s", sig);
        return ERROR;
    }
    //use JSON_COMPACT to reduce the size of the JSON string.
    *out = json_dumps(payload, JSON_COMPACT | JSON_ENCODE_ANY);
    return (*out != NULL) ? OK : ERROR;
}

int jsonRpc_prepareInvokeRequest(const dyn_function_type* func, const char* id, void* args[], char** out) {
    json_auto_t* invoke = json_object();
    // each method must have a non-null id
    if (json_object_set_new_nocheck(invoke, "m", json_string(id)) != 0) {
        celix_err_pushf("Error setting method name '%s'", id);
        return ERROR;
    }

    json_auto_t* arguments = json_array();
    if (json_object_set_nocheck(invoke, "a", arguments) != 0) {
        celix_err_pushf("Error adding arguments array for '%s'", id);
        return ERROR;
    }

    const struct dyn_function_arguments_head* dynArgs = dynFunction_arguments(func);
    dyn_function_argument_type* entry = NULL;
    TAILQ_FOREACH(entry, dynArgs, entries) {
        const dyn_type* type = dynType_realType(entry->type);
        enum dyn_function_argument_meta meta = entry->argumentMeta;
        if (meta == DYN_FUNCTION_ARGUMENT_META__STD) {
            json_t* val = NULL;

            int rc = jsonSerializer_serializeJson(type, args[entry->index], &val);
            if (rc != 0) {
                celix_err_pushf("Failed to serialize args for function '%s'\n", id);
                return ERROR;
            }

            if (dynType_descriptorType(type) == 't') {
                // we need to get meta info from the original type, which could be a reference, rather than the real type
                const char* metaArgument = dynType_getMetaInfo(entry->type, "const");
                if (metaArgument != NULL && strcmp("true", metaArgument) == 0) {
                    //const char * as input -> nop
                } else {
                    char** str = args[entry->index];
                    free(*str); //char * as input -> got ownership -> free it.
                }
            }
            if (json_array_append_new(arguments, val) != 0) {
                celix_err_pushf("Error adding argument (%d) for '%s'", entry->index, id);
                return ERROR;
            }
        }
    }

    //use JSON_COMPACT to reduce the size of the JSON string.
    char* invokeStr = json_dumps(invoke, JSON_COMPACT | JSON_ENCODE_ANY);
    *out = invokeStr;
    return *out != NULL ? OK : ERROR;
}

int jsonRpc_handleReply(const dyn_function_type* func, const char* reply, void* args[], int* rsErrno) {
    int status = OK;

    json_error_t error;
    json_auto_t* replyJson = json_loads(reply, JSON_DECODE_ANY, &error);
    if (replyJson == NULL) {
        celix_err_pushf("Error parsing json '%s', got error '%s'", reply, error.text);
        return ERROR;
    }

    json_t* result = NULL;
    json_t* rsError = NULL;
    *rsErrno = 0;

    const struct dyn_function_arguments_head* arguments = dynFunction_arguments(func);
    dyn_function_argument_type* last = TAILQ_LAST(arguments, dyn_function_arguments_head);
    const dyn_type* argType = dynType_realType(last->type);
    enum dyn_function_argument_meta meta = last->argumentMeta;
    rsError = json_object_get(replyJson, "e");
    if (rsError != NULL) {
        //get the invocation error of remote service function
        *rsErrno = (int) json_integer_value(rsError);
        return OK;
    }
    if (meta != DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT && meta != DYN_FUNCTION_ARGUMENT_META__OUTPUT) {
        return OK;
    }
    result = json_object_get(replyJson, "r");
    if (result == NULL) {
        celix_err_pushf("Expected result in reply. got '%s'", reply);
        return ERROR;
    }
    void** lastArg = (void **) args[last->index];
    if (*lastArg == NULL) {
        // caller provides nullptr, no need to deserialize
        return OK;
    }
    if (meta == DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT) {
        void* tmp = NULL;
        void** out = lastArg;
        size_t size = 0;

        const dyn_type* subType = dynType_typedPointer_getTypedType(argType);
        status = jsonSerializer_deserializeJson(subType, result, &tmp);
        if (tmp != NULL) {
            size = dynType_size(subType);
            memcpy(*out, tmp, size);
            dynType_free(subType, tmp);
        }
    } else {
        const dyn_type* subType = dynType_typedPointer_getTypedType(argType);

        if (dynType_descriptorType(subType) == 't') {
            char*** out = (char ***) lastArg;
            char** ptrToString = NULL;
            status = jsonSerializer_deserializeJson(subType, result, (void**)&ptrToString);
            if (ptrToString != NULL) {
                **out = (void*)*ptrToString;
                free(ptrToString);
            }
        } else {
            const dyn_type* subSubType = dynType_typedPointer_getTypedType(subType);
            void*** out = (void ***) lastArg;
            if (json_is_null(result)) {
                **out = NULL;
            } else {
                status = jsonSerializer_deserializeJson(subSubType, result, *out);
            }
        }
    }

    return status;
}
