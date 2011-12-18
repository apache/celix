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
 *  Created on: Jul 5, 2010
 *      Author: alexanderb
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "celixbool.h"

#include "manifest.h"
#include "utils.h"

int fpeek(FILE *stream);
celix_status_t manifest_readAttributes(MANIFEST manifest, PROPERTIES properties, FILE *file);
apr_status_t manifest_destroy(void *manifestP);

celix_status_t manifest_create(apr_pool_t *pool, MANIFEST *manifest) {
	celix_status_t status = CELIX_SUCCESS;

	*manifest = apr_palloc(pool, sizeof(**manifest));
	if (!*manifest) {
		status = CELIX_ENOMEM;
	} else {
		apr_pool_pre_cleanup_register(pool, *manifest, manifest_destroy);
		(*manifest)->pool = pool;
		(*manifest)->mainAttributes = properties_create();
		(*manifest)->attributes = hashMap_create(string_hash, NULL, string_equals, NULL);
	}

	return status;
}

apr_status_t manifest_destroy(void *manifestP) {
	MANIFEST manifest = manifestP;
	if (manifest != NULL) {
		properties_destroy(manifest->mainAttributes);
		hashMap_destroy(manifest->attributes, false, false);
		manifest->mainAttributes = NULL;
		manifest->attributes = NULL;
		manifest = NULL;
	}
	return APR_SUCCESS;
}

celix_status_t manifest_createFromFile(apr_pool_t *pool, char *filename, MANIFEST *manifest) {
	celix_status_t status = CELIX_SUCCESS;

	status = manifest_create(pool, manifest);
	if (status == CELIX_SUCCESS) {
		manifest_read(*manifest, filename);
	}

	return status;
}

void manifest_clear(MANIFEST manifest) {

}

PROPERTIES manifest_getMainAttributes(MANIFEST manifest) {
	return manifest->mainAttributes;
}

celix_status_t manifest_getEntries(MANIFEST manifest, HASH_MAP *map) {
	*map = manifest->attributes;
	return CELIX_SUCCESS;
}

celix_status_t manifest_read(MANIFEST manifest, char *filename) {
    celix_status_t status = CELIX_SUCCESS;

	FILE *file = fopen ( filename, "r" );
	if (file != NULL) {
		char lbuf[512];
		manifest_readAttributes(manifest, manifest->mainAttributes, file);

		int len;
		char * name = NULL;
		bool skipEmptyLines = true;
		char * lastline;

		while (fgets(lbuf, sizeof(lbuf), file) != NULL ) {
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
					name = (char *) malloc((len + 1) - 6);
					name = strncpy(name, lbuf+6, len - 6);
					name[len - 6] = '\0';
				} else {
					printf("MANIFEST: Invalid manifest format\n");
					return CELIX_FILE_IO_EXCEPTION;
				}

				if (fpeek(file) == ' ') {
					int newlen = len - 6;
					lastline = (char *) malloc(newlen + 1);
					lastline = strncpy(lastline, lbuf+6, len - 6);
					lastline[newlen] = '\0';
					continue;
				}
			} else {
				int newlen = strlen(lastline) + len;
				char buf[newlen];
				strcpy(buf, lastline);
				strncat(buf, lbuf+1, len - 1);
				buf[newlen] = '\0';

				if (fpeek(file) == ' ') {
					lastline = realloc(lastline, strlen(buf) + 1);
					lastline = strcpy(lastline, buf);
					continue;
				}
				name = (char *) malloc(strlen(buf) + 1);
				name = strcpy(name, buf);
				name[strlen(buf)] = '\0';
				lastline = NULL;
			}

			PROPERTIES attributes = hashMap_get(manifest->attributes, name);
			if (attributes == NULL) {
				attributes = properties_create();
				hashMap_put(manifest->attributes, strdup(name), attributes);
			}
			manifest_readAttributes(manifest, attributes, file);

			free(name);
			name = NULL;
			skipEmptyLines = true;
		}
		fclose(file);
	} else {
		printf("Could not read manifest file.\n");
		status = CELIX_FILE_IO_EXCEPTION;
	}

	return status;
}

void manifest_write(MANIFEST manifest, char * filename) {

}

char * manifest_getValue(MANIFEST manifest, const char * name) {
	char * val = properties_get(manifest->mainAttributes, (char *) name);
	return val;
}

int fpeek(FILE *stream) {
	int c;
	c = fgetc(stream);
	ungetc(c, stream);
	return c;
}

celix_status_t manifest_readAttributes(MANIFEST manifest, PROPERTIES properties, FILE *file) {
	char *name = NULL;
	char *value = NULL;
	char *lastLine = NULL;
	char lbuf[512];

	int len;
	while (fgets(lbuf, sizeof(lbuf), file ) != NULL ) {
		len = strlen(lbuf);
		bool lineContinued = false;
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
		int i = 0;
		if (lbuf[0] == ' ') {
			// Line continued
			if (name == NULL) {
				printf("MANIFEST: No continued line expected\n");
				return CELIX_FILE_IO_EXCEPTION;
			}
			lineContinued = true;
			int newlen = strlen(lastLine) + len;
			char buf[newlen];
			strcpy(buf, lastLine);
			strncat(buf, lbuf+1, len - 1);
			buf[newlen] = '\0';

			if (fpeek(file) == ' ') {
				lastLine = realloc(lastLine, strlen(buf) + 1);
				lastLine = strcpy(lastLine, buf);
				continue;
			}
			value = (char *) malloc(strlen(buf) + 1);
			value = strcpy(value, buf);
			free(lastLine);
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
			name = (char *) malloc((i + 1) - 2);
			name = strncpy(name, lbuf, i - 2);
			name[i - 2] = '\0';
			if (fpeek(file) == ' ') {
				int newlen = len - i;
				lastLine = (char *) malloc(newlen + 1);
				lastLine = strncpy(lastLine, lbuf+i, len -i);
				lastLine[newlen] = '\0';
				continue;
			}
			value = (char *) malloc((len + 1) - i);
			value = strncpy(value, lbuf+i, len - i);
			value[len - i] = '\0';
		}

		if ((properties_set(properties, name, value) != NULL) && (!lineContinued)) {
			printf("Duplicate entry: %s", name);
		}
		free(name);
		free(value);
	}

	return CELIX_SUCCESS;
}

