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

int dynCommon_parseName(FILE *stream, char **result);
int dynCommon_parseNameAlsoAccept(FILE *stream, const char *acceptedChars, char **result);
int dynCommon_parseNameValue(FILE *stream, char **name, char **value);
int dynCommon_eatChar(FILE *stream, int c);

void dynCommon_clearNamValHead(struct namvals_head *head);

#ifdef __cplusplus
}
#endif

#endif 
