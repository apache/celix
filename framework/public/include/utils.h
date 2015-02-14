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
/*
 * utils.h
 *
 *  \date       Jul 27, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <ctype.h>

#include "celix_errno.h"
#include "celixbool.h"
#include "framework_exports.h"
#include "celix_threads.h"

FRAMEWORK_EXPORT unsigned int utils_stringHash(void * string);
FRAMEWORK_EXPORT int utils_stringEquals(void * string, void * toCompare);
FRAMEWORK_EXPORT char * string_ndup(const char *s, size_t n);
FRAMEWORK_EXPORT char * utils_stringTrim(char * string);

FRAMEWORK_EXPORT celix_status_t thread_equalsSelf(celix_thread_t thread, bool *equals);

FRAMEWORK_EXPORT celix_status_t utils_isNumeric(char *number, bool *ret);

#endif /* UTILS_H_ */
