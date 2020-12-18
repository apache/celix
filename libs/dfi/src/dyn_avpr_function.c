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

#include "dyn_function.h"
#include "dyn_function_common.h"

#include "jansson.h"

#include <stdlib.h>
#include <string.h>

#include "dyn_common.h"
#include "dyn_type.h"

DFI_SETUP_LOG(dynAvprFunction)

#define FQN_SIZE 256
#define ARG_SIZE 32

// Section: static function declarations
dyn_function_type * dynAvprFunction_parseFromJson(json_t * const root, const char * fqn);
static json_t const * const dynAvprFunction_findFunc(const char * fqn, json_t * const messagesObject, const char * parentNamespace);
static bool dynAvprFunction_parseFunc(dyn_function_type * func, json_t const * const jsonFuncObject, json_t * const root, const char * fqn, const char * parentNamespace);
inline static bool dynAvprFunction_parseArgument(dyn_function_type * func, size_t index, json_t const * entry, json_t * const root, const char * namespace, char * argBuffer, char * typeBuffer);
inline static bool dynAvprFunction_parseReturn(dyn_function_type * func, size_t index, json_t const * const response_type, json_t * const root, const char * namespace, char * typeBuffer);
inline static bool dynAvprFunction_createHandle(dyn_function_type * func);
inline static dyn_function_argument_type * dynAvprFunction_prepareArgumentEntry(const char * name);
inline static dyn_type * dynAvprFunction_createVoidType();
inline static dyn_type * dynAvprFunction_createNativeType();
inline static bool dynAvprFunction_initCif(dyn_function_type * func, size_t nofArguments);

// Section: extern function definitions
ffi_type * dynType_ffiType(dyn_type *type);
dyn_type * dynAvprType_createNestedForFunction(dyn_type * store_type, const char* fqn);
dyn_type * dynAvprType_parseFromTypedJson(json_t * const root, json_t const * const type_entry, const char * namespace);
void dynAvprType_constructFqn(char *destination, size_t size, const char *possibleFqn, const char *ns);

// Section: function definitions
dyn_function_type * dynFunction_parseAvpr(FILE * avprStream, const char * fqn) {
    json_error_t error;
    json_t *root = json_loadf(avprStream, JSON_REJECT_DUPLICATES, &error);
    if (!root) {
        LOG_ERROR("function_parseAvpr: Error decoding json: line %d: %s", error.line, error.text);
        return NULL;
    }

    dyn_function_type * func = dynAvprFunction_parseFromJson(root, fqn);
    json_decref(root);
    return func;
}

dyn_function_type * dynFunction_parseAvprWithStr(const char * avpr, const char * fqn) {
    FILE *avprStream = fmemopen((char*)avpr, strlen(avpr), "r");

    if (!avprStream) {
        LOG_ERROR("function_parseAvprWithStr: Error creating mem stream for descriptor string. %s", strerror(errno)); 
        return NULL;
    }

    dyn_function_type *returnValue = dynFunction_parseAvpr(avprStream, fqn);
    fclose(avprStream);
    return returnValue;
}

dyn_function_type * dynAvprFunction_parseFromJson(json_t * const root, const char * fqn) {
    bool valid = true;
    dyn_function_type *func = calloc(1, sizeof(*func));

    if (!json_is_object(root)) {
        LOG_ERROR("ParseFunction: Error decoding json, root should be an object");
        valid = false;
    }

    // Get base namespace
    const char *parentNamespace = json_string_value(json_object_get(root, "namespace"));
    if (valid && !parentNamespace) {
        LOG_ERROR("ParseFunction: No namespace found in root, or it is null!");
        valid = false;
    }

    // Get messages array
    json_t * const messagesObject = json_object_get(root, "messages");
    if (valid && !json_is_object(messagesObject)) {
        LOG_ERROR("ParseFunction: No messages, or it is not an object");
        valid = false;
    }

    if (valid && !dynAvprFunction_parseFunc(func, dynAvprFunction_findFunc(fqn, messagesObject, parentNamespace), root, fqn, parentNamespace)) {
        LOG_ERROR("parseAvpr: Destroying incorrect result");
        dynFunction_destroy(func);
        func = NULL;
    }
    else {
        LOG_DEBUG("ParseFunction: Succesfully parsed \"%s\"", fqn)
    }

    return func;
}

static json_t const * const dynAvprFunction_findFunc(const char * fqn, json_t * const messagesObject, const char * parentNamespace) {
    char nameBuffer[FQN_SIZE];
    const char * func_name;
    json_t const * func_entry;
    json_t const * local_ns;
    json_object_foreach(messagesObject, func_name, func_entry) {
        local_ns = json_object_get(func_entry, "namespace");
        dynAvprType_constructFqn(nameBuffer, FQN_SIZE, func_name, json_is_string(local_ns) ? json_string_value(local_ns) : parentNamespace);

        LOG_DEBUG("FindFunc: Comparing: %s and %s", fqn, nameBuffer);
        if (strcmp(nameBuffer, fqn) == 0) {
            LOG_DEBUG("FindFunc: Found function definition for %s", fqn);
            return func_entry;
        }
    }
    LOG_INFO("FindFunc: Did not find function definition for %s", fqn);
    return NULL;
}

static bool dynAvprFunction_parseFunc(dyn_function_type * func, json_t const * const jsonFuncObject, json_t * const root, const char * fqn, const char * parentNamespace) {
    if (!jsonFuncObject) {
        LOG_WARNING("ParseFunc: Received NULL, function not found, nothing to parse");
        return false;
    }

    func->name = strdup(fqn);
    if (!func->name) {
        LOG_ERROR("ParseFunc: Error allocating memory for function name %s", fqn);
        return false;
    }

    json_t const * const argument_list = json_object_get(jsonFuncObject, "request");
    if (!json_is_array(argument_list)) {
        LOG_ERROR("ParseFunc: Request is not an array");
        free(func->name);
        return false;
    }

    json_t const * const json_function_namespace = json_object_get(jsonFuncObject, "namespace");
    const char* function_namespace = json_is_string(json_function_namespace) ? json_string_value(json_function_namespace) : parentNamespace;

    // Parse requests argument array
    TAILQ_INIT(&func->arguments);
    if (!dynAvprFunction_createHandle(func)) {
        LOG_ERROR("ParseFunc: Could not creat handle for %s", fqn);
        free(func->name);
        return false;
    }
    char argBuffer[ARG_SIZE]; 
    char typeBuffer[FQN_SIZE];
    size_t index;
    json_t const * arg_entry;
    json_array_foreach(argument_list, index, arg_entry) {
        if (!dynAvprFunction_parseArgument(func, index+1, arg_entry, root, function_namespace, argBuffer, typeBuffer)) { /* Offset index to account for the handle */
            LOG_ERROR("ParseFunc: Could not parse argument %d for %s", index, fqn);
            return false;
        }
        LOG_DEBUG("Added argument %d", index+1);
    }
    index++;

    json_t const * const response_type = json_object_get(jsonFuncObject, "response");
    if (!dynAvprFunction_parseReturn(func, index, response_type, root, function_namespace, typeBuffer)) {
        LOG_ERROR("ParseFunc: Could not parse return type for %s", fqn);
        free(func->name);
        return false;
    }
    index+=1;

    func->funcReturn = dynAvprFunction_createNativeType();
    if (!func->funcReturn) {
        LOG_ERROR("ParseFunc: Could not create error indicator for %s", fqn);
        return false;
    }

    if (!dynAvprFunction_initCif(func, index)) {
        LOG_ERROR("ParseFunc: Could not prepare cif information");
        return false;
    }

    // Everything successfull
    return true;
}

inline static bool dynAvprFunction_parseArgument(dyn_function_type * func, size_t index, json_t const * entry, json_t * const root, const char * namespace, char * argBuffer, char * typeBuffer) {
    const char * entry_name = json_string_value(json_object_get(entry, "name"));
    if (!entry_name)  {
        LOG_INFO("ParseArgument: Could not find argument name for %d, using default", index);
        snprintf(argBuffer, ARG_SIZE, "arg%04lu", (long unsigned int) index);
        entry_name = argBuffer;
    }

    dyn_type * entry_dyn_type = dynAvprType_parseFromTypedJson(root, json_object_get(entry, "type"), namespace);
    if (!entry_dyn_type) {
        LOG_ERROR("ParseArgument: Could not parse type for argument");
        return false;
    }

    if (json_is_true(json_object_get(entry, "ptr"))) {
        entry_dyn_type = dynAvprType_createNestedForFunction(entry_dyn_type, entry_name);
        if (!entry_dyn_type) {
            LOG_ERROR("ParseArgument: Could not create pointer argument for %s", entry_name);
            dynType_destroy(entry_dyn_type);
            return false;
        }
    }

    dyn_function_argument_type *arg = dynAvprFunction_prepareArgumentEntry(entry_name);
    if (!arg) {
        dynType_destroy(entry_dyn_type);
        return false;
    }

    arg->index = (int) index;
    arg->type = entry_dyn_type;
    arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__STD;

    TAILQ_INSERT_TAIL(&func->arguments, arg, entries);
    return true;
}

inline static bool dynAvprFunction_parseReturn(dyn_function_type * func, size_t index, json_t const * const response_type, json_t * const root, const char * namespace, char * typeBuffer) {
    dyn_type * return_dyn_type = dynAvprType_parseFromTypedJson(root, response_type, namespace);
    if (!return_dyn_type) {
        LOG_ERROR("ParseReturn: Could not parse return argument");
        return false;
    }

    dyn_type * out_dyn_type = dynAvprType_createNestedForFunction(return_dyn_type, "out");
    if (!out_dyn_type) {
        LOG_ERROR("ParseReturn: Could not create pointer for return type");
        dynType_destroy(return_dyn_type);
        return false;
    }

    dyn_function_argument_type *arg = dynAvprFunction_prepareArgumentEntry("out");
    if (!arg) {
        LOG_ERROR("ParseReturn: Could not create nested type for return type");
        dynType_destroy(out_dyn_type);
        return false;
    }

    arg->index = (int) index;

    // Check descriptor type, if it is a struct ({), sequence ([) or a string (t) an extra level of indirection is needed
    int descriptor = dynType_descriptorType(return_dyn_type);
    if (descriptor == '{' || descriptor == '[') {
        out_dyn_type = dynAvprType_createNestedForFunction(out_dyn_type, "out");
        if (!out_dyn_type) {
            LOG_ERROR("ParseReturn: Could not create nested type for return complex type");
            dynType_destroy(out_dyn_type);
            return false;
        }
        arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__OUTPUT;
    }
    else {
        arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT;
    }

    arg->type = out_dyn_type;
    TAILQ_INSERT_TAIL(&func->arguments, arg, entries);
    return true;
}

inline static bool dynAvprFunction_createHandle(dyn_function_type * func) {
    dyn_function_argument_type * arg = dynAvprFunction_prepareArgumentEntry("handle");
    if (!arg) {
        return false;
    }

    arg->index = 0;
    arg->type = dynAvprFunction_createVoidType();
    if (!arg->type) {
        free(arg);
        return false;
    }

    arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__HANDLE;
    TAILQ_INSERT_TAIL(&func->arguments, arg, entries);
    return true;
}

inline static dyn_function_argument_type * dynAvprFunction_prepareArgumentEntry(const char * name) {
    if (!name) {
        LOG_ERROR("PrepareArgumentEntry: Need name to create argument entry");
        return NULL;
    }

    dyn_function_argument_type *arg = calloc(1, sizeof(*arg));
    arg->name = strndup(name, ARG_SIZE);
    return arg;
}

inline static dyn_type * dynAvprFunction_createVoidType() {
    const char * entry = "{ \
                \"protocol\" : \"types\", \
                \"namespace\" : \"dummy\", \
                \"types\" : [ { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"VoidPtr\", \
                    \"alias\" : \"void_ptr\", \
                    \"size\" : 4 \
                  } ] ,\
                \"messages\" : { }\
                }";
    return dynType_parseAvprWithStr(entry, "dummy.VoidPtr");
}

inline static dyn_type * dynAvprFunction_createNativeType() {
    const char * entry = "{ \
                \"protocol\" : \"types\", \
                \"namespace\" : \"dummy\", \
                \"types\" : [ { \
                    \"type\" : \"fixed\", \
                    \"name\" : \"NativeInt\", \
                    \"alias\" : \"native_int\", \
                    \"size\" : 4 \
                  } ] ,\
                \"messages\" : { }\
                }";
    return dynType_parseAvprWithStr(entry, "dummy.NativeInt");
}

inline static bool dynAvprFunction_initCif(dyn_function_type * func, size_t nofArguments) {
    func->ffiArguments = calloc(nofArguments, sizeof(ffi_type*));
    dyn_function_argument_type *entry = NULL;
    TAILQ_FOREACH(entry, &func->arguments, entries) {
        func->ffiArguments[entry->index] = dynType_ffiType(entry->type);
    }

    ffi_type *returnType = dynType_ffiType(func->funcReturn);
    int ffi_result = ffi_prep_cif(&func->cif, FFI_DEFAULT_ABI, (int) nofArguments, returnType, func->ffiArguments);

    return ffi_result == FFI_OK;
}

