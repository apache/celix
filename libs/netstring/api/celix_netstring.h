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

#ifndef CELIX_CELIX_NETSTRING_H
#define CELIX_CELIX_NETSTRING_H
#ifdef __cplusplus
extern "C" {
#endif
#include "celix_netstring_export.h"
#include "celix_errno.h"
#include <stdio.h>
#include <stddef.h>

CELIX_NETSTRING_EXPORT
celix_status_t celix_netstring_encodef(const char* bytes, size_t bytesSize, FILE* netstringOut);

CELIX_NETSTRING_EXPORT
celix_status_t celix_netstring_encodeb(const char* bytes, size_t bytesSize, char* netstringBufferOut, size_t bufferSize, size_t* bufferOffsetInOut);

CELIX_NETSTRING_EXPORT
celix_status_t celix_netstring_decodef(FILE* netstringIn, char** bytes, size_t* bytesSize);

CELIX_NETSTRING_EXPORT
celix_status_t celix_netstring_decodeb(const char* bufferIn, size_t bufferInSize, const char** bytes, size_t* bytesSize);


#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_NETSTRING_H
