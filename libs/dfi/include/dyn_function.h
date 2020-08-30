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

#ifndef __DYN_FUNCTION_H_
#define __DYN_FUNCTION_H_

#include "dyn_type.h"
#include "dfi_log_util.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * Uses the following schema
 * (Name)([Type]*)Type
 *
 * Dyn function argument meta (am) as meta info, with the following possible values
 * am=handle #void pointer for the handle
 * am=pre #output pointer with memory pre-allocated
 * am=out #output pointer
 *
 * text argument (t) can also be annotated to be considered const string.
 * Normally a text argument will be handled as char*, meaning that the callee is expected to take of ownership.
 * If a const=true annotation is used the text argument will be handled as a const char*, meaning that the caller
 * keeps ownership of the string.
 */

typedef struct _dyn_function_type dyn_function_type;

DFI_SETUP_LOG_HEADER(dynFunction);
DFI_SETUP_LOG_HEADER(dynAvprFunction);

enum dyn_function_argument_meta {
    DYN_FUNCTION_ARGUMENT_META__STD = 0,
    DYN_FUNCTION_ARGUMENT_META__HANDLE = 1,
    DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT = 2,
    DYN_FUNCTION_ARGUMENT_META__OUTPUT = 3
};

int dynFunction_parse(FILE *descriptorStream, struct types_head *refTypes, dyn_function_type **dynFunc);
int dynFunction_parseWithStr(const char *descriptor, struct types_head *refTypes, dyn_function_type **dynFunc);

int dynFunction_nrOfArguments(dyn_function_type *dynFunc);
dyn_type *dynFunction_argumentTypeForIndex(dyn_function_type *dynFunc, int argumentNr);
enum dyn_function_argument_meta dynFunction_argumentMetaForIndex(dyn_function_type *dynFunc, int argumentNr);
dyn_type * dynFunction_returnType(dyn_function_type *dynFunction);

void dynFunction_destroy(dyn_function_type *dynFunc);
int dynFunction_call(dyn_function_type *dynFunc, void(*fn)(void), void *returnValue, void **argValues);

int dynFunction_createClosure(dyn_function_type *func, void (*bind)(void *, void **, void*), void *userData, void(**fn)(void));
int dynFunction_getFnPointer(dyn_function_type *func, void (**fn)(void));

/**
 * Returns whether the function has a return type.
 * Will return false if return is void.
 */
bool dynFunction_hasReturn(dyn_function_type *dynFunction);

// Avpr parsing
dyn_function_type * dynFunction_parseAvprWithStr(const char * avpr, const char * fqn);
dyn_function_type * dynFunction_parseAvpr(FILE * avprStream, const char * fqn);

#ifdef __cplusplus
}
#endif

#endif
