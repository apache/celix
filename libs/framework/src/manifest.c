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
/**
 * manifest.c
 *
 *  \date       Jul 5, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <string.h>
#include <stdbool.h>

#include "celix_stdio_cleanup.h"
#include "manifest.h"
#include "utils.h"
#include "celix_log.h"

int fpeek(FILE *stream);

static celix_status_t manifest_readAttributes(manifest_pt manifest, properties_pt properties, FILE *file);

celix_status_t manifest_create(manifest_pt *manifest) {
	celix_status_t status = CELIX_SUCCESS;

	*manifest = malloc(sizeof(**manifest));
	if (!*manifest) {
		status = CELIX_ENOMEM;
	} else {
		(*manifest)->mainAttributes = properties_create();
		(*manifest)->attributes = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
	}

	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot create manifest");

	return status;
}

manifest_pt manifest_clone(manifest_pt manifest) {
	celix_status_t status = CELIX_SUCCESS;

	manifest_pt clone = NULL;
	status = manifest_create(&clone);
	if (status == CELIX_SUCCESS) {
		hash_map_iterator_t iter = hashMapIterator_construct(manifest->attributes);
		while (hashMapIterator_hasNext(&iter)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(&iter);
			char *key = hashMapEntry_getKey(entry);
			celix_properties_t* value = hashMapEntry_getValue(entry);
			celix_properties_t* cloneValue = celix_properties_copy(value);
			hashMap_put(clone->attributes, key, cloneValue);
		}
	}

	return clone;
}

celix_status_t manifest_destroy(manifest_pt manifest) {
    if (manifest != NULL) {
        properties_destroy(manifest->mainAttributes);
        hash_map_iterator_t iter = hashMapIterator_construct(manifest->attributes);
        while (hashMapIterator_hasNext(&iter)) {
            hash_map_entry_pt entry = hashMapIterator_nextEntry(&iter);
            celix_properties_t* value = hashMapEntry_getValue(entry);
            celix_properties_destroy(value);
        }
        hashMap_destroy(manifest->attributes, true, false);
        manifest->mainAttributes = NULL;
        manifest->attributes = NULL;
        free(manifest);
        manifest = NULL;
    }
    return CELIX_SUCCESS;
}

celix_status_t manifest_createFromFile(const char *filename, manifest_pt *manifest) {
	celix_status_t status;

	status = manifest_create(manifest);

	if (status == CELIX_SUCCESS) {
		manifest_read(*manifest, filename);
	}

	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot create manifest from file");

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

celix_status_t manifest_read(manifest_pt manifest, const char *filename) {
    celix_status_t status = CELIX_SUCCESS;

    celix_autoptr(FILE) file = fopen(filename, "r");
    if (file != NULL) {
        status = manifest_readFromStream(manifest, file);
    } else {
        status = CELIX_FILE_IO_EXCEPTION;
    }

    if (status != CELIX_SUCCESS) {
        fw_logCode(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, status,
                   "Cannot read manifest %s", filename);
    }

    return status;
}

celix_status_t manifest_readFromStream(manifest_pt manifest, FILE* stream) {
    celix_status_t status = CELIX_SUCCESS;

    char lbuf[512];
    char name[512];
    bool skipEmptyLines = true;
    char lastline[512];
    memset(lbuf,0,512);
    memset(name,0,512);
    memset(lastline,0,512);

    manifest_readAttributes(manifest, manifest->mainAttributes, stream);

    while (status==CELIX_SUCCESS && fgets(lbuf, sizeof(lbuf), stream) != NULL) {
        properties_pt attributes;
        int len = strlen(lbuf);

        if (lbuf[--len] != '\n') {
            status = CELIX_FILE_IO_EXCEPTION;
            framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Manifest line too long");
            break;
        }
        if (len > 0 && lbuf[len - 1] == '\r') {
            --len;
        }
        if (len == 0 && skipEmptyLines) {
            continue;
        }
        skipEmptyLines = false;

        if (strlen(name) == 0) {

            if ((tolower(lbuf[0]) == 'n') && (tolower(lbuf[1]) == 'a') &&
                (tolower(lbuf[2]) == 'm') && (tolower(lbuf[3]) == 'e') &&
                (lbuf[4] == ':') && (lbuf[5] == ' ')) {
                name[0] = '\0';
                strncpy(name, lbuf+6, len - 6);
                name[len - 6] = '\0';
            } else {
                status = CELIX_FILE_IO_EXCEPTION;
                framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Manifest invalid format");
                break;
            }

            if (fpeek(stream) == ' ') {
                int newlen = len - 6;
                lastline[0] = '\0';
                strncpy(lastline, lbuf+6, len - 6);
                lastline[newlen] = '\0';
                continue;
            }
        } else {
            int newlen = strlen(lastline) + len;
            char buf[512];
            buf[0] = '\0';
            strcpy(buf, lastline);
            strncat(buf, lbuf+1, len - 1);
            buf[newlen] = '\0';

            if (fpeek(stream) == ' ') {
                lastline[0] = '\0';
                strcpy(lastline, buf);
                continue;
            }
            name[0] = '\0';
            strcpy(name, buf);
            name[strlen(buf)] = '\0';
        }

        attributes = hashMap_get(manifest->attributes, name);
        if (attributes == NULL) {
            attributes = properties_create();
            hashMap_put(manifest->attributes, strdup(name), attributes);
        }
        manifest_readAttributes(manifest, attributes, stream);

        name[0] = '\0';
        skipEmptyLines = true;
    }

    if (status != CELIX_SUCCESS) {
        fw_logCode(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, status, "Cannot read manifest");
    }

    return status;
}

void manifest_write(manifest_pt manifest, const char * filename) {

}

const char* manifest_getValue(manifest_pt manifest, const char* name) {
	const char* val = properties_get(manifest->mainAttributes, name);
	bool isEmpty = utils_isStringEmptyOrNull(val);
	return isEmpty ? NULL : val;
}

int fpeek(FILE *stream) {
	int c;
	c = fgetc(stream);
	ungetc(c, stream);
	return c;
}

static celix_status_t manifest_readAttributes(manifest_pt manifest, properties_pt properties, FILE *file) {
	char name[512]; memset(name,0,512);
	char value[512]; memset(value,0,512);
	char lastLine[512]; memset(lastLine,0,512);
	char lbuf[512]; memset(lbuf,0,512);


	while (fgets(lbuf, sizeof(lbuf), file ) != NULL ) {
		int len = strlen(lbuf);

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
			char buf[512];
			buf[0] = '\0';

			// Line continued
			strcat(buf, lastLine);
			strncat(buf, lbuf+1, len - 1);

			if (fpeek(file) == ' ') {
//				lastLine = realloc(lastLine, strlen(buf) + 1);
				lastLine[0] = '\0';
				strcpy(lastLine, buf);
				continue;
			}
			value[0] = '\0';
			strcpy(value, buf);
		} else {
	        int i = 0;
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
			name[0] = '\0';
			strncpy(name, lbuf, i - 2);
			name[i - 2] = '\0';
			if (fpeek(file) == ' ') {
				int newlen = len - i;
				lastLine[0] = '\0';
				strncpy(lastLine, lbuf+i, len -i);
				lastLine[newlen] = '\0';
				continue;
			}
			value[0] = '\0';
			strncpy(value, lbuf+i, len - i);
			value[len - i] = '\0';
		}

		properties_set(properties, name, value);
	}

	return CELIX_SUCCESS;
}

