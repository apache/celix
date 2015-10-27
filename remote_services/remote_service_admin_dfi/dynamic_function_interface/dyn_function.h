/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#ifndef __DYN_FUNCTION_H_
#define __DYN_FUNCTION_H_

#include <ffi.h>
#include "dyn_type.h"
#include "dfi_log_util.h"

/**
 * Uses the following schema
 * (Name)([Type]*)Type
 *
 * Dyn fynction argument meta (am) as meta info, with the following possible values
 * am=handle #void pointer for the handle
 * am=pre #output pointer with memory preallocated
 * am=out #output pointer
 */

typedef struct _dyn_function_type dyn_function_type;

DFI_SETUP_LOG_HEADER(dynFunction);

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

#endif
