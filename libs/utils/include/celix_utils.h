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
#define CELIX_US_IN_SEC (1000000)
#define CELIX_NS_IN_SEC ((CELIX_US_IN_SEC)*1000)


/**
 * Creates a copy of a provided string.
 * The strdup is limited to the CELIX_UTILS_MAX_STRLEN and uses strndup to achieve this.
 * @return a copy of the string (including null terminator).
 */
char* celix_utils_strdup(const char *str);

/**
 * @brief Creates a hash from a string
 * @param string
 * @return hash
 */
unsigned int celix_utils_stringHash(const char* string);

/**
 * @brief Compares two strings and returns true if the strings are equal.
 */
bool celix_utils_stringEquals(const char* a, const char* b);

/**
 * @brief Returns a trimmed string.
 *
 * The trim will remove eny leading and trailing whitespaces (' ', '\t', etc based on `isspace`)/
 * Caller is owner of the returned string.
 */
char* celix_utils_trim(const char* string);

/**
 * @brief Check if a string is NULL or empty "".
 */
bool celix_utils_isStringNullOrEmpty(const char* s);


/**
 * @brief Extract a local name and namespace from a fully qualified name using the provided namespace separator.
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
 * @brief Returns the diff in seconds between tBegin and tEnd.
 * @param tBegin The begin time.
 * @param tEnd   The end time.
 * @return       Diff in seconds.
 */
double celix_difftime(const struct timespec *tBegin, const struct timespec *tEnd);

/**
 * @brief Returns the current time as struct timespec
 * @param clockId The clock to use (see time.h)
 */
struct timespec celix_gettime(clockid_t clockId);

/**
 * @brief Returns the elapsed time - in seconds - relative to the startTime
 * using the clock for the provided clockid.
 */
double celix_elapsedtime(clockid_t clockId, struct timespec startTime);

/**
 * @brief Creates a hash from a string
 */
unsigned int celix_utils_stringHash(const char* string);

/**
 * @brief Compares services using the service id and ranking.
 *
 * If the service id are the same -> compare return 0.
 *
 * If the service ranking of A is higher -> return -1; (smaller -> A is sorted before B)
 *
 * If the service rankings are the same, but the svcId of A is smaller (older service) -> return -1:
 * (smaller A is sorted before B)
 *
 * And vica versa.
 */
int celix_utils_compareServiceIdsAndRanking(long svcIdA, long svcRankA, long svcIdB, long svcRankB);


#ifdef __cplusplus
}
#endif
#endif /* CELIX_UTILS_H_ */
