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
#include <string.h>
#include <assert.h>

#include "utils.h"
#include "celix_utils.h"


#ifdef __APPLE__
#include "memstream/open_memstream.h"
#else
#include <stdio.h>
#include <assert.h>

#endif

unsigned int utils_stringHash(const void* strPtr) {
    return celix_utils_stringHash((const char*)strPtr);
}

int utils_stringEquals(const void* string, const void* toCompare) {
    return celix_utils_stringEquals((const char*)string, (const char*)toCompare);
}

unsigned int celix_utils_stringHash(const char* string) {
    if (string == NULL) {
        return 0;
    }
    unsigned int hc = 5381;
    char ch;
    while((ch = *string++) != '\0'){
        hc = (hc << 5) + hc + ch;
    }
    return hc;
}

bool celix_utils_stringEquals(const char* a, const char* b) {
    if (a == b) {
        return true;
    } else if (a == NULL || b == NULL) {
        return false;
    } else {
        return strncmp(a, b, 1024*124*10) == 0;
    }
}

bool celix_utils_isStringNullOrEmpty(const char* s) {
    return s == NULL || s[0] == '\0';
}

char * string_ndup(const char *s, size_t n) {
    size_t len = strlen(s);
    char *ret;

    if (len <= n) {
        return strdup(s);
    }

    ret = malloc(n + 1);
    strncpy(ret, s, n);
    ret[n] = '\0';
    return ret;
}

static char* celix_utilsTrimInternal(char *string) {
    if (string == NULL) {
        return NULL;
    }

    char* begin = string; //save begin to correctly free in the end.

    char *end;
    // Trim leading space
    while (isspace(*string)) {
        string++;
    }

    // Trim trailing space
    end = string + strlen(string) - 1;
    while(end > string && isspace(*end)) {
        *(end) = '\0';
        end--;
    }

    if (string != begin) {
        //beginning whitespaces -> move char in copy to to begin string
        //This to ensure free still works on the same pointer.
        char* nstring = begin;
        while(*string != '\0') {
            *(nstring++) = *(string++);
        }
        (*nstring) = '\0';
    }

    return begin;
}

char* celix_utils_trim(const char* string) {
    return celix_utilsTrimInternal(celix_utils_strdup(string));
}

char* utils_stringTrim(char* string) {
    return celix_utilsTrimInternal(string);
}

bool utils_isStringEmptyOrNull(const char * const str) {
    bool empty = true;
    if (str != NULL) {
        int i;
        for (i = 0; i < strnlen(str, 1024 * 1024); i += 1) {
            if (!isspace(str[i])) {
                empty = false;
                break;
            }
        }
    }

    return empty;
}

celix_status_t thread_equalsSelf(celix_thread_t thread, bool *equals) {
    celix_status_t status = CELIX_SUCCESS;

    celix_thread_t self = celixThread_self();
    if (status == CELIX_SUCCESS) {
        *equals = celixThread_equals(self, thread);
    }

    return status;
}

celix_status_t utils_isNumeric(const char *number, bool *ret) {
    celix_status_t status = CELIX_SUCCESS;
    *ret = true;
    while(*number) {
        if(!isdigit(*number) && *number != '.') {
            *ret = false;
            break;
        }
        number++;
    }
    return status;
}

int utils_compareServiceIdsAndRanking(long svcIdA, long svcRankA, long svcIdB, long svcRankB) {
    return celix_utils_compareServiceIdsAndRanking(svcIdA, svcRankA, svcIdB, svcRankB);
}

int celix_utils_compareServiceIdsAndRanking(long svcIdA, long svcRankA, long svcIdB, long svcRankB) {
    int result;

    if (svcIdA == svcIdB) {
        result = 0;
    } else if (svcRankA != svcRankB) {
        result = svcRankA < svcRankB ? 1 : -1;
    } else { //equal service rank, compare service ids
        result = svcIdA < svcIdB ? -1 : 1;
    }

    return result;
}

double celix_difftime(const struct timespec *tBegin, const struct timespec *tEnd) {
    struct timespec diff;
    if ((tEnd->tv_nsec - tBegin->tv_nsec) < 0) {
        diff.tv_sec = tEnd->tv_sec - tBegin->tv_sec - 1;
        diff.tv_nsec = tEnd->tv_nsec - tBegin->tv_nsec + CELIX_NS_IN_SEC;
    } else {
        diff.tv_sec = tEnd->tv_sec - tBegin->tv_sec;
        diff.tv_nsec = tEnd->tv_nsec - tBegin->tv_nsec;
    }

    return ((double)diff.tv_sec) + diff.tv_nsec * 1.0 /  CELIX_NS_IN_SEC;
}

struct timespec celix_gettime(clockid_t clockId) {
    struct timespec t;
    errno = 0;
    int rc = clock_gettime(clockId, &t);
    if (rc != 0) {
        fprintf(stderr, "Cannot get time from clock_gettime. Error is %s\n", strerror(errno));
    }
    return t;
}

double celix_elapsedtime(clockid_t clockId, struct timespec startTime) {
    struct timespec now = celix_gettime(clockId);
    return celix_difftime(&startTime, &now);
}

char* celix_utils_strdup(const char *str) {
    if (str != NULL) {
        return strndup(str, CELIX_UTILS_MAX_STRLEN);
    } else {
        return NULL;
    }
}


void celix_utils_extractLocalNameAndNamespaceFromFullyQualifiedName(const char *fullyQualifiedName, const char *namespaceSeparator, char **outLocalName, char **outNamespace) {
    assert(namespaceSeparator != NULL);
    if (fullyQualifiedName == NULL) {
        *outLocalName = NULL;
        *outNamespace = NULL;
        return;
    }

    char *cpy = celix_utils_strdup(fullyQualifiedName);

    char *local = NULL;
    char *namespace = NULL;
    size_t namespaceLen;
    FILE *namespaceStream = open_memstream(&namespace, &namespaceLen);

    char *savePtr = NULL;
    char *nextSubStr = NULL;
    char *currentSubStr = strtok_r(cpy, namespaceSeparator, &savePtr);
    bool firstNamespaceEntry = true;
    while (currentSubStr != NULL) {
        nextSubStr = strtok_r(NULL, namespaceSeparator, &savePtr);
        if (nextSubStr != NULL) {
            //still more, so last is part of the namespace
            if (firstNamespaceEntry) {
                firstNamespaceEntry = false;
            } else {
                fprintf(namespaceStream, "%s", namespaceSeparator);
            }
            fprintf(namespaceStream, "%s", currentSubStr);
        } else {
            //end reached current is local name
            local = celix_utils_strdup(currentSubStr);
        }
        currentSubStr = nextSubStr;
    }
    fclose(namespaceStream);
    free(cpy);
    *outLocalName = local;
    if (namespace == NULL) {
      *outNamespace = NULL;
    } else if (strncmp("", namespace, 1) == 0)  {
        //empty string -> set to NULL
        *outNamespace = NULL;
        free(namespace);
    } else {
        *outNamespace = namespace;
    }
}