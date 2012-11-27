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
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <apr_strings.h>

#include "version.h"
#include "celix_errno.h"
#include "framework_log.h"


struct version {
	apr_pool_t *pool;

	int major;
	int minor;
	int micro;
	char *qualifier;
};

static apr_status_t version_destroy(void *version);

celix_status_t version_createVersion(apr_pool_t *pool, int major, int minor, int micro, char * qualifier, VERSION *version) {
	celix_status_t status = CELIX_SUCCESS;

	*version = (VERSION) apr_palloc(pool, sizeof(**version));
	if (!*version) {
		status = CELIX_ENOMEM;
	} else {
		unsigned int i;
		apr_pool_pre_cleanup_register(pool, *version, version_destroy);

		(*version)->pool = pool;
		(*version)->major = major;
		(*version)->minor = minor;
		(*version)->micro = micro;
		if (qualifier == NULL) {
			qualifier = "";
		}
		(*version)->qualifier = apr_pstrdup(pool, qualifier);

		if (major < 0) {
			celix_log("Negative major");
		}
		if (minor < 0) {
			celix_log("Negative minor");
		}
		if (micro < 0) {
			celix_log("Negative micro");
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
			celix_log("Invalid qualifier");
		}
	}

	return status;
}

celix_status_t version_clone(VERSION version, apr_pool_t *pool, VERSION *clone) {
	return version_createVersion(pool, version->major, version->minor, version->micro, version->qualifier, clone);
}

apr_status_t version_destroy(void *versionP) {
	VERSION version = versionP;
	version->major = 0;
	version->minor = 0;
	version->micro = 0;
	version->qualifier = NULL;
	return APR_SUCCESS;
}

celix_status_t version_createVersionFromString(apr_pool_t *pool, char * versionStr, VERSION *version) {
	celix_status_t status = CELIX_SUCCESS;

	int major = 0;
	int minor = 0;
	int micro = 0;
	char * qualifier = "";

	char delims[] = ".";
	char *token = NULL;
	char *last;

	token = apr_strtok(versionStr, delims, &last);
	if (token != NULL) {
		major = atoi(token);
		token = apr_strtok(NULL, delims, &last);
		if (token != NULL) {
			minor = atoi(token);
			token = apr_strtok(NULL, delims, &last);
			if (token != NULL) {
				micro = atoi(token);
				token = apr_strtok(NULL, delims, &last);
				if (token != NULL) {
					qualifier = apr_pstrdup(pool, token);
					token = apr_strtok(NULL, delims, &last);
					if (token != NULL) {
						printf("invalid format");
						*version = NULL;
						status = CELIX_ILLEGAL_ARGUMENT;
					}
				}
			}
		}
	}
	if (status == CELIX_SUCCESS) {
		status = version_createVersion(pool, major, minor, micro, qualifier, version);
	}
	return status;
}

celix_status_t version_createEmptyVersion(apr_pool_t *pool, VERSION *version) {
	return version_createVersion(pool, 0, 0, 0, "", version);
}

celix_status_t version_compareTo(VERSION version, VERSION compare, int *result) {
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

celix_status_t version_toString(VERSION version, apr_pool_t *pool, char **string) {
	if (strlen(version->qualifier) > 0) {
		*string = apr_psprintf(pool, "%d.%d.%d.%s", version->major, version->minor, version->micro, version->qualifier);
	} else {
		*string = apr_psprintf(pool, "%d.%d.%d", version->major, version->minor, version->micro);
	}
	return CELIX_SUCCESS;
}
