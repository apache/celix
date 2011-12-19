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
 *  Created on: Apr 27, 2010
 *      Author: dk489
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include "properties.h"
#include "utils.h"

//DEFINE_HASHTABLE_INSERT(prop_insert, struct key, struct value);
//DEFINE_HASHTABLE_REMOVE(prop_remove, struct key, struct value);
//DEFINE_HASHTABLE_SEARCH(prop_search, struct key, struct value);
//
//static unsigned int hashfromkey(void * ks) {
//	struct key *k = (struct key *) ks;
//	unsigned long hash = 5381;
//	int i;
//	char * ck = strdup(k->key);
//	for (i=0; i<strlen(ck); ++i) hash = 33*hash + ck[i];
//	//int c;
//	//while (c = *ck++) {
//	//	hash = ((hash << 5) + hash) + c;
//	//}
//	free(ck);
//	return hash;
//}
//
//static int equalskey(void * k1, void * k2) {
//	struct key * key1 = (struct key *) k1;
//	struct key * key2 = (struct key *) k2;
//	return (strcmp(key1->key, key2->key) == 0);
//}
//
//char * addEntry(HASHTABLE properties, char * k, char * v);
//char * addEntry(HASHTABLE properties, char * k, char * v) {
//	struct key * search_key = (struct key *) malloc(sizeof(*search_key));
//	struct key * s_key = (struct key *) malloc(sizeof(*s_key));
//	struct value * s_value = (struct value *) malloc(sizeof(*s_value));
//	struct value * oldValue = NULL;
//	s_key->key = k;
//	search_key->key = k;
//	s_value->value = v;
//	oldValue = prop_search(properties, search_key);
//	if (oldValue != NULL) {
//		prop_remove(properties, search_key);
//	}
//	free(search_key);
//	prop_insert(properties, s_key, s_value);
//
//	return oldValue == NULL ? NULL : oldValue->value;
//}

PROPERTIES properties_create(void) {
	return hashMap_create(string_hash, string_hash, string_equals, string_equals);
}

void properties_destroy(PROPERTIES properties) {
	HASH_MAP_ITERATOR iter = hashMapIterator_create(properties);
	while (hashMapIterator_hasNext(iter)) {
		HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iter);
		free(hashMapEntry_getKey(entry));
		free(hashMapEntry_getValue(entry));
	}
	hashMapIterator_destroy(iter);
	hashMap_destroy(properties, false, false);
}

PROPERTIES properties_load(char * filename) {
	PROPERTIES props = properties_create();
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
					key = string_trim(string_ndup((char *)line, pos));
					ival = string_ndup(line+pos+1, strlen(line));
					value = string_trim(ival);
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
					strcat(value, string_trim(line));
					old = hashMap_put(props, strdup(key), strdup(value));
				}
			}
		}
		fclose(file);
	}
	free(cont);
	return props;
}

/**
 * Header is ignored for now, cannot handle comments yet
 */
void properties_store(PROPERTIES properties, char * filename, char * header) {
	FILE *file = fopen ( filename, "w+" );
	if (file != NULL) {
		if (hashMap_size(properties) > 0) {
			HASH_MAP_ITERATOR iterator = hashMapIterator_create(properties);
			while (hashMapIterator_hasNext(iterator)) {
				HASH_MAP_ENTRY entry = hashMapIterator_nextEntry(iterator);

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

char * properties_get(PROPERTIES properties, char * key) {
	return hashMap_get(properties, key);
}

char * properties_getWithDefault(PROPERTIES properties, char * key, char * defaultValue) {
	char * value = properties_get(properties, key);
	return value == NULL ? defaultValue : value;
}

char * properties_set(PROPERTIES properties, char * key, char * value) {
	return hashMap_put(properties, strdup(key), strdup(value));
}
