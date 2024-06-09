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
#include <stdarg.h>
#include <stdbool.h>

#include "celix_utils_export.h"
#include "celix_compiler.h"
#include "celix_cleanup.h"

#define CELIX_US_IN_SEC (1000000)
#define CELIX_NS_IN_SEC ((CELIX_US_IN_SEC)*1000)

/**
 * Creates a copy of a provided string.
 * The strdup is limited to the CELIX_UTILS_MAX_STRLEN and uses strndup to achieve this.
 * @return a copy of the string (including null terminator).
 */
CELIX_UTILS_EXPORT char* celix_utils_strdup(const char *str);

/**
 * Returns the length of the provided string with a max of CELIX_UTILS_MAX_STRLEN.
 * @param str The string to get the length from.
 * @return The length of the string or 0 if the string is NULL.
 */
CELIX_UTILS_EXPORT size_t celix_utils_strlen(const char *str);

/**
 * @brief Creates a hash from a string
 * @param string
 * @return hash
 */
CELIX_UTILS_EXPORT unsigned int celix_utils_stringHash(const char* string);

/**
 * The proposed buffer size to use for celix_utils_writeOrCreateString with a buffer on the stcck.
 */
#define CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE 512

/**`
 * @brief Format a string to the provided buffer or a newly allocated buffer if the provided buffer is to small.
 * @param[in,out] buffer The buffer to write the formatted string to.
 * @param[in] bufferSize The size of the buffer.
 * @param[in] format The format string.
 * @param[in] ... The arguments for the format string.
 * @return The formatted string in the provided buffer or a newly allocated buffer if the provided buffer is to small.
 * @retval NULL if a allocation was needed, but failed.
 */
CELIX_UTILS_EXPORT char* celix_utils_writeOrCreateString(char* buffer, size_t bufferSize, const char* format, ...)
    __attribute__((format(printf, 3, 4)));

/**
 * @brief Format a string to the provided buffer or a newly allocated buffer if the provided buffer is to small.
 * @param[in,out] buffer The buffer to write the formatted string to.
 * @param[in] bufferSize The size of the buffer.
 * @param[in] format The format string.
 * @param[in] formatArgs The arguments for the format string.
 * @return The formatted string in the provided buffer or a newly allocated buffer if the provided buffer is to small.
 * @retval NULL if a allocation was needed, but failed.
 */
CELIX_UTILS_EXPORT char* celix_utils_writeOrCreateVString(char* buffer, size_t bufferSize, const char* format, va_list formatArgs)
__attribute__((format(printf, 3, 0)));

/**
 * @brief Free the provided str if the str is not equal to the provided buffer.
 * @note This function is useful combined with celix_utils_writeOrCreateString.
 * @param buffer The buffer to compare the str to.
 * @param str The string to free if it is not equal to the buffer.
 */
CELIX_UTILS_EXPORT void celix_utils_freeStringIfNotEqual(const char* buffer, char* str);

/**
 * @brief Guard for a string created with celix_utils_writeOrCreateString, celix_utils_writeOrCreateVString.
 *
 * Can be used with celix_auto() to automatically and correctly free the string.
 * If the string is pointing to the buffer, the string should not be freed, otherwise the string should be freed.
 */
typedef struct celix_utils_string_guard {
    const char* buffer;
    char* string;
} celix_utils_string_guard_t;

/**
 * @brief Initialize a guard for a string created with celix_utils_writeOrCreateString or
 * celix_utils_writeOrCreateVString.
 *
 * De-initialize with celix_utils_stringGuard_deinit().
 *
 * No allocation is performed.
 * This is intended to be used with celix_auto().
 *
 * * Example:
 * ```
 * const char* possibleLongString = ...
 * char buffer[64];
 * char* str = celix_utils_writeOrCreateString(buffer, sizeof(buffer), "Hello %s", possibleLongString);
 * celix_auto(celix_utils_string_guard_t) strGuard = celix_utils_stringGuard_init(buffer, str);
 * ```
 * If the strGuard goes out of scope, the string will be freed correctly.
 *
 * @param buffer A (local) buffer which was potentially used to create the string.
 * @param string The string to guard.
 * @return An initialized string guard to be used with celix_auto().
 */
static CELIX_UNUSED inline celix_utils_string_guard_t celix_utils_stringGuard_init(const char* buffer, char* string) {
    celix_utils_string_guard_t guard;
    guard.buffer = buffer;
    guard.string = string;
    return guard;
}

/**
 * @brief De-initialize a string guard.
 *
 * This will free the string if it is not equal to the buffer.
 * This is intended to be used with celix_auto().
 *
 * @param guard The guard to de-initialize.
 */
static CELIX_UNUSED inline void celix_utils_stringGuard_deinit(celix_utils_string_guard_t* guard) {
    celix_utils_freeStringIfNotEqual(guard->buffer, guard->string);
}

CELIX_DEFINE_AUTO_CLEANUP_CLEAR_FUNC(celix_utils_string_guard_t, celix_utils_stringGuard_deinit)

/**
 * @brief Compares two strings and returns true if the strings are equal.
 */
CELIX_UTILS_EXPORT bool celix_utils_stringEquals(const char* a, const char* b);

/**
 * Check if the provided string contains a whitespace (spaces, tabs, etc).
 * The check is based on `isspace`.
 */
CELIX_UTILS_EXPORT bool celix_utils_containsWhitespace(const char* s);

/**
 * @brief Returns a trimmed string.
 *
 * This function will remove any leading and trailing whitespaces (' ', '\t', etc based on isspace) from the
 * input string.
 *
 * @param[in] string The input string to be trimmed.
 * @return A trimmed version of the input string. The caller is responsible for freeing the memory of this string.
 */
CELIX_UTILS_EXPORT char* celix_utils_trim(const char* string);
/**
 * @brief Trims the provided string in place.
 *
 * The trim will remove any leading and trailing whitespaces (' ', '\t', etc based on `isspace`)/
 * @param string the string to be trimmed.
 * @return string.
 */
CELIX_UTILS_EXPORT char* celix_utils_trimInPlace(char* string);

/**
 * @brief Check if a string is NULL or empty "".
 */
CELIX_UTILS_EXPORT bool celix_utils_isStringNullOrEmpty(const char* s);

/** @brief create a C identifier from the provided string by replacing each non-alphanumeric character with a
 * underscore.
 *
 * If the first character is a digit, a prefix underscore will also be added.
 * Will return NULL if the input is NULL or an empty string.
 *
 * @param string the input string to make a C identifier for.
 * @return new newly allocated string or NULL if the input was wrong. The caller is owner of the returned string.
 */
CELIX_UTILS_EXPORT char* celix_utils_makeCIdentifier(const char* s);


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
CELIX_UTILS_EXPORT void celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(const char *fullyQualifiedName, const char *namespaceSeparator, char **outLocalName, char **outNamespace);

/**
 * @brief Returns the diff in seconds between tBegin and tEnd.
 * @param tBegin The begin time.
 * @param tEnd   The end time.
 * @return       Diff in seconds.
 */
CELIX_UTILS_EXPORT double celix_difftime(const struct timespec *tBegin, const struct timespec *tEnd);

/**
 * @brief Returns the current time as struct timespec
 * @param clockId The clock to use (see time.h)
 */
CELIX_UTILS_EXPORT struct timespec celix_gettime(clockid_t clockId);

/**
 * @brief Returns the absolute time for the provided delay in seconds.
 * @param[in] time The time to add the delay to. Can be NULL, in which case the time is 0.
 * @param[in] delayInSeconds The delay in seconds.
 * @return A new time with the delay added.
 */
CELIX_UTILS_EXPORT struct timespec celix_delayedTimespec(const struct timespec* time, double delayInSeconds);

/**
 * @brief Returns the elapsed time - in seconds - relative to the startTime
 * using the clock for the provided clockid.
 */
CELIX_UTILS_EXPORT double celix_elapsedtime(clockid_t clockId, struct timespec startTime);

/**
 * @brief Compare two time arguments.
 * @param[in] a The first timespec.
 * @param[in] b The second timespec.
 * @return 0 if equal, -1 if a is before b and 1 if a is after b.
 */
CELIX_UTILS_EXPORT int celix_compareTime(const struct timespec* a, const struct timespec* b);

/**
 * @brief Creates a hash from a string
 */
CELIX_UTILS_EXPORT unsigned int celix_utils_stringHash(const char* string);

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
CELIX_UTILS_EXPORT int celix_utils_compareServiceIdsAndRanking(long svcIdA, long svcRankA, long svcIdB, long svcRankB);


#ifdef __cplusplus
}
#endif
#endif /* CELIX_UTILS_H_ */
