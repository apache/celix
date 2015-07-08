/*
 * Licensed under Apache License v2. See LICENSE for more information.
 */
#include "dyn_function.h"

#include <stdio.h>
#include <stdarg.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include <ffi.h>

#include "dyn_type.h"
#include "dfi_log_util.h"

DFI_SETUP_LOG(dynFunction)

struct _dyn_function_type {
    dyn_type *arguments;
    dyn_type *funcReturn;
    void (*fn)(void);
    ffi_cif cif;
};

struct _dyn_closure_type {
    dyn_type *arguments;
    dyn_type *funcReturn;
    ffi_cif cif;
    ffi_closure *ffiClosure;
    void (*fn)(void);
    void (*bind)(void *userData, void *args[], void *ret);
    void *userData; //for bind
};


static int dynFunction_initCif(ffi_cif *cif, dyn_type *arguments, dyn_type  *funcReturn);
static int dynFunction_parseDescriptor(const char *functionDescriptor, dyn_type_list_type *typeReferences, dyn_type **arguments, dyn_type **funcReturn);
static void dynClosure_ffiBind(ffi_cif *cif, void *ret, void *args[], void *userData); 

int dynFunction_create(const char *descriptor, dyn_type_list_type *typeReferences, void (*fn)(void), dyn_function_type **out)  {
    int status = 0;
    dyn_function_type *dynFunc = NULL;
    LOG_DEBUG("Creating dyn function for descriptor '%s'\n", descriptor);
    
    dynFunc = calloc(1, sizeof(*dynFunc));

    if (dynFunc != NULL) {
        dynFunc->fn = fn;
        status = dynFunction_parseDescriptor(descriptor, typeReferences, &dynFunc->arguments, &dynFunc->funcReturn);
        if (status == 0) {
            status = dynFunction_initCif(&dynFunc->cif, dynFunc->arguments, dynFunc->funcReturn);
        }
    } else {
        status = 2;
    }
    
    if (status == 0) {
        *out = dynFunc;
    } else {
        if (status == 1) {
            LOG_ERROR("Cannot parse func descriptor '%s'\n", descriptor);
        } else {
            LOG_ERROR("Cannot allocate memory for dyn function\n");
        }
    }
    return status;
}

static int dynFunction_parseDescriptor(const char *descriptor, dyn_type_list_type *typeReferences, dyn_type **arguments, dyn_type **funcReturn) {
    int status = 0;
    char *startPos = index(descriptor, '(');
    char *endPos = index(descriptor, ')');

    if (startPos != NULL && endPos != NULL) {
        int len = endPos - startPos - 1;

        //TODO add names (arg001, arg002, etc)
        char argDesc[len+3];
        argDesc[0] = '{';
        argDesc[len+1] = '}';
        memcpy(argDesc+1, startPos +1, len);
        argDesc[len+2] = '\0';
        LOG_DEBUG("argDesc is '%s'\n", argDesc);

        len = strlen(endPos);
        char returnDesc[len+1];
        memcpy(returnDesc, endPos + 1, len);
        returnDesc[len] = '\0';
        LOG_DEBUG("returnDesc is '%s'\n", returnDesc);

        status = dynType_create(argDesc, typeReferences, arguments);
        if (status == 0) {
            status = dynType_create(returnDesc, typeReferences, funcReturn);
        } 
    } else {
        status = 1;
    }
    
    return status;
}

static int dynFunction_initCif(ffi_cif *cif, dyn_type *arguments, dyn_type *returnValue) {
    int result = 0;

    int count = 0;
    int i;
    for (i = 0; arguments->ffiType->elements[i] != NULL; i += 1) {
        count += 1;
    }

    ffi_type **args = arguments->ffiType->elements;
    ffi_type *returnType = returnValue->ffiType;

    int ffiResult = ffi_prep_cif(cif, FFI_DEFAULT_ABI, count, returnType, args);
    if (ffiResult != FFI_OK) {
        result = 1;
    }

    return result;
}

int dynFunction_destroy(dyn_function_type *dynFunc) {
    int result = 0;
    if (dynFunc != NULL) {
        if (dynFunc->arguments != NULL) {
	    dynType_destroy(dynFunc->arguments);
	}
        if (dynFunc->funcReturn != NULL) {
	    dynType_destroy(dynFunc->funcReturn);
	}
	free(dynFunc);
    }
    return result;
}

int dynFunction_call(dyn_function_type *dynFunc, void *returnValue, void **argValues) {
    ffi_call(&dynFunc->cif, dynFunc->fn, returnValue, argValues);    
    return 0;
}

static void dynClosure_ffiBind(ffi_cif *cif, void *ret, void *args[], void *userData) {
    dyn_closure_type *dynClosure = userData;
    dynClosure->bind(dynClosure->userData, args, ret);
}

int dynClosure_create(const char *descriptor, dyn_type_list_type *typeReferences, void (*bind)(void *, void **, void*), void *userData, dyn_closure_type **out) {
    int status = 0;
    dyn_closure_type *dynClosure = calloc(1, sizeof(*dynClosure));
    if (dynClosure != NULL) {
        dynClosure->bind = bind;
        dynClosure->userData = userData;
        status = dynFunction_parseDescriptor(descriptor, typeReferences, &dynClosure->arguments, &dynClosure->funcReturn);
        if (status == 0) {
            status = dynFunction_initCif(&dynClosure->cif, dynClosure->arguments, dynClosure->funcReturn);
            if (status == 0) {
                dynClosure->ffiClosure = ffi_closure_alloc(sizeof(ffi_closure), (void **)&dynClosure->fn);
                if (dynClosure->ffiClosure != NULL) {
                    int rc = ffi_prep_closure_loc(dynClosure->ffiClosure, &dynClosure->cif, dynClosure_ffiBind, dynClosure, dynClosure->fn);
                    if (rc != FFI_OK) {
                        status = 1;
                    }
                } else {
                    status = 2;
                }
            }
        }
    } else {
        status = 2;
    }

    if (status == 0) {
        *out = dynClosure;
    }
    return status;
}

int dynClosure_getFnPointer(dyn_closure_type *dynClosure, void (**fn)(void)) {
    int status = 0;
    if (dynClosure != NULL) {
        (*fn) = dynClosure->fn;
    } else {
        status = 1;
    }
    return status;
}


int dynClosure_destroy(dyn_closure_type *dynClosure) {
    int result = 0;
    if (dynClosure != NULL) {
        if (dynClosure->arguments != NULL) {
	    dynType_destroy(dynClosure->arguments);
	}
        if (dynClosure->funcReturn != NULL) {
	    dynType_destroy(dynClosure->funcReturn);
	}
	if (dynClosure->ffiClosure != NULL) {
	    ffi_closure_free(dynClosure->ffiClosure);
	}
	free(dynClosure);
    }
    return result;
}
