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

#include "dyn_type.h"

#include "dyn_type_common.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ffi.h>

DFI_SETUP_LOG(dynTypeCommon)

dyn_type * dynType_findType(dyn_type *type, char *name) {
    dyn_type *result = NULL;

    struct type_entry *entry = NULL;
    if (type->referenceTypes != NULL) {
        TAILQ_FOREACH(entry, type->referenceTypes, entries) {
            LOG_DEBUG("checking ref type '%s' with name '%s'", entry->type->name, name);
            if (strcmp(name, entry->type->name) == 0) {
                result = entry->type;
                break;
            }
        }
    }

    if (result == NULL) {
        struct type_entry *nEntry = NULL;
        TAILQ_FOREACH(nEntry, &type->nestedTypesHead, entries) {
            LOG_DEBUG("checking nested type '%s' with name '%s'", nEntry->type->name, name);
            if (strcmp(name, nEntry->type->name) == 0) {
                result = nEntry->type;
                break;
            }
        }
    }

    if (result == NULL && type->parent != NULL) {
        result = dynType_findType(type->parent, name);
    }

    return result;
}

ffi_type * dynType_ffiType(dyn_type * type) {
    if (type->type == DYN_TYPE_REF) {
        if (type->ref.ref == NULL) {
            LOG_ERROR("Error. Ref for %s is not (yet) initialized", type->name);
            return NULL;
        }
        return type->ref.ref->ffiType;
    }
    return type->ffiType;
}

void dynType_prepCif(ffi_type *type) {
    ffi_cif cif;
    ffi_type *args[1];
    args[0] = type;
    ffi_prep_cif(&cif, FFI_DEFAULT_ABI, 1, &ffi_type_uint, args);
}

