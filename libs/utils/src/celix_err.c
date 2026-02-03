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

#include "celix_err_private.h"
#include "celix_err.h"

#include <stdarg.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <uv.h>


#ifdef CELIX_ERR_DISABLE_CONSTRUCTORS
#define CELIX_ERR_CONSTRUCTOR
#define CELIX_ERR_DESTRUCTOR
#else
#define CELIX_ERR_CONSTRUCTOR __attribute__((constructor))
#define CELIX_ERR_DESTRUCTOR __attribute__((destructor))
#endif

typedef struct celix_err {
    char buffer[CELIX_ERR_BUFFER_SIZE];
    size_t pos;
} celix_err_t;


uv_key_t celix_err_tssKey;
bool celix_err_tssKeyInitialized = false;

static celix_err_t* celix_err_getRawTssErr() {
    if (!celix_err_tssKeyInitialized) {
        return NULL;
    }
    return uv_key_get(&celix_err_tssKey);
}


celix_err_t* celix_err_getTssErr() {
    if (!celix_err_tssKeyInitialized) {
        fprintf(stderr, "celix_err_tssKey is not initialized\n");
        return NULL;
    }

    celix_err_t* err = uv_key_get(&celix_err_tssKey);
    if (err) {
        return err;
    }

    err = malloc(sizeof(*err));
    if (err) {
        err->pos = 0; //no entry
        uv_key_set(&celix_err_tssKey, err);
    } else {
        fprintf(stderr, "Failed to allocate memory for celix_err\n");
    }
    return err;
}

CELIX_ERR_CONSTRUCTOR void celix_err_initThreadSpecificStorageKey() {
    if (celix_err_tssKeyInitialized) {
        return;
    }
    int rc = uv_key_create(&celix_err_tssKey);
    if (rc == 0) {
        celix_err_tssKeyInitialized = true;
    } else {
        fprintf(stderr,"Failed to create thread specific storage key for celix_err\n");
    }
}

CELIX_ERR_DESTRUCTOR void celix_err_deinitThreadSpecificStorageKey() {
    if (!celix_err_tssKeyInitialized) {
        return; //note: error is logged during init
    }
    uv_key_delete(&celix_err_tssKey);
    celix_err_tssKeyInitialized = false;
}

const char* celix_err_popLastError() {
    const char* result = NULL;
    celix_err_t* err = celix_err_getRawTssErr();
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
    celix_err_t* err = celix_err_getRawTssErr();
    for (int i = 0; err && i < err->pos; ++i) {
        if (err->buffer[i] == '\0') {
            result += 1;
        }
    }
    return result;
}

void celix_err_resetErrors() {
    celix_err_t* err = celix_err_getRawTssErr();
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

int celix_err_dump(char* buf, size_t size, const char* prefix, const char* postfix) {
    int ret;
    int bytes = 0;
    const char* pre = prefix == NULL ? "" : prefix;
    const char* post = postfix == NULL ? "\n" : postfix;
    for (const char *errMsg = celix_err_popLastError(); errMsg != NULL && bytes < size; errMsg = celix_err_popLastError()) {
        ret = snprintf(buf + bytes, size - bytes, "%s%s%s", pre, errMsg, post);
        if (ret > 0) {
            bytes += ret;
        }
    }
    return bytes;
}
