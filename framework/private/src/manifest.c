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

void manifest_clear(MANIFEST manifest) {

}

PROPERTIES manifest_getMainAttributes(MANIFEST manifest) {
	return manifest->mainAttributes;
}

celix_status_t manifest_getEntries(MANIFEST manifest, HASH_MAP *map) {
	*map = manifest->attributes;
	return CELIX_SUCCESS;
}

celix_status_t manifest_read(char *filename, MANIFEST *manifest) {
    celix_status_t status = CELIX_SUCCESS;
	MANIFEST mf = NULL;

	mf = (MANIFEST) malloc(sizeof(*mf));
	if (mf != NULL) {
		mf->mainAttributes = properties_create();
		mf->attributes = hashMap_create(string_hash, NULL, string_equals, NULL);

        FILE *file = fopen ( filename, "r" );
        if (file != NULL) {
            char lbuf[512];
        	manifest_readAttributes(mf, mf->mainAttributes, file);

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

					if (fpeek(file) == ' ') {
						lastline = strcpy(lastline, buf);
						continue;
					}
					name = (char *) malloc(strlen(buf) + 1);
					name = strcpy(name, buf);
					name[strlen(buf)] = '\0';
					lastline = NULL;
				}

				PROPERTIES attributes = hashMap_get(mf->attributes, name);
				if (attributes == NULL) {
					attributes = properties_create();
					hashMap_put(mf->attributes, strdup(name), attributes);
				}
				manifest_readAttributes(mf, attributes, file);

                free(name);
				name = NULL;
				skipEmptyLines = true;
            }
            fclose(file);

            *manifest = mf;
        } else {
            printf("Could not read manifest file.\n");
            status = CELIX_FILE_IO_EXCEPTION;
        }
	} else {
	    status = CELIX_ENOMEM;
	}

	return status;
}

void manifest_destroy(MANIFEST manifest) {
	if (manifest != NULL) {
		properties_destroy(manifest->mainAttributes);
		manifest->mainAttributes = NULL;
		free(manifest);
		manifest = NULL;
	}
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
			int newlen = strlen(lastLine) + len - 1;
			char buf[newlen];
			strcpy(buf, lastLine);
			strncat(buf, lbuf+1, len - 1);

			if (fpeek(file) == ' ') {
				lastLine = strcpy(lastLine, buf);
				continue;
			}
			value = (char *) malloc(strlen(buf) + 1);
			value = strcpy(value, buf);
			value[strlen(buf)] = '\0';
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

