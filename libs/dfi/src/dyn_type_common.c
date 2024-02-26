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
#include "celix_err.h"

#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ffi.h>

dyn_type* dynType_findType(dyn_type *type, char *name) {
    for (dyn_type *current = type; current != NULL; current = current->parent) {
        struct type_entry *entry = NULL;
        if (current->name != NULL && strcmp(current->name, name) == 0) {
            return current;
        }
        if (current->referenceTypes != NULL) {
            TAILQ_FOREACH(entry, current->referenceTypes, entries) {
                if (strcmp(name, entry->type->name) == 0) {
                    return entry->type;
                }
            }
        }

        struct type_entry *nEntry = NULL;
        TAILQ_FOREACH(nEntry, &current->nestedTypesHead, entries) {
            if (strcmp(name, nEntry->type->name) == 0) {
                return nEntry->type;
            }
        }
    }
    return NULL;
}

ffi_type* dynType_ffiType(const dyn_type * type) {
    return dynType_realType(type)->ffiType;
}

