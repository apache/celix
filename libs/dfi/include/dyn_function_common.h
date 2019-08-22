/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */

#ifndef _DYN_FUNCTION_COMMON_H_
#define _DYN_FUNCTION_COMMON_H_

#include "dyn_function.h"

#include <strings.h>
#include <stdlib.h>
#include <ffi.h>

#include "dyn_common.h"

struct _dyn_function_type {
    char *name;
    struct types_head *refTypes; //NOTE not owned
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
    enum dyn_function_argument_meta argumentMeta;
    dyn_type *type;
    TAILQ_ENTRY(_dyn_function_argument_type) entries;
};

#endif
