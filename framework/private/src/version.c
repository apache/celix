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

#include "celix_errno.h"
#include "version_private.h"
#include "celix_log.h"

static apr_status_t version_destroy(void *handle);

celix_status_t version_createVersion(apr_pool_t *pool, int major, int minor, int micro, char * qualifier, version_pt *version) {
	celix_status_t status = CELIX_SUCCESS;

	if (*version != NULL || pool == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*version = (version_pt) apr_palloc(pool, sizeof(**version));
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
			    fw_log(OSGI_FRAMEWORK_LOG_ERROR, "Negative major");
				status = CELIX_ILLEGAL_ARGUMENT;
			}
			if (minor < 0) {
			    fw_log(OSGI_FRAMEWORK_LOG_ERROR, "Negative minor");
				status = CELIX_ILLEGAL_ARGUMENT;
			}
			if (micro < 0) {
			    fw_log(OSGI_FRAMEWORK_LOG_ERROR, "Negative micro");
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
				fw_log(OSGI_FRAMEWORK_LOG_ERROR, "Invalid qualifier");
				status = CELIX_ILLEGAL_ARGUMENT;
				break;
			}
		}
	}

	framework_logIfError(status, NULL, "Cannot create version");

	return status;
}

celix_status_t version_clone(version_pt version, apr_pool_t *pool, version_pt *clone) {
	return version_createVersion(pool, version->major, version->minor, version->micro, version->qualifier, clone);
}

apr_status_t version_destroy(void *handle) {
	version_pt version = (version_pt) handle;
	version->major = 0;
	version->minor = 0;
	version->micro = 0;
	version->qualifier = NULL;
	return APR_SUCCESS;
}

celix_status_t version_createVersionFromString(apr_pool_t *pool, char * versionStr, version_pt *version) {
	celix_status_t status = CELIX_SUCCESS;

	int major = 0;
	int minor = 0;
	int micro = 0;
	char * qualifier = "";

	char delims[] = ".";
	char *token = NULL;
	char *last = NULL;

	int i = 0;

	token = apr_strtok(versionStr, delims, &last);
	if (token != NULL) {
		for (i = 0; i < strlen(token); i++) {
			char ch = token[i];
			if (('0' <= ch) && (ch <= '9')) {
				continue;
			}
			fw_log(OSGI_FRAMEWORK_LOG_ERROR, "Invalid format");
			status = CELIX_ILLEGAL_ARGUMENT;
			break;
		}
		major = atoi(token);
		token = apr_strtok(NULL, delims, &last);
		if (token != NULL) {
			for (i = 0; i < strlen(token); i++) {
				char ch = token[i];
				if (('0' <= ch) && (ch <= '9')) {
					continue;
				}
				fw_log(OSGI_FRAMEWORK_LOG_ERROR, "Invalid format");
				status = CELIX_ILLEGAL_ARGUMENT;
				break;
			}
			minor = atoi(token);
			token = apr_strtok(NULL, delims, &last);
			if (token != NULL) {
				for (i = 0; i < strlen(token); i++) {
					char ch = token[i];
					if (('0' <= ch) && (ch <= '9')) {
						continue;
					}
					fw_log(OSGI_FRAMEWORK_LOG_ERROR, "Invalid format");
					status = CELIX_ILLEGAL_ARGUMENT;
					break;
				}
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

	framework_logIfError(status, NULL, "Cannot create version [versionString=%s]", versionStr);

	return status;
}

celix_status_t version_createEmptyVersion(apr_pool_t *pool, version_pt *version) {
	return version_createVersion(pool, 0, 0, 0, "", version);
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

celix_status_t version_getQualifier(version_pt version, char **qualifier) {
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

	framework_logIfError(status, NULL, "Cannot compare versions");

	return status;
}

celix_status_t version_toString(version_pt version, apr_pool_t *pool, char **string) {
	if (strlen(version->qualifier) > 0) {
		*string = apr_psprintf(pool, "%d.%d.%d.%s", version->major, version->minor, version->micro, version->qualifier);
	} else {
		*string = apr_psprintf(pool, "%d.%d.%d", version->major, version->minor, version->micro);
	}
	return CELIX_SUCCESS;
}
