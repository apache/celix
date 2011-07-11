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

int fpeek(FILE *stream);

void manifest_clear(MANIFEST manifest) {

}

PROPERTIES manifest_getMainAttributes(MANIFEST manifest) {
	return manifest->mainAttributes;
}

celix_status_t manifest_read(char *filename, MANIFEST *manifest) {
    celix_status_t status = CELIX_SUCCESS;
	MANIFEST mf = NULL;

	mf = (MANIFEST) malloc(sizeof(*mf));
	if (mf != NULL) {
		PROPERTIES mainAttributes = properties_create();

        mf->mainAttributes = mainAttributes;

        FILE *file = fopen ( filename, "r" );
        if (file != NULL) {
            char lbuf [ 512 ];
            char * lastline;
            char * name = NULL;
            char * value = NULL;

            while ( fgets ( lbuf, sizeof lbuf, file ) != NULL ) {
                bool lineContinued = false;
                int len = strlen(lbuf);

                if (lbuf[--len] != '\n') {
                    return CELIX_INVALID_SYNTAX;
                }

                if (len > 0 && lbuf[len-1] == '\r') {
                    --len;
                }

                if (len == 0) {
                    break;
                }

                int i = 0;
                if (lbuf[0] == ' ') {
                    // continuation of previous line
                    if (name == NULL) {
                        return CELIX_INVALID_SYNTAX;
                    }
                    lineContinued = true;
                    int newlen = strlen(lastline) + len - 1;
                    char buf [newlen];
                    strcpy(buf, lastline);
                    strncat(buf, lbuf+1, len - 1);

                    if (fpeek(file) == ' ') {
                        lastline = strcpy(lastline, buf);
                        continue;
                    }
                    value = (char *) malloc(strlen(buf) + 1);
                    value = strcpy(value, buf);
                    value[strlen(buf)] = '\0';
                    lastline = NULL;
                } else {
                    while (lbuf[i++] != ':') {
                        if (i >= len) {
    //						throw new IOException("invalid header field");
                            return CELIX_INVALID_SYNTAX;
                        }
                    }
                    if (lbuf[i++] != ' ') {
    //					throw new IOException("invalid header field");
                        return CELIX_INVALID_SYNTAX;
                    }
                    name = (char *) malloc((i + 1) - 2);
                    name = strncpy(name, lbuf, i - 2);
                    name[i - 2] = '\0';
                    if (fpeek(file) == ' ') {
                        int newlen = len - i;
                        lastline = (char *) malloc(newlen + 1);
                        lastline = strncpy(lastline, lbuf+i, len -i);
                        continue;
                    }
                    value = (char *) malloc((len + 1) - i);
                    value = strncpy(value, lbuf+i, len - i);
                    value[len - i] = '\0';
                }

                if ((properties_set(mainAttributes, name, value) != NULL) && (!lineContinued)) {
                    printf("Duplicate entry: %s", name);
                }
                free(name);
                free(value);
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

