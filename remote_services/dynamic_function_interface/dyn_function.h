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
 * e.g add(DD)D or sum({[D[D setA setB})D
 */

typedef struct _dyn_function_type dyn_function_type;
typedef struct _dyn_closure_type dyn_closure_type;

DFI_SETUP_LOG_HEADER(dynFunction);

int dynFunction_parse(FILE *descriptorStream, dyn_type_list_type *typeReferences, dyn_function_type **dynFunc);
int dynFunction_parseWithStr(const char *descriptor, dyn_type_list_type *typeReferences, dyn_function_type **dynFunc);

void dynFunction_destroy(dyn_function_type *dynFunc);
int dynFunction_call(dyn_function_type *dynFunc, void(*fn)(void), void *returnValue, void **argValues);

int dynFunction_createClosure(dyn_function_type *func, void (*bind)(void *, void **, void*), void *userData, void(**fn)(void));
int dynFunction_getFnPointer(dyn_function_type *func, void (**fn)(void));

#endif
