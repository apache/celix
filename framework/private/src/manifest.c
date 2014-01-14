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
 * manifest.c
 *
 *  \date       Jul 5, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "celixbool.h"

#include "manifest.h"
#include "utils.h"
#include "celix_log.h"

int fpeek(FILE *stream);
celix_status_t manifest_readAttributes(manifest_pt manifest, properties_pt properties, FILE *file);
apr_status_t manifest_destroy(void *manifestP);

celix_status_t manifest_create(apr_pool_t *pool, manifest_pt *manifest) {
	celix_status_t status = CELIX_SUCCESS;

	*manifest = apr_palloc(pool, sizeof(**manifest));
	if (!*manifest) {
		status = CELIX_ENOMEM;
	} else {
		apr_pool_pre_cleanup_register(pool, *manifest, manifest_destroy);
		(*manifest)->pool = pool;
		(*manifest)->mainAttributes = properties_create();
		(*manifest)->attributes = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	}

	framework_logIfError(logger, status, NULL, "Cannot create manifest");

	return status;
}

apr_status_t manifest_destroy(void *manifestP) {
	manifest_pt manifest = manifestP;
	if (manifest != NULL) {
		properties_destroy(manifest->mainAttributes);
		hashMap_destroy(manifest->attributes, false, false);
		manifest->mainAttributes = NULL;
		manifest->attributes = NULL;
		manifest = NULL;
	}
	return APR_SUCCESS;
}

celix_status_t manifest_createFromFile(apr_pool_t *pool, char *filename, manifest_pt *manifest) {
	celix_status_t status = CELIX_SUCCESS;

	status = manifest_create(pool, manifest);
	if (status == CELIX_SUCCESS) {
		manifest_read(*manifest, filename);
	}

	framework_logIfError(logger, status, NULL, "Cannot create manifest from file");

	return status;
}

void manifest_clear(manifest_pt manifest) {

}

properties_pt manifest_getMainAttributes(manifest_pt manifest) {
	return manifest->mainAttributes;
}

celix_status_t manifest_getEntries(manifest_pt manifest, hash_map_pt *map) {
	*map = manifest->attributes;
	return CELIX_SUCCESS;
}

celix_status_t manifest_read(manifest_pt manifest, char *filename) {
    celix_status_t status = CELIX_SUCCESS;

	FILE *file = fopen ( filename, "r" );
	if (file != NULL) {
		char lbuf[512];
		int len;
		char * name = NULL;
		bool skipEmptyLines = true;
		char * lastline = NULL;
		apr_pool_t *subpool;

		manifest_readAttributes(manifest, manifest->mainAttributes, file);
		
		apr_pool_create(&subpool, manifest->pool);
		while (fgets(lbuf, sizeof(lbuf), file) != NULL ) {
			properties_pt attributes;

			len = strlen(lbuf);
			if (lbuf[--len] != '\n') {
				printf("MANIFEST: Line too long\n");
				return CELIX_FILE_IO_EXCEPTION;
			}
			if (len > 0 && lbuf[len - 1] == '\r') {
				--len;
			}
			if (len == 0 && skipEmptyLines) {
				continue;
			}
			skipEmptyLines = false;

			if (name == NULL) {
				
				if ((tolower(lbuf[0]) == 'n') && (tolower(lbuf[1]) == 'a') &&
					(tolower(lbuf[2]) == 'm') && (tolower(lbuf[3]) == 'e') &&
					(lbuf[4] == ':') && (lbuf[5] == ' ')) {
					name = (char *) apr_palloc(subpool, (len + 1) - 6);
					name[0] = '\0';
					name = strncpy(name, lbuf+6, len - 6);
					name[len - 6] = '\0';
				} else {
					printf("MANIFEST: Invalid manifest format\n");
					return CELIX_FILE_IO_EXCEPTION;
				}

				if (fpeek(file) == ' ') {
					int newlen = len - 6;
					lastline = (char *) apr_palloc(subpool, newlen + 1);
					lastline[0] = '\0';
					lastline = strncpy(lastline, lbuf+6, len - 6);
					lastline[newlen] = '\0';
					continue;
				}
			} else {
				int newlen = strlen(lastline) + len;
				char * buf = (char *) apr_palloc(subpool, newlen);
				buf[0] = '\0';
				strcpy(buf, lastline);
				strncat(buf, lbuf+1, len - 1);
				buf[newlen] = '\0';

				if (fpeek(file) == ' ') {
//					lastline = realloc(lastline, strlen(buf) + 1);
					lastline = (char *) apr_palloc(subpool, strlen(buf) + 1);
					lastline[0] = '\0';
					lastline = strcpy(lastline, buf);
					continue;
				}
				name = (char *) apr_palloc(subpool, strlen(buf) + 1);
				name[0] = '\0';
				name = strcpy(name, buf);
				name[strlen(buf)] = '\0';
				lastline = NULL;
			}

			attributes = hashMap_get(manifest->attributes, name);
			if (attributes == NULL) {
				attributes = properties_create();
				hashMap_put(manifest->attributes, strdup(name), attributes);
			}
			manifest_readAttributes(manifest, attributes, file);

			name = NULL;
			skipEmptyLines = true;
		}
		apr_pool_destroy(subpool);
		fclose(file);
	} else {
		status = CELIX_FILE_IO_EXCEPTION;
	}

	framework_logIfError(logger, status, NULL, "Cannot read manifest");

	return status;
}

void manifest_write(manifest_pt manifest, char * filename) {

}

char * manifest_getValue(manifest_pt manifest, const char * name) {
	char * val = properties_get(manifest->mainAttributes, (char *) name);
	return val;
}

int fpeek(FILE *stream) {
	int c;
	c = fgetc(stream);
	ungetc(c, stream);
	return c;
}

celix_status_t manifest_readAttributes(manifest_pt manifest, properties_pt properties, FILE *file) {
	char *name = NULL;
	char *value = NULL;
	char *lastLine = NULL;
	char lbuf[512];

	int len;
	apr_pool_t *subpool;
	apr_pool_create(&subpool, manifest->pool);
	while (fgets(lbuf, sizeof(lbuf), file ) != NULL ) {

		bool lineContinued = false;
		int i = 0;
		len = strlen(lbuf);
		if (lbuf[--len] != '\n') {
			printf("MANIFEST: Line too long\n");
			return CELIX_FILE_IO_EXCEPTION;
		}
		if (len > 0 && lbuf[len - 1] == '\r') {
			--len;
		}
		if (len == 0) {
			break;
		}
		
		if (lbuf[0] == ' ') {
			int newlen = strlen(lastLine) + len;
			char * buf = (char *) apr_palloc(subpool, newlen);
			buf[0] = '\0';

			// Line continued
			if (name == NULL) {
				free(buf);
				printf("MANIFEST: No continued line expected\n");
				return CELIX_FILE_IO_EXCEPTION;
			}
			lineContinued = true;
			strcat(buf, lastLine);
			strncat(buf, lbuf+1, len - 1);

			if (fpeek(file) == ' ') {
//				lastLine = realloc(lastLine, strlen(buf) + 1);
				lastLine = (char *) apr_palloc(subpool, strlen(buf) + 1);
				lastLine[0] = '\0';
				lastLine = strcpy(lastLine, buf);
				continue;
			}
			value = (char *) apr_palloc(subpool, strlen(buf) + 1);
			value[0] = '\0';
			value = strcpy(value, buf);
			lastLine = NULL;
		} else {
			while (lbuf[i++] != ':') {
				if (i >= len) {
					printf("MANIFEST: Invalid header\n");
					return CELIX_FILE_IO_EXCEPTION;
				}
			}
			if (lbuf[i++] != ' ') {
				printf("MANIFEST: Invalid header\n");
				return CELIX_FILE_IO_EXCEPTION;
			}
			name = (char *) apr_palloc(subpool, (i + 1) - 2);
			name[0] = '\0';
			name = strncpy(name, lbuf, i - 2);
			name[i - 2] = '\0';
			if (fpeek(file) == ' ') {
				int newlen = len - i;
				lastLine = (char *) apr_palloc(subpool, newlen + 1);
				lastLine[0] = '\0';
				lastLine = strncpy(lastLine, lbuf+i, len -i);
				lastLine[newlen] = '\0';
				continue;
			}
			value = (char *) apr_palloc(subpool, (len + 1) - i);
			value[0] = '\0';
			value = strncpy(value, lbuf+i, len - i);
			value[len - i] = '\0';
		}

		if ((properties_set(properties, name, value) != NULL) && (!lineContinued)) {
			printf("Duplicate entry: %s", name);
		}
	}
	apr_pool_destroy(subpool);

	return CELIX_SUCCESS;
}

