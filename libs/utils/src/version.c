/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
/*
 * version.c
 *
 *  \date       Jul 12, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "celix_errno.h"
#include "version_private.h"

celix_status_t version_createVersion(int major, int minor, int micro, char * qualifier, version_pt *version) {
	celix_status_t status = CELIX_SUCCESS;

	if (*version != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*version = (version_pt) malloc(sizeof(**version));
		if (!*version) {
			status = CELIX_ENOMEM;
		} else {
			unsigned int i;

			(*version)->major = major;
			(*version)->minor = minor;
			(*version)->micro = micro;
			if (qualifier == NULL) {
				qualifier = "";
			}
			(*version)->qualifier = strdup(qualifier);

			if (major < 0) {
				status = CELIX_ILLEGAL_ARGUMENT;
			}
			if (minor < 0) {
				status = CELIX_ILLEGAL_ARGUMENT;
			}
			if (micro < 0) {
				status = CELIX_ILLEGAL_ARGUMENT;
			}

			for (i = 0; i < strlen(qualifier); i++) {
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
				status = CELIX_ILLEGAL_ARGUMENT;
				break;
			}
		}
	}

	return status;
}

celix_status_t version_clone(version_pt version, version_pt *clone) {
	return version_createVersion(version->major, version->minor, version->micro, version->qualifier, clone);
}

celix_status_t version_destroy(version_pt version) {
	version->major = 0;
	version->minor = 0;
	version->micro = 0;
	free(version->qualifier);
	version->qualifier = NULL;
	free(version);
	return CELIX_SUCCESS;
}

celix_status_t version_createVersionFromString(const char * versionStr, version_pt *version) {
	celix_status_t status = CELIX_SUCCESS;

	if (versionStr == NULL) {
		*version = NULL;
		return status;
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
						*version = NULL;
						status = CELIX_ILLEGAL_ARGUMENT;
					}
				}
			}
		}
	}

	free(versionWrkStr);

	if (status == CELIX_SUCCESS) {
		status = version_createVersion(major, minor, micro, qualifier, version);
	}

	if (qualifier != NULL) {
	    free(qualifier);
	}

	return status;
}

celix_status_t version_createEmptyVersion(version_pt *version) {
	return version_createVersion(0, 0, 0, "", version);
}

celix_status_t version_getMajor(version_pt version, int *major) {
	celix_status_t status = CELIX_SUCCESS;
	*major = version->major;
	return status;
}

celix_status_t version_getMinor(version_pt version, int *minor) {
	celix_status_t status = CELIX_SUCCESS;
	*minor = version->minor;
	return status;
}

celix_status_t version_getMicro(version_pt version, int *micro) {
	celix_status_t status = CELIX_SUCCESS;
	*micro = version->micro;
	return status;
}

celix_status_t version_getQualifier(version_pt version, const char **qualifier) {
	celix_status_t status = CELIX_SUCCESS;
	*qualifier = version->qualifier;
	return status;
}

celix_status_t version_compareTo(version_pt version, version_pt compare, int *result) {
	celix_status_t status = CELIX_SUCCESS;
	if (compare == version) {
		*result = 0;
	} else {
		int res = version->major - compare->major;
		if (res != 0) {
			*result = res;
		} else {
			res = version->minor - compare->minor;
			if (res != 0) {
				*result = res;
			} else {
				res = version->micro - compare->micro;
				if (res != 0) {
					*result = res;
				} else {
					*result = strcmp(version->qualifier, compare->qualifier);
				}
			}
		}
	}

	return status;
}

celix_status_t version_toString(version_pt version, char **string) {
    celix_status_t status = CELIX_SUCCESS;
	if (strlen(version->qualifier) > 0) {
	    char str[512];
	    int written = snprintf(str, 512, "%d.%d.%d.%s", version->major, version->minor, version->micro, version->qualifier);
	    if (written >= 512 || written < 0) {
	        status = CELIX_BUNDLE_EXCEPTION;
	    }
	    *string = strdup(str);
	} else {
	    char str[512];
        int written = snprintf(str, 512, "%d.%d.%d", version->major, version->minor, version->micro);
        if (written >= 512 || written < 0) {
            status = CELIX_BUNDLE_EXCEPTION;
        }
        *string = strdup(str);
	}
	return status;
}

celix_status_t version_isCompatible(version_pt user, version_pt provider, bool* isCompatible) {
    celix_status_t status = CELIX_SUCCESS;
    bool result = false;

    if (user == NULL || provider == NULL) {
        result = true; // When no version defined, always respond as compatible
    } else if (user->major == provider->major) {
        result = (provider->minor >= user->minor);
    }

    *isCompatible = result;

    return status;
}
