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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "celixbool.h"
#include "properties.h"
#include "utils.h"

#define MALLOC_BLOCK_SIZE		5

static void parseLine(const char* line, properties_pt props);

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

properties_pt properties_load(const char* filename) {
	FILE *file = fopen(filename, "r");
	if(file==NULL){
		return NULL;
	}
	properties_pt props = properties_loadWithStream(file);
	fclose(file);
	return props;
}

properties_pt properties_loadWithStream(FILE *file) {
	properties_pt props = NULL;


	if (file != NULL ) {
		char *saveptr;
		char *filebuffer = NULL;
		char *line = NULL;
		size_t file_size = 0;

		props = properties_create();
		fseek(file, 0, SEEK_END);
		file_size = ftell(file);
		fseek(file, 0, SEEK_SET);

		if(file_size > 0){
			filebuffer = calloc(file_size + 1, sizeof(char));
			if(filebuffer) {
				size_t rs = fread(filebuffer, sizeof(char), file_size, file);
				if(rs != file_size){
					fprintf(stderr,"fread read only %lu bytes out of %lu\n",rs,file_size);
				}
				filebuffer[file_size]='\0';
				line = strtok_r(filebuffer, "\n", &saveptr);
				while ( line != NULL ) {
					parseLine(line, props);
					line = strtok_r(NULL, "\n", &saveptr);
				}
				free(filebuffer);
			}
		}
	}

	return props;
}


/**
 * Header is ignored for now, cannot handle comments yet
 */
void properties_store(properties_pt properties, const char* filename, const char* header) {
	FILE *file = fopen ( filename, "w+" );
	char *str;

	if (file != NULL) {
		if (hashMap_size(properties) > 0) {
			hash_map_iterator_pt iterator = hashMapIterator_create(properties);
			while (hashMapIterator_hasNext(iterator)) {
				hash_map_entry_pt entry = hashMapIterator_nextEntry(iterator);
				str = hashMapEntry_getKey(entry);
				for (int i = 0; i < strlen(str); i += 1) {
					if (str[i] == '#' || str[i] == '!' || str[i] == '=' || str[i] == ':') {
						fputc('\\', file);
					}
					fputc(str[i], file);
				}

				fputc('=', file);

				str = hashMapEntry_getValue(entry);
				for (int i = 0; i < strlen(str); i += 1) {
					if (str[i] == '#' || str[i] == '!' || str[i] == '=' || str[i] == ':') {
						fputc('\\', file);
					}
					fputc(str[i], file);
				}

				fputc('\n', file);

			}
			hashMapIterator_destroy(iterator);
		}
		fclose(file);
	} else {
		perror("File is null");
	}
}

celix_status_t properties_copy(properties_pt properties, properties_pt *out) {
	celix_status_t status = CELIX_SUCCESS;
	properties_pt copy = properties_create();

	if (copy != NULL) {
		hash_map_iterator_pt iter = hashMapIterator_create(properties);
		while (hashMapIterator_hasNext(iter)) {
			hash_map_entry_pt entry = hashMapIterator_nextEntry(iter);
			char *key = hashMapEntry_getKey(entry);
			char *value = hashMapEntry_getValue(entry);
			properties_set(copy, key, value);
		}
		hashMapIterator_destroy(iter);
	} else {
		status = CELIX_ENOMEM;
	}

	if (status == CELIX_SUCCESS) {
		*out = copy;
	}

	return status;
}

const char* properties_get(properties_pt properties, const char* key) {
	return hashMap_get(properties, (void*)key);
}

const char* properties_getWithDefault(properties_pt properties, const char* key, const char* defaultValue) {
	const char* value = properties_get(properties, key);
	return value == NULL ? defaultValue : value;
}

void properties_set(properties_pt properties, const char* key, const char* value) {
	hash_map_entry_pt entry = hashMap_getEntry(properties, key);
	char* oldValue = NULL;
	if (entry != NULL) {
		char* oldKey = hashMapEntry_getKey(entry);
		oldValue = hashMapEntry_getValue(entry);
		hashMap_put(properties, oldKey, strndup(value, 1024*10));
	} else {
		hashMap_put(properties, strndup(key, 1024*10), strndup(value, 1024*10));
	}
	free(oldValue);
}

static void updateBuffers(char **key, char ** value, char **output, int outputPos, int *key_len, int *value_len) {
	if (*output == *key) {
		if (outputPos == (*key_len) - 1) {
			(*key_len) += MALLOC_BLOCK_SIZE;
			*key = realloc(*key, *key_len);
			*output = *key;
		}
	}
	else {
		if (outputPos == (*value_len) - 1) {
			(*value_len) += MALLOC_BLOCK_SIZE;
			*value = realloc(*value, *value_len);
			*output = *value;
		}
	}
}

static void parseLine(const char* line, properties_pt props) {
	int linePos = 0;
	bool precedingCharIsBackslash = false;
	bool isComment = false;
	int outputPos = 0;
	char *output = NULL;
	int key_len = MALLOC_BLOCK_SIZE;
	int value_len = MALLOC_BLOCK_SIZE;
	linePos = 0;
	precedingCharIsBackslash = false;
	isComment = false;
	output = NULL;
	outputPos = 0;

	//Ignore empty lines
	if (line[0] == '\n' && line[1] == '\0') {
		return;
	}

	char *key = calloc(1, key_len);
	char *value = calloc(1, value_len);
	key[0] = '\0';
	value[0] = '\0';

	while (line[linePos] != '\0') {
		if (line[linePos] == ' ' || line[linePos] == '\t') {
			if (output == NULL) {
				//ignore
				linePos += 1;
				continue;
			}
			else {
				output[outputPos++] = line[linePos];
				updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
			}
		}
		else {
			if (output == NULL) {
				output = key;
			}
		}
		if (line[linePos] == '=' || line[linePos] == ':' || line[linePos] == '#' || line[linePos] == '!') {
			if (precedingCharIsBackslash) {
				//escaped special character
				output[outputPos++] = line[linePos];
				updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
				precedingCharIsBackslash = false;
			}
			else {
				if (line[linePos] == '#' || line[linePos] == '!') {
					if (outputPos == 0) {
						isComment = true;
						break;
					}
					else {
						output[outputPos++] = line[linePos];
						updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
					}
				}
				else { // = or :
					if (output == value) { //already have a seperator
						output[outputPos++] = line[linePos];
						updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
					}
					else {
						output[outputPos++] = '\0';
						updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
						output = value;
						outputPos = 0;
					}
				}
			}
		}
		else if (line[linePos] == '\\') {
			if (precedingCharIsBackslash) { //double backslash -> backslash
				output[outputPos++] = '\\';
				updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
			}
			precedingCharIsBackslash = true;
		}
		else { //normal character
			precedingCharIsBackslash = false;
			output[outputPos++] = line[linePos];
			updateBuffers(&key, &value, &output, outputPos, &key_len, &value_len);
		}
		linePos += 1;
	}
	if (output != NULL) {
		output[outputPos] = '\0';
	}

	if (!isComment) {
		//printf("putting 'key'/'value' '%s'/'%s' in properties\n", utils_stringTrim(key), utils_stringTrim(value));
		properties_set(props, utils_stringTrim(key), utils_stringTrim(value));
	}
	if(key) {
		free(key);
	}
	if(value) {
		free(value);
	}

}
