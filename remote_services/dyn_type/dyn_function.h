#ifndef __DYN_FUNCTION_H_
#define __DYN_FUNCTION_H_

#include <ffi.h>
#include "dyn_type.h"

/**
 * Uses the following schema
 * (Name)([Type]*)Type
 * e.g add(DD)D or sum({[D[D setA setB})D
 */

typedef struct _dyn_function_type dyn_function_type;
typedef struct _dyn_closure_type dyn_closure_type;

int dynFunction_create(const char *schema, void (*fn)(void), dyn_function_type **dynFunc);
int dynFunction_destroy(dyn_function_type *dynFunc);
int dynFunction_call(dyn_function_type *dynFunc, void *returnValue, void **argValues);

int dynClosure_create(const char *schema, void (*bind)(void *, void **, void*), void *userData, dyn_closure_type **out);
int dynClosure_getFnPointer(dyn_closure_type *dynClosure, void(**fn)(void));
int dynClosure_destroy(dyn_closure_type *dynClosure);

#endif
