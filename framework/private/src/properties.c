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
 * properties.c
 *
 *  \date       Apr 27, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "celixbool.h"
#include "properties.h"
#include "utils.h"

properties_pt properties_create(void) {
	return hashMap_create(utils_stringHash, utils_stringHash, utils_stringEquals, utils_stringEquals);
}

void properties_destroy(properties_pt properties) {
	hash_map_iterator_pt iter = hashMapIterator_create(properties);
	while (hashMapIterator_hasNext(iter)) {
		hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
		free(hashMapEntry_getKey(entry));
		free(hashMapEntry_getValue(entry));
	}
	hashMapIterator_destroy(iter);
	hashMap_destroy(properties, false, false);
}

properties_pt properties_load(char *filename) {
	properties_pt props = properties_create();
	FILE *file = fopen ( filename, "r" );

	char line[1024];
	char key[1024];
	char value[1024];
	bool precedingCharIsBackslash = false;
	bool isComment = false;
	int valueStart = 0;
	int linePos = 0;
	int outputPos = 0;
	char *output = NULL;


	while ( fgets ( line, sizeof line, file ) != NULL ) {

		linePos = 0;
		precedingCharIsBackslash = false;
		isComment = false;
		output = NULL;
		outputPos = 0;
		key[0] = '\0';
		value[0] = '\0';

		while (line[linePos] != '\0') {
			if (line[linePos] == ' ' || line[linePos] == '\t') {
				if (output == NULL) {
					//ignore
					linePos+=1;
					continue;
				} else {
					output[outputPos++] = line[linePos];
				}
			} else {
				if (output == NULL) {
					output = key;
				}
			}
			if (line[linePos] == '=' || line[linePos] == ':' || line[linePos] == '#' || line[linePos] == '!') {
				if (precedingCharIsBackslash) {
					//escaped special character
					output[outputPos++] = line[linePos];
				} else {
					if (line[linePos] == '#' || line[linePos] == '!') {
						if (outputPos == 0) {
							isComment = true;
							break;
						} else {
							output[outputPos++] = line[linePos];
						}
					} else { // = or :
						if (output == value) { //already have a seperator
							output[outputPos++] =line[linePos];
						} else {
							output[outputPos++] = '\0';
							output = value;
							outputPos = 0;
						}
					}
				}
			} else if (line[linePos] == '\\') {
				if (precedingCharIsBackslash) { //double backslash -> backslash
						output[outputPos++] = '\\';
				}
				precedingCharIsBackslash = !precedingCharIsBackslash;
			} else { //normal character
				precedingCharIsBackslash = false;
				output[outputPos++] = line[linePos];
			}
			linePos+=1;
		}
		if (output != NULL) {
			output[outputPos] = '\0';
		}

		if (!isComment) {
			printf("putting 'key'/'value' '%s'/'%s' in properties\n", utils_stringTrim(key), utils_stringTrim(value));
			hashMap_put(props, strdup(utils_stringTrim(key)), strdup(utils_stringTrim(value)));
		}
	}

	fclose(file);

	return props;
}

/*
properties_pt properties_load_tmp(char * filename) {
	properties_pt props = properties_create();
	FILE *file = fopen ( filename, "r" );

	char * cont = strdup("\\");

	if (file != NULL) {
		char line [ 1024 ];
		char * key = NULL;
		char * value = NULL;
		int split = 0;

		while ( fgets ( line, sizeof line, file ) != NULL ) {
			if (!split) {
				unsigned int pos = strcspn(line, "=");
				if (pos != strlen(line)) {
					char * ival = NULL;
					key = utils_stringTrim(string_ndup((char *)line, pos));
					ival = string_ndup(line+pos+1, strlen(line));
					value = utils_stringTrim(ival);
					if (value != NULL) {
						char * cmp = string_ndup(value+strlen(value)-1, 1);
						if (strcmp(cont, cmp) == 0) {
							split = 1;
							value = string_ndup(value, strlen(value)-1);
						} else {
							char * old = hashMap_put(props, strdup(key), strdup(value));
						}
						free(cmp);
						free(ival);
					}
					free(key);
				}
			} else {
				if (strcmp(cont, string_ndup(line+strlen(line)-1, 1)) == 0) {
					split = 1;
					strcat(value, string_ndup(line, strlen(line)-1));
				} else {
					char * old = NULL;
					split = 0;
					strcat(value, utils_stringTrim(line));
					old = hashMap_put(props, strdup(key), strdup(value));
				}
			}
		}
		fclose(file);
	}
	free(cont);
	return props;
}
*/

/**
 * Header is ignored for now, cannot handle comments yet
 */
void properties_store(properties_pt properties, char * filename, char * header) {
	FILE *file = fopen ( filename, "w+" );
	if (file != NULL) {
		if (hashMap_size(properties) > 0) {
			hash_map_iterator_pt iterator = hashMapIterator_create(properties);
			while (hashMapIterator_hasNext(iterator)) {
				hash_map_entry_pt entry = hashMapIterator_nextEntry(iterator);

				char * line = strdup(hashMapEntry_getKey(entry));
				strcat(line, "=");
				strcat(line, strdup(hashMapEntry_getValue(entry)));
				strcat(line, "\n");
				fputs(line, file);
			}
		}
		fclose(file);
	} else {
		perror("File is null");
	}
}

char * properties_get(properties_pt properties, char * key) {
	return hashMap_get(properties, key);
}

char * properties_getWithDefault(properties_pt properties, char * key, char * defaultValue) {
	char * value = properties_get(properties, key);
	return value == NULL ? defaultValue : value;
}

char * properties_set(properties_pt properties, char * key, char * value) {
	return hashMap_put(properties, strdup(key), strdup(value));
}
