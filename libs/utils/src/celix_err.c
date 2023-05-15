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

#include "celix_err.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>

#ifndef CELIX_ERR_USE_THREAD_LOCAL
#include <stdlib.h>
#include "celix_threads.h"
#endif

typedef struct celix_err {
    char buffer[CELIX_ERR_BUFFER_SIZE];
    size_t pos;
} celix_err_t;


celix_tss_key_t celix_err_tssKey;
bool celix_err_tssKeyInitialized = false;

static void celix_err_destroyTssErr(void* data) {
    celix_err_t* err = data;
    if (err != NULL) {
        free(err);
    }
}

celix_err_t* celix_err_getTssErr() {
    if (!celix_err_tssKeyInitialized) {
        fprintf(stderr, "celix_err_tssKey is not initialized\n");
        return NULL;
    }

    celix_err_t* err = celix_tss_get(celix_err_tssKey);
    if (err) {
        return err;
    }

    err = malloc(sizeof(*err));
    if (err) {
        err->pos = 0; //no entry
        celix_status_t status = celix_tss_set(celix_err_tssKey, err);
        if (status != CELIX_SUCCESS) {
            fprintf(stderr, "Failed to set thread specific storage for celix_err\n");
            free(err);
            err = NULL;
        }
    } else {
        fprintf(stderr, "Failed to allocate memory for celix_err\n");
    }
    return err;
}

__attribute__((constructor)) void celix_err_initThreadSpecificStorageKey() {
    celix_status_t status = celix_tss_create(&celix_err_tssKey, celix_err_destroyTssErr);
    if (status == CELIX_SUCCESS) {
        celix_err_tssKeyInitialized = true;
    } else {
        fprintf(stderr,"Failed to create thread specific storage key for celix_err\n");
    }
}

__attribute__((destructor)) void celix_err_deinitThreadSpecificStorageKey() {
    if (!celix_err_tssKeyInitialized) {
        fprintf(stderr, "celix_err_tssKey is not initialized\n");
        return;
    }

    celix_status_t status = celix_tss_delete(celix_err_tssKey);
    if (status != CELIX_SUCCESS) {
        fprintf(stderr,"Failed to delete thread specific storage key for celix_err\n");
    }
}

const char* celix_err_popLastError() {
    const char* result = NULL;
    celix_err_t* err = celix_err_getTssErr();
    if (err && err->pos > 0) {
        //move back to start last error message
        err->pos -= 1; //move before \0 in last error message
        while (err->pos > 0 && err->buffer[err->pos-1] != '\0') {
            err->pos -= 1;
        }
        result = err->buffer + err->pos;
    }
    return result;
}

int celix_err_getErrorCount() {
    int result = 0;
    if (!celix_err_tssKeyInitialized) {
        fprintf(stderr, "celix_err_tssKey is not initialized\n");
        return 0;
    }
    celix_err_t* err = celix_tss_get(celix_err_tssKey);
    for (int i = 0; err && i < err->pos; ++i) {
        if (err->buffer[i] == '\0') {
            result += 1;
        }
    }
    return result;
}

void celix_err_resetErrors() {
    celix_err_t* err = celix_err_getTssErr();
    if (err) {
        err->pos = 0; //no entry
    }
}

void celix_err_push(const char* msg) {
    celix_err_t* err = celix_err_getTssErr();
    if (err) {
        size_t len = strnlen(msg, CELIX_ERR_BUFFER_SIZE);
        if (err->pos + len + 1 <= CELIX_ERR_BUFFER_SIZE) {
            strcpy(err->buffer + err->pos, msg);
            err->pos += len + 1;
        } else {
            fprintf(stderr, "Failed to add error message '%s' to celix_err\n", msg);
        }
    }
}

void celix_err_pushf(const char* format, ...) {
    va_list args;
    va_list argsCopy;
    va_start(args, format);
    va_copy(argsCopy, args);
    celix_err_t* err = celix_err_getTssErr();
    if (err) {
        size_t len = vsnprintf(err->buffer + err->pos, CELIX_ERR_BUFFER_SIZE - err->pos, format, args);
        if (err->pos + len + 1 <= CELIX_ERR_BUFFER_SIZE) {
            err->pos += len;
            err->buffer[err->pos++] = '\0';
        } else {
            fprintf(stderr, "Failed to add error message '");
            vfprintf(stderr, format, argsCopy);
            fprintf(stderr, "' to celix_err\n");
        }
    }
    va_end(argsCopy);
    va_end(args);
}

void celix_err_printErrors(FILE* stream, const char* prefix, const char* postfix) {
    const char* errMsg;
    const char* pre = prefix == NULL ? "" : prefix;
    const char* post = postfix == NULL ? "\n" : postfix;
    while (errMsg = celix_err_popLastError(), errMsg != NULL) {
        fprintf(stream, "%s%s%s", pre, errMsg, post);
    }
}
