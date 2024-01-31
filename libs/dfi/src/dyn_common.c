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
#include "celix_stdio_cleanup.h"
#include "celix_stdlib_cleanup.h"

#include <stdlib.h>
#include <stdio.h>
#include <ctype.h>
#include <stdbool.h>

static const int OK = 0;
static const int ERROR = 1;


int dynCommon_parseName(FILE* stream, char** result) {
    return dynCommon_parseNameAlsoAccept(stream, NULL, result);
}

int dynCommon_parseNameAlsoAccept(FILE* stream, const char* acceptedChars, char** result) {
    celix_autofree char* buf = NULL;
    size_t size = 0;
    celix_autoptr(FILE) name = open_memstream(&buf, &size);
    if (name == NULL) {
        celix_err_pushf("Error creating mem stream for name. %s", strerror(errno));
        return ERROR;
    }

    int c = getc(stream);
    while (c != EOF && (isalnum(c) || c == '_' || (acceptedChars != NULL && strchr(acceptedChars, c) != NULL))) {
        if(fputc(c, name) == EOF) {
            celix_err_push("Error writing to mem stream for name.");
            return ERROR;
        }
        c = getc(stream);
    }
    if (c != EOF) {
        ungetc(c, stream);
    }
    if(fclose(celix_steal_ptr(name)) != 0) {
        celix_err_pushf("Error closing mem stream for name. %s", strerror(errno));
        return ERROR;
    }

    if (size == 0) {
        celix_err_push("Parsed empty name");
        return ERROR;
    }

    *result = celix_steal_ptr(buf);

    return OK;
}

static int dynCommon_parseNameValue(FILE* stream, char** outName, char** outValue) {
    int status;
    celix_autofree char* name = NULL;
    celix_autofree char* value = NULL;
    do {
        if ((status = dynCommon_parseName(stream, &name)) != OK) {
            break;
        }
        if ((status = dynCommon_eatChar(stream, '=')) != OK) {
            break;
        }
        const char *valueAcceptedChars = ".<>{}[]?;:~!@#$%^&*()_+-=,./\\'\"";
        //NOTE use different more lenient function e.g. only stop at '\n' ?
        if ((status = dynCommon_parseNameAlsoAccept(stream, valueAcceptedChars, &value)) != OK) {
            break;
        }
        *outName = celix_steal_ptr(name);
        *outValue = celix_steal_ptr(value);
    } while(false);
    return status;
}

int dynCommon_eatChar(FILE* stream, int expected) {
    int status = OK;
    int c = fgetc(stream);
    if (c != expected) {
        status = ERROR;
        celix_err_pushf("Error parsing, expected token '%c' got '%c' at position %li", expected, c, ftell(stream));
    }
    return status;
}

void dynCommon_clearNamValHead(struct namvals_head* head) {
    struct namval_entry* entry = TAILQ_FIRST(head);
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

int dynCommon_parseNameValueSection(FILE* stream, struct namvals_head* head) {
    int status = OK;

    int peek = fgetc(stream);
    while (peek != ':' && peek != EOF) {
        ungetc(peek, stream);

        celix_autofree char* name = NULL;
        celix_autofree char* value = NULL;
        if ((status = dynCommon_parseNameValue(stream, &name, &value)) != OK) {
            return status;
        }

        if ((status = dynCommon_eatChar(stream, '\n')) != OK) {
            return status;
        }

        struct namval_entry* entry = NULL;
        entry = calloc(1, sizeof(*entry));
        if (entry == NULL) {
            celix_err_pushf("Error allocating memory for namval entry");
            return ERROR;
        }

        entry->name = celix_steal_ptr(name);
        entry->value = celix_steal_ptr(value);
        TAILQ_INSERT_TAIL(head, entry, entries);

        peek = fgetc(stream);
    }
    if (peek != EOF) {
        ungetc(peek, stream);
    }

    return OK;
}

int dynCommon_getEntryForHead(const struct namvals_head* head, const char* name, const char** out) {
    int status = OK;
    char* value = NULL;
    struct namval_entry* entry = NULL;
    TAILQ_FOREACH(entry, head, entries) {
        if (strcmp(name, entry->name) == 0) {
            value = entry->value;
            break;
        }
    }
    if (value != NULL) {
        *out = value;
    } else {
        status = ERROR;
        celix_err_pushf("Cannot find '%s' in list", name);
    }
    return status;
}
