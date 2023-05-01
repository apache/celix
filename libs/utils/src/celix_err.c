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

#include <stdlib.h>
#include <stdarg.h>
#include <stdio.h>
#include <pthread.h>

#include "celix_array_list.h"
#include "celix_utils.h"

pthread_key_t celix_rcm_tssErrorsKey; //TODO replace with celix_tss_key and celix_tss_* functions

static void celix_err_destroyTssErrors(void* data) {
    //TODO replace array list with some thread specific storage char buffer
    celix_array_list_t* errors = data;
    if (errors != NULL) {
        for (int i = 0; i < celix_arrayList_size(errors); ++i) {
            char* msg = celix_arrayList_get(errors, i);
            free(msg);
        }
    }
    celix_arrayList_destroy(errors);
}

__attribute__((constructor)) void celix_err_initThreadSpecificStorageKey() {
    int rc = pthread_key_create(&celix_rcm_tssErrorsKey, celix_err_destroyTssErrors);
    if (rc != 0) {
        fprintf(stderr,"Failed to create thread specific storage key for celix_err\n");
    }
}

__attribute__((destructor)) void celix_err_deinitThreadSpecificStorageKey() {
    int rc = pthread_key_delete(celix_rcm_tssErrorsKey);
    if (rc != 0) {
        fprintf(stderr,"Failed to delete thread specific storage key for celix_err\n");
    }
}

char* celix_err_popLastError() {
    char* result = NULL;
    celix_array_list_t* errors = pthread_getspecific(celix_rcm_tssErrorsKey);
    if (errors != NULL && celix_arrayList_size(errors) > 0) {
        int last = celix_arrayList_size(errors) - 1;
        result = celix_arrayList_get(errors, last);
        celix_arrayList_removeAt(errors, last);
    }
    return result;
}

int celix_err_getErrorCount() {
    int result = 0;
    celix_array_list_t* errors = pthread_getspecific(celix_rcm_tssErrorsKey);
    if (errors != NULL) {
        result = celix_arrayList_size(errors);
    }
    return result;
}

void celix_err_resetErrors() {
    celix_array_list_t* errors = pthread_getspecific(celix_rcm_tssErrorsKey);
    celix_err_destroyTssErrors(errors);
    pthread_setspecific(celix_rcm_tssErrorsKey, NULL);
}

static void celix_err_pushMsg(char* msg) {
    celix_array_list_t* errors = pthread_getspecific(celix_rcm_tssErrorsKey);
    if (errors == NULL) {
        errors = celix_arrayList_create();
        if (errors == NULL) {
            fprintf(stderr, "Failed to create error list for Apache Celix rcm lib\n");
            free(msg);
            return;
        }
        pthread_setspecific(celix_rcm_tssErrorsKey, errors);
    }
    if (errors != NULL) {
        celix_status_t rc = celix_arrayList_add(errors, msg);
        if (rc != CELIX_SUCCESS) {
            fprintf(stderr, "Failed to add error to error list for Apache Celix rcm lib\n");
            free(msg);
        }
    }
}

void celix_err_push(const char* msg) {
    char* msgCpy = celix_utils_strdup(msg);
    if (msgCpy == NULL) {
        fprintf(stderr, "Failed to copy error message for Apache Celix rcm lib\n");
        return;
    }
    celix_err_pushMsg(msgCpy);
}

void celix_err_pushf(const char* format, ...) {
    va_list args;
    va_start(args, format);
    char* msg = NULL;
    int rc = vasprintf(&msg, format, args);
    if (rc >= 0) {
        celix_err_pushMsg(msg);
    } else {
        fprintf(stderr, "Failed to copy error message for Apache Celix rcm lib\n");
    }
    va_end(args);
}
