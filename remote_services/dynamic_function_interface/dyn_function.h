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

int dynFunction_create(FILE *descriptorStream, dyn_type_list_type *typeReferences, void (*fn)(void), dyn_function_type **dynFunc);
int dynFunction_createWithStr(const char *descriptor, dyn_type_list_type *typeReferences, void (*fn)(void), dyn_function_type **dynFunc);
void dynFunction_destroy(dyn_function_type *dynFunc);
int dynFunction_call(dyn_function_type *dynFunc, void *returnValue, void **argValues);

int dynClosure_create(FILE *descriptorStream, dyn_type_list_type *typeReferences, void (*bind)(void *, void **, void*), void *userData, dyn_closure_type **out);
int dynClosure_createWithStr(const char *descriptor, dyn_type_list_type *typeReferences, void (*bind)(void *, void **, void*), void *userData, dyn_closure_type **out);
int dynClosure_getFnPointer(dyn_closure_type *dynClosure, void(**fn)(void));
void dynClosure_destroy(dyn_closure_type *dynClosure);

#endif
