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

#ifndef _DYN_TYPE_COMMON_H_
#define _DYN_TYPE_COMMON_H_

#include "dyn_common.h"
#include "dyn_type.h"

#include <ffi.h>


#ifdef __cplusplus
extern "C" {
#endif

struct _dyn_type {
    char* name;
    char descriptor;
    int type;
    ffi_type* ffiType;
    dyn_type* parent;
    const struct types_head* referenceTypes; //NOTE: not owned
    struct types_head nestedTypesHead;
    struct meta_properties_head metaProperties;
    union {
        struct {
            struct complex_type_entries_head entriesHead;
            ffi_type structType; //dyn_type.ffiType points to this
            dyn_type** types; //based on entriesHead for fast access
        } complex;
        struct {
            ffi_type seqType; //dyn_type.ffiType points to this
            dyn_type* itemType;
        } sequence;
        struct {
            dyn_type* typedType;
        } typedPointer;
        struct {
            dyn_type* ref;
        } ref;
    };
};

dyn_type* dynType_findType(dyn_type* type, char* name);
ffi_type* dynType_ffiType(const dyn_type* type);

#ifdef __cplusplus
}
#endif

#endif
