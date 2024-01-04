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

#ifndef CELIX_CELIX_UTILS_API_H_
#define CELIX_CELIX_UTILS_API_H_

#include <stdbool.h>

#include "celix_errno.h"
#include "celix_threads.h"
#include "array_list.h"
#include "hash_map.h"
#include "celix_properties.h"
#include "utils.h"
#include "celix_utils.h"
#include "version.h"
#include "version_range.h"

#if defined(NO_MEMSTREAM_AVAILABLE)
#include "memstream/open_memstream.h"
#include "memstream/fmemopen.h"
#endif

#endif //CELIX_CELIX_UTILS_API_H_
