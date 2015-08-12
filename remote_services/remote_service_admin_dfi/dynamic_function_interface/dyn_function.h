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
 * (Name)([ArgType]*)Type
 *
 * ArgType = (Type|PreAllocatedOutputType|OutputType)
 * PreAllocatedOutputType = ^(Type) #Note must be *(Type)
 * OutputType = ~(Type) #Note must be **(Type)
 * e.g add(DD)D or sum({[D[D setA setB})D
 */

//TODO maybe refactor to meta info flags (e.g context/handler, output, etc with a start/stop -> M(MetaType);

#define DYN_FUNCTION_ARG_META_UNKNOWN_TYPE 0
#define DYN_FUNCTION_ARG_META_STD_TYPE 1
#define DYN_FUNCTION_ARG_META_PRE_ALLOCATED_OUTPUT_TYPE 2
#define DYN_FUNCTION_ARG_META_OUPUT_TYPE 3
#define DYN_FUNCTION_ARG_META_HANDLE_TYPE 4

typedef struct _dyn_function_type dyn_function_type;

DFI_SETUP_LOG_HEADER(dynFunction);

int dynFunction_parse(FILE *descriptorStream, struct types_head *refTypes, dyn_function_type **dynFunc);
int dynFunction_parseWithStr(const char *descriptor, struct types_head *refTypes, dyn_function_type **dynFunc);

int dynFunction_nrOfArguments(dyn_function_type *dynFunc);
dyn_type *dynFunction_argumentTypeForIndex(dyn_function_type *dynFunc, int argumentNr);
dyn_type * dynFunction_returnType(dyn_function_type *dynFunction);
int dynFunction_argumentMetaInfoForIndex(dyn_function_type *dynFunc, int argumentNr);

void dynFunction_destroy(dyn_function_type *dynFunc);
int dynFunction_call(dyn_function_type *dynFunc, void(*fn)(void), void *returnValue, void **argValues);

int dynFunction_createClosure(dyn_function_type *func, void (*bind)(void *, void **, void*), void *userData, void(**fn)(void));
int dynFunction_getFnPointer(dyn_function_type *func, void (**fn)(void));

#endif
