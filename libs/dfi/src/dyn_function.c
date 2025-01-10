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
#include "celix_cleanup.h"
#include "celix_err.h"
#include "celix_stdlib_cleanup.h"

#include <strings.h>
#include <stdlib.h>
#include <ffi.h>
#include "dyn_type_common.h"

static const int OK = 0;
static const int MEM_ERROR = 1;
static const int PARSE_ERROR = 2;
static const int ERROR = 2;

static int dynFunction_initCif(dyn_function_type* dynFunc);
static int dynFunction_parseDescriptor(dyn_function_type* dynFunc, FILE* descriptor);
static void dynFunction_ffiBind(ffi_cif* cif, void* ret, void* args[], void* userData);

int dynFunction_parse(FILE* descriptor, struct types_head* refTypes, dyn_function_type** out) {
    int status = OK;
    celix_autoptr(dyn_function_type) dynFunc = calloc(1, sizeof(*dynFunc));
    if (dynFunc == NULL) {
        celix_err_pushf("Error allocating memory for dyn function");
        return MEM_ERROR;
    }

    TAILQ_INIT(&dynFunc->arguments);
    dynFunc->refTypes = refTypes;
    status = dynFunction_parseDescriptor(dynFunc, descriptor);
    if (status != OK)  {
        celix_err_pushf("Error parsing descriptor");
        return status;
    }
    status = dynFunction_initCif(dynFunc);
    if (status != OK) {
        return status;
    }

    dyn_function_argument_type* arg = NULL;
    TAILQ_FOREACH(arg, &dynFunc->arguments, entries) {
        const dyn_type* real = dynType_realType(arg->type);
        const char* meta = dynType_getMetaInfo(arg->type, "am");
        if (meta == NULL) {
            arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__STD;
        } else if (strcmp(meta, "handle") == 0) {
            if (dynType_descriptorType(real) != 'P') {
                celix_err_pushf("Error 'handle' is only allowed for untyped pointer not '%c'", dynType_descriptorType(real));
                return PARSE_ERROR;
            }
            arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__HANDLE;
        } else if (strcmp(meta, "pre") == 0) {
            if (dynType_type(real) != DYN_TYPE_TYPED_POINTER) {
                celix_err_pushf("Error 'pre' is only allowed for typed pointer not '%c'", dynType_descriptorType(real));
                return PARSE_ERROR;
            }
            const dyn_type* sub = dynType_typedPointer_getTypedType(real);
            if (!dynType_isTrivial(sub)) {
                celix_err_pushf("Error 'pre' is only allowed for pointer to trivial types not non-trivial '%c'", dynType_descriptorType(sub));
                return PARSE_ERROR;
            }
            arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT;
        } else if (strcmp(meta, "out") == 0) {
            if (dynType_type(real) != DYN_TYPE_TYPED_POINTER) {
                celix_err_pushf("Error 'out' is only allowed for typed pointer not '%c'", dynType_descriptorType(real));
                return PARSE_ERROR;
            }
            const dyn_type* sub = dynType_typedPointer_getTypedType(real);
            int subType = dynType_type(sub);
            if (subType != DYN_TYPE_TEXT && subType != DYN_TYPE_TYPED_POINTER
                && subType != DYN_TYPE_BUILTIN_OBJECT) {
                celix_err_pushf("Error 'out' is only allowed for pointer to text or built-in object or typed pointer not to '%c'",
                                dynType_descriptorType(sub));
                return PARSE_ERROR;
            }
            arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__OUTPUT;
        } else {
            arg->argumentMeta = DYN_FUNCTION_ARGUMENT_META__STD;
        }
    }

    *out = celix_steal_ptr(dynFunc);
    return OK;
}

int dynFunction_parseWithStr(const char* descriptor, struct types_head* refTypes, dyn_function_type** out)  {
    int status = OK;
    FILE* stream = fmemopen((char* )descriptor, strlen(descriptor), "r");
    if (stream == NULL) {
        celix_err_pushf("Error creating mem stream for descriptor string. %s", strerror(errno));
        return MEM_ERROR;
    }
    status = dynFunction_parse(stream, refTypes, out);
    fclose(stream);
    return status;
}

static int dynFunction_parseDescriptor(dyn_function_type* dynFunc, FILE* descriptor) {
    int status = OK;

    if ((status = dynCommon_parseName(descriptor, &dynFunc->name)) != OK) {
        return status;
    }

    status = dynCommon_eatChar(descriptor, '(');
    if (status != OK) {
        return PARSE_ERROR;
    }

    int nextChar = fgetc(descriptor);
    int index = 0;
    while (nextChar != ')' && nextChar != EOF)  {
        ungetc(nextChar, descriptor);

        celix_autoptr(dyn_type) type = NULL;
        dyn_function_argument_type* arg = NULL;

        if ((status = dynType_parse(descriptor, NULL, dynFunc->refTypes, &type)) != OK) {
            return status;
        }

        arg = calloc(1, sizeof(*arg));
        if (arg == NULL) {
            celix_err_pushf("Error allocating arg");
            return MEM_ERROR;
        }
        arg->index = index++;
        arg->type = celix_steal_ptr(type);

        TAILQ_INSERT_TAIL(&dynFunc->arguments, arg, entries);

        nextChar = fgetc(descriptor);
    }
    if (nextChar != ')') {
        celix_err_push("Error missing ')'");
        return PARSE_ERROR;
    }

    return dynType_parse(descriptor, NULL, dynFunc->refTypes, &dynFunc->funcReturn);
}

enum dyn_function_argument_meta dynFunction_argumentMetaForIndex(const dyn_function_type* dynFunc, int argumentNr) {
    enum dyn_function_argument_meta result = 0;
    dyn_function_argument_type* arg = NULL;
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

const struct dyn_function_arguments_head* dynFunction_arguments(const dyn_function_type* dynFunc) {
    return &dynFunc->arguments;
}

static int dynFunction_initCif(dyn_function_type* dynFunc) {
    unsigned int nargs = 0;
    dyn_function_argument_type* entry = NULL;
    TAILQ_FOREACH(entry, &dynFunc->arguments, entries) {
        nargs +=1;
    }

    dynFunc->ffiArguments = calloc(nargs, sizeof(ffi_type*));
    if (dynFunc->ffiArguments == NULL) {
        celix_err_push("Error allocating memory for ffi args");
        return MEM_ERROR;
    }

    TAILQ_FOREACH(entry, &dynFunc->arguments, entries) {
        dynFunc->ffiArguments[entry->index] = dynType_ffiType(entry->type);
    }
    
    ffi_type** args = dynFunc->ffiArguments;
    ffi_type* returnType = dynType_ffiType(dynFunc->funcReturn);

    int ffiResult = ffi_prep_cif(&dynFunc->cif, FFI_DEFAULT_ABI, nargs, returnType, args);
    if (ffiResult != FFI_OK) {
        celix_err_pushf("Error ffi_prep_cif %d", ffiResult);
        return ERROR;
    }

    return OK;
}

void dynFunction_destroy(dyn_function_type* dynFunc) {
    if (dynFunc != NULL) {
        // release resource in strict reverse order
        if (dynFunc->ffiClosure != NULL) {
            ffi_closure_free(dynFunc->ffiClosure);
        }
        if (dynFunc->ffiArguments != NULL) {
            free(dynFunc->ffiArguments);
        }
        if (dynFunc->funcReturn != NULL) {
            dynType_destroy(dynFunc->funcReturn);
        }
        dyn_function_argument_type* entry = NULL;
        dyn_function_argument_type* tmp = NULL;
        entry = TAILQ_FIRST(&dynFunc->arguments);
        while (entry != NULL) {
            dynType_destroy(entry->type);
            tmp = entry;
            entry = TAILQ_NEXT(entry, entries);
            free(tmp);
        }
        if (dynFunc->name != NULL) {
            free(dynFunc->name);
        }
        free(dynFunc);
    }
}

int dynFunction_call(const dyn_function_type* dynFunc, void(*fn)(void), void* returnValue, void** argValues) {
    ffi_call((ffi_cif*)&dynFunc->cif, fn, returnValue, argValues);
    return 0;
}

static void dynFunction_ffiBind(ffi_cif* cif, void* ret, void* args[], void* userData) {
    dyn_function_type* dynFunc = userData;
    dynFunc->bind(dynFunc->userData, args, ret);
}

CELIX_DEFINE_AUTOPTR_CLEANUP_FUNC(ffi_closure, ffi_closure_free)

int dynFunction_createClosure(dyn_function_type* dynFunc, void (*bind)(void*, void**, void*), void* userData, void(**out)(void)) {
    void (*fn)(void);
    celix_autoptr(ffi_closure) ffiClosure = ffi_closure_alloc(sizeof(ffi_closure), (void **)&fn);
    if (ffiClosure == NULL) {
        return MEM_ERROR;
    }
    int rc = ffi_prep_closure_loc(ffiClosure, &dynFunc->cif, dynFunction_ffiBind, dynFunc, fn);
    if (rc != FFI_OK) {
        return ERROR;
    }

    dynFunc->ffiClosure = celix_steal_ptr(ffiClosure);
    dynFunc->userData = userData;
    dynFunc->bind = bind;
    dynFunc->fn = fn;
    *out =fn;

    return OK;
}

int dynFunction_getFnPointer(const dyn_function_type* dynFunc, void (**fn)(void)) {
    if (dynFunc == NULL || dynFunc->fn == NULL) {
        return ERROR;
    }
    (*fn) = dynFunc->fn;
    return OK;
}

int dynFunction_nrOfArguments(const dyn_function_type* dynFunc) {
    dyn_function_argument_type* last = TAILQ_LAST(&dynFunc->arguments, dyn_function_arguments_head);
    return last == NULL ? 0 : (last->index+1);
}

const dyn_type* dynFunction_argumentTypeForIndex(const dyn_function_type* dynFunc, int argumentNr) {
    dyn_type* result = NULL;
    int index = 0;
    dyn_function_argument_type* entry = NULL;
    TAILQ_FOREACH(entry, &dynFunc->arguments, entries) {
        if (index == argumentNr) {
            result = entry->type;
            break;
        }
        index +=1;
    }
    return result;
}

const dyn_type* dynFunction_returnType(const dyn_function_type *dynFunction) {
    return dynFunction->funcReturn;
}

bool dynFunction_hasReturn(const dyn_function_type* dynFunction) {
    const dyn_type* t = dynFunction_returnType(dynFunction);
    return t->descriptor != 'V';
}

const char* dynFunction_getName(const dyn_function_type* func) {
    return func->name;
}