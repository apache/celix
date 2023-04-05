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

#include <stdlib.h>
#include <threads.h>
#include <stdarg.h>
#include <stdio.h>

#include "celix_array_list.h"
#include "celix_rcm_err_private.h"
#include "celix_rcm_err.h"
#include "celix_utils.h"

static tss_t celix_rcm_tssErrorsKey;

static void celix_rcm_destroyTssErrors(void* data) {
    celix_array_list_t* errors = data;
    if (errors != NULL) {
        for (int i = 0; i < celix_arrayList_size(errors); ++i) {
            char* msg = celix_arrayList_get(errors, i);
            free(msg);
        }
    }
    celix_arrayList_destroy(errors);
}

__attribute__((constructor)) static void celix_rcm_initThreadSpecificStorageKey() {
    tss_create(&celix_rcm_tssErrorsKey, celix_rcm_destroyTssErrors);
}

__attribute__((destructor)) static void celix_rcm_deinitThreadSpecificStorageKey() {
    tss_delete(celix_rcm_tssErrorsKey);
}

char* celix_rcmErr_popLastError() {
    char* result = NULL;
    celix_array_list_t* errors = tss_get(celix_rcm_tssErrorsKey);
    if (errors != NULL && celix_arrayList_size(errors) > 0) {
        result = celix_arrayList_get(errors, 0);
        celix_arrayList_removeAt(errors, 0);
    }
    return result;
}

int celix_rcmErr_getErrorCount() {
    int result = 0;
    celix_array_list_t* errors = tss_get(celix_rcm_tssErrorsKey);
    if (errors != NULL) {
        result = celix_arrayList_size(errors);
    }
    return result;
}

void celix_rcmErr_resetErrors() {
    celix_array_list_t* errors = tss_get(celix_rcm_tssErrorsKey);
    if (errors != NULL) {
        for (int i = 0; i < celix_arrayList_size(errors); ++i) {
            char* msg = celix_arrayList_get(errors, i);
            free(msg);
        }
        celix_arrayList_clear(errors);
    }
}

static void celix_rcm_pushMsg(char* msg) {
    celix_array_list_t* errors = tss_get(celix_rcm_tssErrorsKey);
    if (errors == NULL) {
        errors = celix_arrayList_create();
        tss_set(celix_rcm_tssErrorsKey, errors);
    }
    if (errors != NULL) {
        celix_arrayList_add(errors, msg);
    }
}

void celix_rcmErr_push(const char* msg) {
    celix_rcm_pushMsg(celix_utils_strdup(msg));
}

void celix_rcmErr_pushf(char* format, ...) {
    va_list args;
    va_start(args, format);
    char* msg = NULL;
    int rc = vasprintf(&msg, format, args);
    if (rc >= 0) {
        celix_rcm_pushMsg(msg);
    }
    va_end(args);
}
