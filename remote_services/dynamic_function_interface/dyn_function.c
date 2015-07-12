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

#include "dyn_common.h"
#include "dyn_type.h"
#include "dfi_log_util.h"

DFI_SETUP_LOG(dynFunction)

struct _dyn_function_type {
    char *name;
    dyn_type_list_type *refTypes; //NOTE not owned
    TAILQ_HEAD(,_dyn_function_argument_type) arguments;
    ffi_type **ffiArguments;
    dyn_type *funcReturn;
    ffi_cif cif;

    //closure part
    ffi_closure *ffiClosure;
    void (*fn)(void);
    void *userData;
    void (*bind)(void *userData, void *args[], void *ret);
};

typedef struct _dyn_function_argument_type dyn_function_argument_type;
struct _dyn_function_argument_type {
    int index;
    char *name;
    dyn_type *type;
    TAILQ_ENTRY(_dyn_function_argument_type) entries;
};

static const int OK = 0;
static const int MEM_ERROR = 1;
static const int PARSE_ERROR = 2;

static int dynFunction_initCif(dyn_function_type *dynFunc);
static int dynFunction_parseDescriptor(dyn_function_type *dynFunc, FILE *descriptor);
static void dynFunction_ffiBind(ffi_cif *cif, void *ret, void *args[], void *userData); 

int dynFunction_parse(FILE *descriptor, dyn_type_list_type *typeReferences, dyn_function_type **out) {
    int status = OK;
    dyn_function_type *dynFunc = NULL;
    LOG_DEBUG("Creating dyn function", descriptor);
    
    dynFunc = calloc(1, sizeof(*dynFunc));

    if (dynFunc != NULL) {
        TAILQ_INIT(&dynFunc->arguments);
        dynFunc->refTypes = typeReferences;
        status = dynFunction_parseDescriptor(dynFunc, descriptor);
        if (status == 0) {
            int rc = dynFunction_initCif(dynFunc);
            status = rc != 0 ? rc : 0;
        }
    } else {
        LOG_ERROR("Error allocationg memory for dyn functipn\n");
        status = MEM_ERROR;
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

int dynFunction_parseWithStr(const char *descriptor, dyn_type_list_type *typeReferences, dyn_function_type **out)  {
    int status = OK;
    FILE *stream = fmemopen((char *)descriptor, strlen(descriptor), "r");
    if (stream != NULL) {
        status = dynFunction_parse(stream, typeReferences, out);
        fclose(stream);
    } else {
        status = MEM_ERROR;
        LOG_ERROR("Error creating mem stream for descriptor string. %s", strerror(errno)); 
    }
    return status;
}

static int dynFunction_parseDescriptor(dyn_function_type *dynFunc, FILE *descriptor) {
    int status = OK;
    char *name = NULL;

    status = dynCommon_parseName(descriptor, &name);

    if (status == OK) {
        dynFunc->name = name;
    }

    if (status == OK) {
        int c = fgetc(descriptor);
        if ( c != '(') {
            status = PARSE_ERROR;
            LOG_ERROR("Expected '(' token got '%c'", c);
        }
    }

    int nextChar = fgetc(descriptor);
    int index = 0;
    dyn_type *type = NULL;
    while (nextChar != ')' && status == 0)  {
        type = NULL;
        ungetc(nextChar, descriptor);
        status = dynType_parse(descriptor, NULL, dynFunc->refTypes, &type); 
        if (status == 0) {
            dyn_function_argument_type *arg = calloc(1, sizeof(*arg));
            arg->index = index++;
            arg->type = type;
            arg->name = NULL; //TODO
            if (arg != NULL) {
                TAILQ_INSERT_TAIL(&dynFunc->arguments, arg, entries);
            } else {
                LOG_ERROR("Error allocating memory");
                status = MEM_ERROR;
            }
        } 
        nextChar = fgetc(descriptor);
    }

    if (status == 0) {
        status = dynType_parse(descriptor, NULL, dynFunc->refTypes, &dynFunc->funcReturn); 
    }
    
    return status;
}

static int dynFunction_initCif(dyn_function_type *dynFunc) {
    int status = 0;

    int count = 0;
    dyn_function_argument_type *entry = NULL;
    TAILQ_FOREACH(entry, &dynFunc->arguments, entries) {
        count +=1;
    }

    dynFunc->ffiArguments = calloc(count, sizeof(ffi_type));

    TAILQ_FOREACH(entry, &dynFunc->arguments, entries) {
        dynFunc->ffiArguments[entry->index] = entry->type->ffiType;
    }
    
    ffi_type **args = dynFunc->ffiArguments;
    ffi_type *returnType = dynFunc->funcReturn->ffiType;

    int ffiResult = ffi_prep_cif(&dynFunc->cif, FFI_DEFAULT_ABI, count, returnType, args);
    if (ffiResult != FFI_OK) {
        status = 1;
    }

    return status;
}

void dynFunction_destroy(dyn_function_type *dynFunc) {
    if (dynFunc != NULL) {
        if (dynFunc->funcReturn != NULL) {
            dynType_destroy(dynFunc->funcReturn);
        }
        if (dynFunc->ffiClosure != NULL) {
            ffi_closure_free(dynFunc->ffiClosure);
        }
        if (dynFunc->name != NULL) {
            free(dynFunc->name);
        }
        if (dynFunc->ffiArguments != NULL) {
            free(dynFunc->ffiArguments);
        }
        
        dyn_function_argument_type *entry = NULL;
        dyn_function_argument_type *tmp = NULL;
        entry = TAILQ_FIRST(&dynFunc->arguments); 
        while (entry != NULL) {
            if (entry->name != NULL) {
                free(entry->name);
            }
            dynType_destroy(entry->type);
            tmp = entry;
            entry = TAILQ_NEXT(entry, entries);
            free(tmp);
        }

        free(dynFunc);
    }
}

int dynFunction_call(dyn_function_type *dynFunc, void(*fn)(void), void *returnValue, void **argValues) {
    //TODO check dynFunc arg
    ffi_call(&dynFunc->cif, fn, returnValue, argValues);    
    return 0;
}

static void dynFunction_ffiBind(ffi_cif *cif, void *ret, void *args[], void *userData) {
    dyn_function_type *dynFunc = userData;
    dynFunc->bind(dynFunc->userData, args, ret);
}

int dynFunction_createClosure(dyn_function_type *dynFunc, void (*bind)(void *, void **, void*), void *userData, void(**out)(void)) {
    int status = 0;
    void (*fn)(void);
    dynFunc->ffiClosure = ffi_closure_alloc(sizeof(ffi_closure), (void **)&fn);
    if (dynFunc->ffiClosure != NULL) {
        int rc = ffi_prep_closure_loc(dynFunc->ffiClosure, &dynFunc->cif, dynFunction_ffiBind, dynFunc, fn);
        if (rc != FFI_OK) {
            status = 1;
        }
    } else {
        status = 2;
    }

    if (status == 0) {
        dynFunc->bind = bind;
        dynFunc->fn = fn;
        *out =fn;
    }

    return status;
}

int dynClosure_getFnPointer(dyn_function_type *dynFunc, void (**fn)(void)) {
    int status = 0;
    if (dynFunc != NULL && dynFunc->fn != NULL) {
        (*fn) = dynFunc->fn;
    } else {
        status = 1;
    }
    return status;
}


