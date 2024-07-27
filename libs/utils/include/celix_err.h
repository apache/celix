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

#ifndef CELIX_CELIX_ERR_H
#define CELIX_CELIX_ERR_H

#include <stdbool.h>
#include <stdio.h>

#include "celix_err_constants.h"
#include "celix_utils_export.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
* @file celix_err.h
* @brief The celix error handling functions.
* @thread_safety Thread safe.
*/

/**
 * @brief Returns the last error message from the current thread.
 *
 * The returned error message is stored on a local thread storage and is valid until a new error message is pushed.
 *
 * @return The last error message from the current thread.
 *
 * @retval NULL if no error message is available.
 */
CELIX_UTILS_EXPORT const char* celix_err_popLastError();

/**
 * @brief Push an formatted error message to the thread specific storage rcm errors.
 *
 * @param format The format string for the error message.
 * @param ... The arguments for the format string.
 */
CELIX_UTILS_EXPORT void celix_err_pushf(const char* format, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Push an error message to the thread specific storage rcm errors.
 *
 * @param msg The error message to push.
 */
CELIX_UTILS_EXPORT void celix_err_push(const char* msg);

/**
 * @brief Returns the number of error messages from the current thread.
 *
 * @return The number of error messages for the current thread.
 */
CELIX_UTILS_EXPORT int celix_err_getErrorCount();

/**
 * @brief Resets the error message for the current thread.
 */
CELIX_UTILS_EXPORT void celix_err_resetErrors();

/**
 * @brief Prints error messages from the current thread to the provided stream.
 *
 * @param[in] stream The stream to print the error messages to.
 * @param[in] prefix The prefix to print before each error message. If NULL no prefix is printed.
 * @param[in] postfix The postfix to print after each error message. If NULL, a '\n' postfix is printed.
 */
CELIX_UTILS_EXPORT void celix_err_printErrors(FILE* stream, const char* prefix, const char* postfix);

/**
 * @brief Dump error messages from the current thread to the provided buffer.
 * @param[in,out] buf The buffer to dump the error messages to.
 * @param[in] size The size of the buffer.
 * @param[in] prefix The prefix to print before each error message. If NULL no prefix is printed.
 * @param[in] postfix The postfix to print after each error message. If NULL, a '\n' postfix is printed.
 * @return the number of characters written to the buffer (excluding the terminating null character).
 * A return value of size or more means that the output was truncated. See also snprintf.
 */
CELIX_UTILS_EXPORT int celix_err_dump(char* buf, size_t size, const char* prefix, const char* postfix);

/*!
 * Helper macro that returns with ENOMEM if the status is ENOMEM and logs an "<func>: Out of memory" error to celix_err.
 */
#define CELIX_RETURN_IF_ENOMEM(status)                                                                                 \
    do {                                                                                                               \
        if ((status) == ENOMEM) {                                                                                      \
            celix_err_pushf("%s: Out of memory", __func__);                                                            \
            return status;                                                                                             \
        }                                                                                                              \
    } while (0)

/*!
 * Helper macro that returns with ENOMEM if the arg is NULL and logs an "<func>: Out of memory" error to celix_err.
 */
#define CELIX_RETURN_IF_NULL(arg)                                                                                      \
    do {                                                                                                               \
        if ((arg) == NULL) {                                                                                           \
            celix_err_pushf("%s: Out of memory", __func__);                                                            \
            return status;                                                                                             \
        }                                                                                                              \
    } while (0)

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_ERR_H
