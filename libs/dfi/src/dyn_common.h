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

#ifdef __cplusplus
extern "C" {
#endif

TAILQ_HEAD(namvals_head, namval_entry);

struct namval_entry {
    char* name;
    char* value;
    TAILQ_ENTRY(namval_entry) entries;
};

/**
 * @brief Parse the name of dynamic type from the given stream. The name is only allowed to contain [a-zA-Z0-9_].
 *
 * The caller is the owner of the dynamic type name and use `free` to deallocate the memory.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] stream The input stream.
 * @param[out] result The name of the Dynamic Type.
 * @return 0 if successful, otherwise 1.
 * @alsoseee dynCommon_parseNameAlsoAccept
 */
int dynCommon_parseName(FILE* stream, char** result);

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
int dynCommon_parseNameAlsoAccept(FILE* stream, const char* acceptedChars, char** result);

/**
 * @brief Parses a section of name-value pairs from the given stream. The name is only allowed to contain [a-zA-Z0-9_].
 *
 * This function reads from the provided stream until it encounters a new section or EOF.
 * Each name-value pair is added to the provided namvals_head structure.
 *
 * The caller is responsible for managing the memory of the namvals_head structure.
 * Use `dynCommon_clearNamValHead` to clear the name-value pairs when they are no longer needed.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] stream The input stream.
 * @param[out] head The namvals_head structure where the parsed name-value pairs will be stored.
 * @return 0 if successful, otherwise 1.
 */
int dynCommon_parseNameValueSection(FILE* stream, struct namvals_head* head);

/**
 * @brief Eat the given character from the given stream.
 *
 * In case of an error, an error message is added to celix_err.
 *
 * @param[in] stream The input stream.
 * @param[in] c The character to be eaten.
 * @return 0 if successful, otherwise 1.
 */
int dynCommon_eatChar(FILE* stream, int c);

/**
 * @brief Clear the given name-value pairs.
 *
 * @param[in] head The name-value pairs to be cleared.
 */
void dynCommon_clearNamValHead(struct namvals_head* head);

int dynCommon_getEntryForHead(const struct namvals_head* head, const char* name, const char** out);

#ifdef __cplusplus
}
#endif

#endif 
