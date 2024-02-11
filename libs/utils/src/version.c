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
#include <stdio.h>
#include <string.h>
#include <celix_utils.h>

#include "celix_version.h"
#include "celix_errno.h"
#include "version_private.h"
#include "celix_err.h"

static const char* const CELIX_VERSION_EMPTY_QUALIFIER = "";

celix_version_t* celix_version_create(int major, int minor, int micro, const char* qualifier) {
    if (major < 0 || minor < 0 || micro < 0) {
        celix_err_push("Invalid version number. Major, minor and micro must be >= 0");
        return NULL;
    }

    if (qualifier == NULL) {
        qualifier = CELIX_VERSION_EMPTY_QUALIFIER;
    }

    size_t qualifierLen = strlen(qualifier);
    for (int i = 0; i < qualifierLen; i++) {
        char ch = qualifier[i];
        if (('A' <= ch) && (ch <= 'Z')) {
            continue;
        }
        if (('a' <= ch) && (ch <= 'z')) {
            continue;
        }
        if (('0' <= ch) && (ch <= '9')) {
            continue;
        }
        if ((ch == '_') || (ch == '-')) {
            continue;
        }
        celix_err_push("Invalid version qualifier. Characters must be [A-Za-z0-9_-]");
        return NULL;
    }

    celix_version_t* version = calloc(1, sizeof(*version));
    if (version) {
        version->major = major;
        version->minor = minor;
        version->micro = micro;
        if (qualifierLen == 0) {
            version->qualifier = (char*)CELIX_VERSION_EMPTY_QUALIFIER;
        } else {
            version->qualifier = celix_utils_strdup(qualifier);
        }
        if (!version->qualifier) {
            celix_version_destroy(version);
            version = NULL;
        }
    }
    if (!version) {
        celix_err_push("Failed to allocate memory for celix_version_create");
    }
    return version;
}

void celix_version_destroy(celix_version_t* version) {
    if (version != NULL) {
        if (version->qualifier != CELIX_VERSION_EMPTY_QUALIFIER) {
            free(version->qualifier);
        }
        free(version);
    }
}


celix_version_t* celix_version_copy(const celix_version_t* version) {
    if (version == NULL) {
        return celix_version_createEmptyVersion();
    }
    return celix_version_create(version->major, version->minor, version->micro, version->qualifier);
}


celix_version_t* celix_version_createVersionFromString(const char *versionStr) {
    if (versionStr == NULL) {
        return NULL;
    }

    int major = 0;
    int minor = 0;
    int micro = 0;
    char * qualifier = NULL;

    char delims[] = ".";
    char *token = NULL;
    char *last = NULL;

    int i = 0;

    char* versionWrkStr = strdup(versionStr);

    celix_status_t status = CELIX_SUCCESS;
    token = strtok_r(versionWrkStr, delims, &last);
    if (token != NULL) {
        for (i = 0; i < strlen(token); i++) {
            char ch = token[i];
            if (('0' <= ch) && (ch <= '9')) {
                continue;
            }
            status = CELIX_ILLEGAL_ARGUMENT;
            break;
        }
        major = atoi(token);
        token = strtok_r(NULL, delims, &last);
        if (token != NULL) {
            for (i = 0; i < strlen(token); i++) {
                char ch = token[i];
                if (('0' <= ch) && (ch <= '9')) {
                    continue;
                }
                status = CELIX_ILLEGAL_ARGUMENT;
                break;
            }
            minor = atoi(token);
            token = strtok_r(NULL, delims, &last);
            if (token != NULL) {
                for (i = 0; i < strlen(token); i++) {
                    char ch = token[i];
                    if (('0' <= ch) && (ch <= '9')) {
                        continue;
                    }
                    status = CELIX_ILLEGAL_ARGUMENT;
                    break;
                }
                micro = atoi(token);
                token = strtok_r(NULL, delims, &last);
                if (token != NULL) {
                    qualifier = strdup(token);
                    token = strtok_r(NULL, delims, &last);
                    if (token != NULL) {
                        status = CELIX_ILLEGAL_ARGUMENT;
                    }
                }
            }
        }
    }

    free(versionWrkStr);

    celix_version_t* version = NULL;
    if (status == CELIX_SUCCESS) {
        version = celix_version_create(major, minor, micro, qualifier);
    }

    if (qualifier != NULL) {
        free(qualifier);
    }

    return version;
}


celix_version_t* celix_version_createEmptyVersion() {
    return celix_version_create(0, 0, 0, NULL);
}

int celix_version_getMajor(const celix_version_t* version) {
    return version->major;
}

int celix_version_getMinor(const celix_version_t* version) {
    return version->minor;
}

int celix_version_getMicro(const celix_version_t* version) {
    return version->micro;
}

const char* celix_version_getQualifier(const celix_version_t* version) {
    return version->qualifier;
}

int celix_version_compareTo(const celix_version_t* version, const celix_version_t* compare) {
    int result;
    if (compare == version) {
        result = 0;
    } else {
        int res = version->major - compare->major;
        if (res != 0) {
            result = res;
        } else {
            res = version->minor - compare->minor;
            if (res != 0) {
                result = res;
            } else {
                res = version->micro - compare->micro;
                if (res != 0) {
                    result = res;
                } else {
                    if(celix_utils_isStringNullOrEmpty(version->qualifier) && celix_utils_isStringNullOrEmpty(version->qualifier)) {
                        result = 0;
                    } else if (celix_utils_isStringNullOrEmpty(version->qualifier) || celix_utils_isStringNullOrEmpty(version->qualifier)) {
                        result = -1;
                    } else {
                        result = strcmp(version->qualifier, compare->qualifier);
                    }
                }
            }
        }
    }
    return result;
}


char* celix_version_toString(const celix_version_t* version) {
    char* string = NULL;
    int rc;
    if (strlen(version->qualifier) > 0) {
        rc = asprintf(&string,"%d.%d.%d.%s", version->major, version->minor, version->micro, version->qualifier);
    } else {
        rc = asprintf(&string, "%d.%d.%d", version->major, version->minor, version->micro);
    }
    if (rc < 0) {
        celix_err_push("Failed to allocate memory for celix_version_toString");
        return NULL;
    }
    return string;
}

bool celix_version_fillString(const celix_version_t* version, char *str, size_t strLen) {
    int written;
    if (strnlen(version->qualifier, 1) > 0) {
        written = snprintf(str, strLen, "%d.%d.%d.%s", version->major, version->minor, version->micro, version->qualifier);
    } else {
        written = snprintf(str, strLen, "%d.%d.%d", version->major, version->minor, version->micro);
    }
    return written >= 0 && written < strLen;
}

bool celix_version_isCompatible(const celix_version_t* user, const celix_version_t* provider) {
    if (user == NULL && provider == NULL) {
        return true;
    } else if (user == NULL || provider == NULL) {
        return false;
    } else {
        return celix_version_isUserCompatible(user, provider->major, provider->minor);
    }
}

bool celix_version_isUserCompatible(const celix_version_t* user, int providerMajorVersionPart, int provideMinorVersionPart) {
    bool isCompatible = false;
    if (providerMajorVersionPart == user->major) {
        isCompatible = (provideMinorVersionPart >= user->minor);
    }
    return isCompatible;
}

unsigned int celix_version_hash(const celix_version_t* version) {
    return (unsigned int)(version->major | version->minor | version->micro | celix_utils_stringHash(version->qualifier));
}

int celix_version_compareToMajorMinor(const celix_version_t* version, int majorVersionPart, int minorVersionPart) {
    int result = version->major - majorVersionPart;
    if (result == 0) {
        result = version->minor - minorVersionPart;
    }
    return result;
}
