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
#include "celix_err.h"
#include "celix_stdlib_cleanup.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

static const int OK = 0;
static const int ERROR = 1;


int dynCommon_parseName(FILE *stream, char **result) {
    return dynCommon_parseNameAlsoAccept(stream, NULL, result);
}

int dynCommon_parseNameAlsoAccept(FILE *stream, const char *acceptedChars, char **result) {
    int status = OK;

    char *buf = NULL;
    size_t size = 0;
    int strLen = 0;
    FILE *name = open_memstream(&buf, &size);
    if (name == NULL) {
        celix_err_pushf("Error creating mem stream for name. %s", strerror(errno));
        return ERROR;
    }

    int c = getc(stream);
    while (isalnum(c) || c == '_' || (acceptedChars != NULL && strchr(acceptedChars, c) != NULL)) {
        fputc(c, name);
        c = getc(stream);
        strLen += 1;
    }
    fflush(name);
    fclose(name);
    ungetc(c, stream);

    if (status == OK) {
        if (strLen == 0) {
            status = ERROR;
            celix_err_pushf("Parsed empty name");
        }
    }

    if (status == OK) {
       *result = buf;
    } else if (buf != NULL) {
        free(buf);
    }

    return status;
}

int dynCommon_parseNameValue(FILE *stream, char **outName, char **outValue) {
    int status;
    celix_autofree char *name = NULL;
    celix_autofree char *value = NULL;

    status = dynCommon_parseName(stream, &name);
    if (status == OK) {
        status = dynCommon_eatChar(stream, '=');
    }
    if (status == OK) {
        const char *valueAcceptedChars = ".<>{}[]?;:~!@#$%^&*()_+-=,./\\'\"";

        status = dynCommon_parseNameAlsoAccept(stream, valueAcceptedChars, &value); //NOTE use different more lenient function e.g. only stop at '\n' ?
    }

    if (status == OK) {
        *outName = celix_steal_ptr(name);
        *outValue = celix_steal_ptr(value);
    }
    return status;
}

int dynCommon_eatChar(FILE *stream, int expected) {
    int status = OK;
    int c = fgetc(stream);
    if (c != expected) {
        status = ERROR;
        celix_err_pushf("Error parsing, expected token '%c' got '%c' at position %li", expected, c, ftell(stream));
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

int dynCommon_parseNameValueSection(FILE *stream, struct namvals_head *head) {
    int status = OK;

    int peek = fgetc(stream);
    while (peek != ':' && peek != EOF) {
        ungetc(peek, stream);

        celix_autofree char *name = NULL;
        celix_autofree char *value = NULL;
        status = dynCommon_parseNameValue(stream, &name, &value);

        if (status == OK) {
            status = dynCommon_eatChar(stream, '\n');
        }

        struct namval_entry *entry = NULL;
        if (status == OK) {
            entry = calloc(1, sizeof(*entry));
            if (entry != NULL) {
                entry->name = celix_steal_ptr(name);
                entry->value = celix_steal_ptr(value);
                TAILQ_INSERT_TAIL(head, entry, entries);
            } else {
                status = ERROR;
                celix_err_pushf("Error allocating memory for namval entry");
            }
        }

        if (status != OK) {
            break;
        }
        peek = fgetc(stream);
    }
    ungetc(peek, stream);

    return status;
}
