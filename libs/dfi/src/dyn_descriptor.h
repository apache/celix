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

#ifndef CELIX_DYN_DESCRIPTOR_H
#define CELIX_DYN_DESCRIPTOR_H
#ifdef __cplusplus
extern "C" {
#endif

#include "dyn_common.h"
#include "dyn_type.h"
#include "celix_version.h"

#include <stdio.h>

#define CELIX_DESCRIPTOR_FIELDS                                               \
    /* public */                                                              \
    struct namvals_head header;                                               \
    struct namvals_head annotations;                                          \
    struct types_head types;                                                  \
    celix_version_t* version;                                                 \

typedef struct celix_descriptor {
    CELIX_DESCRIPTOR_FIELDS
}celix_descriptor_t;

int celix_dynDescriptor_parse(celix_descriptor_t* descriptor, FILE* stream,
                              int (*parseSection)(celix_descriptor_t* descriptor, const char* secName, FILE *stream));

void celix_dynDescriptor_destroy(celix_descriptor_t* descriptor);

#ifdef __cplusplus
}
#endif
#endif //CELIX_DYN_DESCRIPTOR_H
