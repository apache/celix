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

#ifndef _DYN_INTERFACE_COMMON_H_
#define _DYN_INTERFACE_COMMON_H_

#include "dyn_interface.h"

#include <strings.h>
#include <stdlib.h>
#include <ffi.h>

#include "dyn_common.h"

#ifdef __cplusplus
extern "C" {
#endif

struct _dyn_interface_type {
    struct namvals_head header;
    struct namvals_head annotations;
    struct types_head types;
    struct methods_head methods;
    version_pt version;
};

#ifdef __cplusplus
}
#endif

#endif
