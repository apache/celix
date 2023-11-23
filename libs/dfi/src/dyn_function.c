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

#include <strings.h>
#include <stdlib.h>
#include <ffi.h>
#include "dyn_type_common.h"

static const int OK = 0;
static const int MEM_ERROR = 1;
static const int PARSE_ERROR = 2;
static const int ERROR = 2;

DFI_SETUP_LOG(dynFunction)

static int dynFunction_initCif(dyn_function_type *dynFunc);
static int dynFunction_parseDescriptor(dyn_function_type *dynFunc, FILE *descriptor);
static void dynFunction_ffiBind(ffi_cif *cif, void *ret, void *args[], void *userData);

ffi_type * dynType_ffiType(dyn_type *type);

int dynFunction_parse(FILE *descriptor, struct types_head *refTypes, dyn_function_type **out) {
    int status = OK;
    dyn_function_type *dynFunc = NULL;
    
    dynFunc = calloc(1, sizeof(*dynFunc));

    if (dynFunc != NULL) {
        TAILQ_INIT(&dynFunc->arguments);
        dynFunc->refTypes = refTypes;
        status = dynFunction_parseDescriptor(dynFunc, descriptor);
        if (status == 0) {
            int rc = dynFunction_initCif(dynFunc);
            if (rc != 0) {
                LOG_ERROR("Error initializing cif");
                status = ERROR;
            }
        }
    } else {
        LOG_ERROR("Error allocating memory for dyn function\n");
        status = MEM_ERROR;
    }

    if (status == OK) {
        dyn_function_argument_type *arg = NULL;
        TAILQ_FOREACH(arg, &dynFunc->arguments, entries) {
            const char *meta = dynType_getMetaInfo(arg->type, "am");
            if (meta == NULL) {
                arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__STD;
            } else if (strcmp(meta, "handle") == 0) {
                arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__HANDLE;
            } else if (strcmp(meta, "pre") == 0) {
                arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT;
            } else if (strcmp(meta, "out") == 0) {
                arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__OUTPUT;
            } else {
                arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__STD;
            }
        }
    }
    
    if (status == OK) {
        *out = dynFunc;
    }    else {
        LOG_ERROR("Failed to Create dyn function");
        if (dynFunc != NULL) {
            dynFunction_destroy(dynFunc);
        }

    }
    
    return status;
}

int dynFunction_parseWithStr(const char *descriptor, struct types_head *refTypes, dyn_function_type **out)  {
    int status = OK;
    FILE *stream = fmemopen((char *)descriptor, strlen(descriptor) + 1, "r");
    if (stream != NULL) {
        status = dynFunction_parse(stream, refTypes, out);
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
    char argName[32];
    while (nextChar != ')' && status == 0)  {
        ungetc(nextChar, descriptor);
        type = NULL;

        dyn_function_argument_type *arg = NULL;

        status = dynType_parse(descriptor, NULL, dynFunc->refTypes, &type);
        if (status == OK) {
            arg = calloc(1, sizeof(*arg));
            if (arg != NULL) {
                arg->index = index;
                arg->type = type;
                snprintf(argName, 32, "arg%04i", index);
                arg->name = strdup(argName);

                index += 1;
            } else {
                LOG_ERROR("Error allocating memory");
                status = MEM_ERROR;
            }
        }

        if (status == OK) {
            TAILQ_INSERT_TAIL(&dynFunc->arguments, arg, entries);
        }

        nextChar = fgetc(descriptor);
    }

    if (status == 0) {
        status = dynType_parse(descriptor, NULL, dynFunc->refTypes, &dynFunc->funcReturn); 
    }
    
    return status;
}

enum dyn_function_argument_meta dynFunction_argumentMetaForIndex(dyn_function_type *dynFunc, int argumentNr) {
    enum dyn_function_argument_meta result = 0;
    dyn_function_argument_type *arg = NULL;
    int index = 0;
    TAILQ_FOREACH(arg, &dynFunc->arguments, entries) {
        if (index == argumentNr) {
            result = arg->argumentMeta;
            break;
        }
        index += 1;
    }
    return result;
}


static int dynFunction_initCif(dyn_function_type *dynFunc) {
    int status = 0;

    unsigned int nargs = 0;
    dyn_function_argument_type *entry = NULL;
    TAILQ_FOREACH(entry, &dynFunc->arguments, entries) {
        nargs +=1;
    }

    dynFunc->ffiArguments = calloc(nargs, sizeof(ffi_type*));

    TAILQ_FOREACH(entry, &dynFunc->arguments, entries) {
        dynFunc->ffiArguments[entry->index] = dynType_ffiType(entry->type);
    }
    
    ffi_type **args = dynFunc->ffiArguments;
    ffi_type *returnType = dynType_ffiType(dynFunc->funcReturn);

    int ffiResult = ffi_prep_cif(&dynFunc->cif, FFI_DEFAULT_ABI, nargs, returnType, args);
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
        dynFunc->userData = userData;
        dynFunc->bind = bind;
        dynFunc->fn = fn;
        *out =fn;
    }

    return status;
}

int dynFunction_getFnPointer(dyn_function_type *dynFunc, void (**fn)(void)) {
    int status = 0;
    if (dynFunc != NULL && dynFunc->fn != NULL) {
        (*fn) = dynFunc->fn;
    } else {
        status = 1;
    }
    return status;
}

int dynFunction_nrOfArguments(dyn_function_type *dynFunc) {
    int count = 0;
    dyn_function_argument_type *entry = NULL;
    TAILQ_FOREACH(entry, &dynFunc->arguments, entries) {
        count += 1;
    }
    return count;
}

dyn_type *dynFunction_argumentTypeForIndex(dyn_function_type *dynFunc, int argumentNr) {
    dyn_type *result = NULL;
    int index = 0;
    dyn_function_argument_type *entry = NULL;
    TAILQ_FOREACH(entry, &dynFunc->arguments, entries) {
        if (index == argumentNr) {
            result = entry->type;
            break;
        }
        index +=1;
    }
    return result;
}

dyn_type * dynFunction_returnType(dyn_function_type *dynFunction) {
    return dynFunction->funcReturn;
}

bool dynFunction_hasReturn(dyn_function_type *dynFunction) {
    dyn_type *t = dynFunction_returnType(dynFunction);
    return t->descriptor != 'V';
}