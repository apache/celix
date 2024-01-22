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

#ifndef _DYN_COMMON_H_
#define _DYN_COMMON_H_

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <sys/queue.h>

#include "dfi_log_util.h"
#include "celix_dfi_export.h"

#ifdef __cplusplus
extern "C" {
#endif

//logging
DFI_SETUP_LOG_HEADER(dynCommon) ;

TAILQ_HEAD(namvals_head, namval_entry);

struct namval_entry {
    char *name;
    char *value;
    TAILQ_ENTRY(namval_entry) entries;
};

/**
 * @brief Parse the name of dynamic type from the given stream. The name is only allowed to contain [a-zA-Z0-9_].
 *
 * The caller is the owner of the dynamic type name and use `free` deallocate the memory.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] stream The input stream.
 * @param[out] result The name of the Dynamic Type.
 * @return 0 if successful, otherwise 1.
 * @alsoseee dynCommon_parseNameAlsoAccept
 */
CELIX_DFI_EXPORT int dynCommon_parseName(FILE *stream, char **result);

/**
 * @brief Parse the name of dynamic type from the given stream.
 *
 * The caller is the owner of the dynamic type name and use `free` deallocate the memory.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] stream The input stream.
 * @param[in] acceptedChars The extra accepted characters for the name. If NULL, only [a-zA-Z0-9_] are accepted.
 * @param[out] result The name of the Dynamic Type.
 * @return 0 if successful, otherwise 1.
 * @alsoseee dynCommon_parseName
 */
CELIX_DFI_EXPORT int dynCommon_parseNameAlsoAccept(FILE *stream, const char *acceptedChars, char **result);

/**
 * @brief Parse the name and value of a name-value pair from the given stream. The name is only allowed to contain [a-zA-Z0-9_].
 *
 * The caller is the owner of the name and value and use `free` deallocate the memory.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] stream The input stream.
 * @param[out] name The name of the name-value pair.
 * @param[out] value The value of the name-value pair.
 * @return 0 if successful, otherwise 1.
 */
CELIX_DFI_EXPORT int dynCommon_parseNameValue(FILE *stream, char **name, char **value);

/**
 * @brief Eat the given character from the given stream.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] stream The input stream.
 * @param[in] c The character to be eaten.
 * @return 0 if successful, otherwise 1.
 */
CELIX_DFI_EXPORT int dynCommon_eatChar(FILE *stream, int c);

/**
 * @brief Clear the given name-value pairs.
 *
 * @param[in] head The name-value pairs to be cleared.
 */
CELIX_DFI_EXPORT void dynCommon_clearNamValHead(struct namvals_head *head);

#ifdef __cplusplus
}
#endif

#endif 