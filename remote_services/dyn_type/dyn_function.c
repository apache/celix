#include "dyn_function.h"

#include <stdio.h>
#include <stdarg.h>
#include <strings.h>
#include <string.h>
#include <stdlib.h>

#include <ffi.h>

#include "dyn_type.h"

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
    void *userData;
};


static int dynFunction_initCif(ffi_cif *cif, dyn_type *arguments, dyn_type  *funcReturn);
static int dynFunction_parseSchema(const char *schema, dyn_type **arguments, dyn_type **funcReturn);
static void dynClosure_ffiBind(ffi_cif *cif, void *ret, void *args[], void *userData); 

int dynFunction_create(const char *schema, void (*fn)(void), dyn_function_type **out)  {
    int status = 0;
    dyn_function_type *dynFunc = NULL;
    
    dynFunc = calloc(1, sizeof(*dynFunc));

    if (dynFunc != NULL) {
        dynFunc->fn = fn;
        status = dynFunction_parseSchema(schema, &dynFunc->arguments, &dynFunc->funcReturn);
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
            printf("Cannot parse func schema '%s'\n", schema);
        } else {
            printf("Cannot allocate memory for dyn function\n");
        }
    }
    return status;
}

static int dynFunction_parseSchema(const char *schema, dyn_type **arguments, dyn_type **funcReturn) {
    int status = 0;
    //FIXME white space parsing is missing. Note move parsing to lex/bison?
    //TODO move to parse schema function
    char *startPos = index(schema, '(');
    char *endPos = index(schema, ')');

    if (startPos != NULL && endPos != NULL) {
        int nrOfArgs = 0;
        int len = endPos - startPos - 1;

        char *pos = startPos + 1;
        while (*pos != ')') {
            nrOfArgs += 1;
            if (*pos == '{') { //embedded struct
                while (*pos != '}' && *pos != '\0') {
                    pos += 1;
                }
            }
            pos += 1;
        }

        printf("nrOfAgrs is %i\n", nrOfArgs);

        //length needed is len args +2 -> {DD}
        //+ 7 per arg e.g. {DD arg001 arg002}
        //+ 1 x \0
        //--> len args + 3 + 7 * nrArgs
        int argLength = 3 + len + 7 * nrOfArgs;

        char argSchema[argLength];
        argSchema[0] = '{';
        memcpy(argSchema + 1, startPos + 1, len);
        pos += len + 1;
        int i;
        for (i = 0 ; i < nrOfArgs; i += 1) {
            sprintf(argSchema + 1 + len + 7 * i, " arg%03d", i);
            pos += 7;
        }
        argSchema[argLength -2] = '}';
        argSchema[argLength -1] = '\0';
        printf("arg schema is '%s'\n", argSchema);

        size_t returnLen = strlen(endPos + 1);
        char returnSchema[returnLen + 1];
        memcpy(returnSchema, endPos +1, returnLen);
        returnSchema[returnLen] = '\0';
        printf("return schema is '%s'\n", returnSchema);

        status = dynType_create(argSchema, arguments);
        if (status == 0) {
            status = dynType_create(returnSchema, funcReturn);
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
    for (i = 0; arguments->complex.ffiType.elements[i] != NULL; i += 1) {
        count += 1;
    }

    ffi_type **args = arguments->complex.ffiType.elements;
    ffi_type *returnType = &ffi_type_pointer;
    if (returnType->type == DYN_TYPE_SIMPLE) {
        returnType = returnValue->simple.ffiType;
    }


    int ffiResult = ffi_prep_cif(cif, FFI_DEFAULT_ABI, count, returnType, args);
    if (ffiResult != FFI_OK) {
        result = 1;
    }

    return result;
}

int dynFunction_destroy(dyn_function_type *dynFunc) {
    int result = 0;
    printf("TODO destroy dyn dync\n");
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

int dynClosure_create(const char *schema, void (*bind)(void *, void **, void*), void *userData, dyn_closure_type **out) {
    int status = 0;
    dyn_closure_type *dynClosure = calloc(1, sizeof(*dynClosure));
    if (dynClosure != NULL) {
        dynClosure->bind = bind;
        dynClosure->userData = userData;
        status = dynFunction_parseSchema(schema, &dynClosure->arguments, &dynClosure->funcReturn);
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
    printf("TODO destroy closure\n");
    return result;
}
