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

#ifndef CELIX_UTILS_H_
#define CELIX_UTILS_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <time.h>
#include <stdbool.h>

#define CELIX_UTILS_MAX_STRLEN      1024*1024*1024

/**
 * Creates a copy of a provided string.
 * The strdup is limited to the CELIX_UTILS_MAX_STRLEN and uses strndup to achieve this.
 * @return a copy of the string (including null terminator).
 */
char* celix_utils_strdup(const char *str);

/**
 * Creates a hash from a string
 * @param string
 * @return hash
 */
unsigned int celix_utils_stringHash(const char* string);

/**
 * Compares two strings and returns true if the strings are equal.
 */
bool celix_utils_stringEquals(const char* a, const char* b);



/**
 * Extract a local name and namespace from a fully qualified name using the provided namespace separator.
 * so fully qualified name = celix::extra::lb, namespace separator = "::" -> local name = lb, namespace = celix::extra
 *
 * Note that if no namespace is present the output for namespace will be NULL.
 *
 * @param fullyQualifiedName    The fully qualified name to split
 * @param namespaceSeparator    The namespace separator
 * @param outLocalName          A output argument for the local name part. Caller is owner of the data.
 * @param outNamespace          A output argument for the (optional) namespace part. Caller is owner of the data.
 */
void celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(const char *fullyQualifiedName, const char *namespaceSeparator, char **outLocalName, char **outNamespace);

/**
 * Returns the diff in seconds between tBegin and tEnd.
 * @param tBegin The begin time.
 * @param tEnd   The end time.
 * @return       Diff in seconds.
 */
double celix_difftime(const struct timespec *tBegin, const struct timespec *tEnd);

/**
 * Returns the current time as struct timespec
 * @param clockId The clock to use (see time.h)
 */
struct timespec celix_gettime(clockid_t clockId);

/**
 * Returns the elapsed time - in seconds - relative to the startTime
 * using the clock for the provided clockid.
 */
double celix_elapsedtime(clockid_t clockId, struct timespec startTime);

/**
 * Creates a hash from a string
 */
unsigned int celix_utils_stringHash(const char* string);


#ifdef __cplusplus
}
#endif
#endif /* CELIX_UTILS_H_ */
