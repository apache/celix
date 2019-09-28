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
/**
 * utils.h
 *
 *  \date       Jul 27, 2010
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <ctype.h>

#include "celix_errno.h"
#include "celixbool.h"
#include "exports.h"
#include "celix_threads.h"

#ifdef __cplusplus
extern "C" {
#endif

UTILS_EXPORT unsigned int utils_stringHash(const void *string);

UTILS_EXPORT int utils_stringEquals(const void *string, const void *toCompare);

UTILS_EXPORT char *string_ndup(const char *s, size_t n);

UTILS_EXPORT char *utils_stringTrim(char *string);

UTILS_EXPORT bool utils_isStringEmptyOrNull(const char *const str);

UTILS_EXPORT int
utils_compareServiceIdsAndRanking(unsigned long servId, long servRank, unsigned long otherServId, long otherServRank);

UTILS_EXPORT celix_status_t thread_equalsSelf(celix_thread_t thread, bool *equals);

UTILS_EXPORT celix_status_t utils_isNumeric(const char *number, bool *ret);

UTILS_EXPORT double celix_difftime(const struct timespec *tBegin, const struct timespec *tEnd);

#ifdef __cplusplus
}
#endif
#endif /* UTILS_H_ */
