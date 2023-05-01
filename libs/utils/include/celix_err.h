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
 * The error message must be freed by the caller.
 * @returnval NULL if no error message is available.
 */
CELIX_UTILS_EXPORT
char* celix_err_popLastError(); //TODO update to return const char* and remove free

/**
 * @brief Push an formatted error message to the thread specific storage rcm errors.
 */
CELIX_UTILS_EXPORT
void celix_err_pushf(const char* format, ...) __attribute__((format(printf, 1, 2)));

/**
 * @brief Push an error message to the thread specific storage rcm errors.
 */
CELIX_UTILS_EXPORT
void celix_err_push(const char* msg);

/**
 * @brief Returns the number of error messages from the current thread.
 */
CELIX_UTILS_EXPORT
int celix_err_getErrorCount();

/**
 * @brief Resets the error message for the current thread.
 */
CELIX_UTILS_EXPORT
void celix_err_resetErrors();

#ifdef __cplusplus
}
#endif

#endif //CELIX_CELIX_ERR_H
