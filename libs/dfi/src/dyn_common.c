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

#include "dyn_common.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

#if NO_MEMSTREAM_AVAILABLE
#include "open_memstream.h"
#include "fmemopen.h"
#endif

static const int OK = 0;
static const int ERROR = 1;

DFI_SETUP_LOG(dynCommon)

static bool dynCommon_charIn(int c, const char *acceptedChars);

int dynCommon_parseName(FILE *stream, char **result) {
    return dynCommon_parseNameAlsoAccept(stream, NULL, result);
}

int dynCommon_parseNameAlsoAccept(FILE *stream, const char *acceptedChars, char **result) {
    int status = OK;

    char *buf = NULL;
    size_t size = 0;
    int strLen = 0;
    FILE *name = open_memstream(&buf, &size);

    if (name != NULL) { 
        int c = getc(stream);
        while (isalnum(c) || c == '_' || dynCommon_charIn(c, acceptedChars)) {
            fputc(c, name); 
            c = getc(stream);
            strLen += 1;
        }
        fflush(name);
        fclose(name);
        ungetc(c, stream);
    } else {
        status = ERROR;
        LOG_ERROR("Error creating mem stream for name. %s", strerror(errno));
    }

    if (status == OK) {
        if (strLen == 0) {
            status = ERROR;
            LOG_ERROR("Parsed empty name");
        }
    }

    if (status == OK) {
       LOG_DEBUG("Parsed name '%s'", buf);
       *result = buf;
    } else if (buf != NULL) {
        free(buf);
    }

    return status;
}

int dynCommon_parseNameValue(FILE *stream, char **outName, char **outValue) {
    int status;
    char *name = NULL;
    char *value = NULL;

    status = dynCommon_parseName(stream, &name);
    if (status == OK) {
        status = dynCommon_eatChar(stream, '=');
    }
    if (status == OK) {
        const char *valueAcceptedChars = ".<>{}[]?;:~!@#$%^&*()_+-=,./\\'\"";

        status = dynCommon_parseNameAlsoAccept(stream, valueAcceptedChars, &value); //NOTE use different more lenient function e.g. only stop at '\n' ?
    }

    if (status == OK) {
        *outName = name;
        *outValue = value;
    } else {
        if (name != NULL) {
            free(name);
        }
        if (value != NULL) {
            free(value);
        }
    }
    return status;
}

int dynCommon_eatChar(FILE *stream, int expected) {
    int status = OK;
    long loc = ftell(stream);
    int c = fgetc(stream);
    if (c != expected) {
        status = ERROR;
        LOG_ERROR("Error parsing, expected token '%c' got '%c' at position %li", expected, c, loc);
    }
    return status;
}

static bool dynCommon_charIn(int c, const char *acceptedChars) {
    bool status = false;
    if (acceptedChars != NULL) {
        int i;
        for (i = 0; acceptedChars[i] != '\0'; i += 1) {
            if (c == acceptedChars[i]) {
                status = true;
                break;
            }
        }
    }

    return status;
}

void dynCommon_clearNamValHead(struct namvals_head *head) {
    struct namval_entry *entry = TAILQ_FIRST(head);
    while (entry != NULL) {
        struct namval_entry *tmp = entry;

        if (entry->name != NULL) {
            free(entry->name);
        }
        if (entry->value != NULL) {
            free(entry->value);
        }
        entry = TAILQ_NEXT(entry, entries);
        free(tmp);
    }
}
