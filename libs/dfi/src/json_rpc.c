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
#include "celix_compiler.h"
#include "celix_err.h"

#include <jansson.h>
#include <stdint.h>
#include <string.h>
#include <ffi.h>

static int OK = 0;
static int ERROR = 1;

typedef void (*gen_func_type)(void);

struct generic_service_layout {
    void* handle;
    gen_func_type methods[];
};

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
        celix_err_push("Error getting arguments array");
        return ERROR;
    }

    const struct methods_head* methods = dynInterface_methods(intf);
    struct method_entry* entry = NULL;
    struct method_entry* method = NULL;
    TAILQ_FOREACH(entry, methods, entries) {
        if (strcmp(sig, entry->id) == 0) {
            method = entry;
            break;
        }
    }

    if (method == NULL) {
        celix_err_pushf("Cannot find method with sig '%s'", sig);
        return ERROR;
    }

    struct generic_service_layout* serv = service;
    void* handle = serv->handle;
    void (*fp)(void) = serv->methods[method->index];

    dyn_function_type* func = method->dynFunc;
    int nrOfArgs = dynFunction_nrOfArguments(method->dynFunc);
    void* args[nrOfArgs];

    json_t* value = NULL;

    int i;
    int index = 0;

    void* ptr = NULL;
    void* ptrToPtr = &ptr;
    void* instPtr = NULL;

    //setup and deserialize input
    for (i = 0; i < nrOfArgs; ++i) {
        const dyn_type* argType = dynType_realType(dynFunction_argumentTypeForIndex(func, i));
        enum dyn_function_argument_meta  meta = dynFunction_argumentMetaForIndex(func, i);
        if (meta == DYN_FUNCTION_ARGUMENT_META__STD) {
            value = json_array_get(arguments, index++);
            void* outPtr = NULL;
            status = jsonSerializer_deserializeJson(argType, value, &outPtr);
            args[i] = outPtr;
        } else if (meta == DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT) {
            void* inst = NULL;
            const dyn_type *subType = dynType_typedPointer_getTypedType(argType);
            dynType_alloc(subType, &inst);
            instPtr = inst;
            args[i] = &instPtr;
        } else if (meta == DYN_FUNCTION_ARGUMENT_META__OUTPUT) {
            args[i] = &ptrToPtr;
        } else if (meta == DYN_FUNCTION_ARGUMENT_META__HANDLE) {
            args[i] = &handle;
        }

        if (status != OK) {
            break;
        }
    }
    json_decref(celix_steal_ptr(js_request));

    ffi_sarg returnVal = 1;

    if (status == OK) {
        status = dynFunction_call(func, fp, (void *) &returnVal, args);
    }

    int funcCallStatus = (int)returnVal;

    //free input args
    json_t* jsonResult = NULL;
    for(i = 0; i < nrOfArgs; ++i) {
        const dyn_type* argType = dynFunction_argumentTypeForIndex(func, i);
        enum dyn_function_argument_meta meta = dynFunction_argumentMetaForIndex(func, i);
        if (meta == DYN_FUNCTION_ARGUMENT_META__STD) {
            if (dynType_descriptorType(argType) == 't') {
                const char* isConst = dynType_getMetaInfo(argType, "const");
                if (isConst != NULL && strncmp("true", isConst, 5) == 0) {
                    dynType_free(argType, args[i]);
                } else {
                    //char* -> callee is now owner, no free for char seq needed
                    //will free the actual pointer
                    free(args[i]);
                }
            } else {
                dynType_free(argType, args[i]);
            }
        }
    }

    //serialize and free output
    for (i = 0; i < nrOfArgs; i += 1) {
        const dyn_type* argType = dynType_realType(dynFunction_argumentTypeForIndex(func, i));
        enum dyn_function_argument_meta  meta = dynFunction_argumentMetaForIndex(func, i);
        if (meta == DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT) {
            if (funcCallStatus == 0 && status == OK) {
                status = jsonSerializer_serializeJson(argType, args[i], &jsonResult);
            }
            const dyn_type* subType = dynType_typedPointer_getTypedType(argType);
            void** ptrToInst = (void**)args[i];
            dynType_free(subType, *ptrToInst);
        } else if (meta == DYN_FUNCTION_ARGUMENT_META__OUTPUT) {
            if (funcCallStatus == 0 && ptr != NULL) {
                const dyn_type* typedType = NULL;
                if (status == OK) {
                    typedType = dynType_typedPointer_getTypedType(argType);
                }
                if (status == OK && dynType_descriptorType(typedType) == 't') {
                    status = jsonSerializer_serializeJson(typedType, (void*) &ptr, &jsonResult);
                    free(ptr);
                } else {
                    const dyn_type* typedTypedType = NULL;
                    if (status == OK) {
                        typedTypedType = dynType_typedPointer_getTypedType(typedType);
                    }

                    if(status == OK){
                        status = jsonSerializer_serializeJson(typedTypedType, ptr, &jsonResult);
                    }

                    if (status == OK) {
                        dynType_free(typedTypedType, ptr);
                    }
                }

            }
        }

        if (status != OK) {
            break;
        }
    }

    char* response = NULL;
    if (status == OK) {
        json_t* payload = json_object();
        if (funcCallStatus == 0) {
            if (jsonResult == NULL) {
                //ignore -> no result
            } else {
                json_object_set_new_nocheck(payload, "r", jsonResult);
            }
        } else {
            json_object_set_new_nocheck(payload, "e", json_integer(funcCallStatus));
        }
        //use JSON_COMPACT to reduce the size of the JSON string.
        response = json_dumps(payload, JSON_COMPACT | JSON_ENCODE_ANY);
        json_decref(payload);
    }

    if (status == OK) {
        *out = response;
    } else {
        free(response);
    }

    return status;
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
                const char* metaArgument = dynType_getMetaInfo(type, "const");
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
            status = jsonSerializer_deserializeJson(subSubType, result, *out);
        }
    }

    return status;
}
