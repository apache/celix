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
#include "hashtable_itr.h"
#include "utils.h"

DEFINE_HASHTABLE_INSERT(prop_insert, struct key, struct value);
DEFINE_HASHTABLE_REMOVE(prop_remove, struct key, struct value);
DEFINE_HASHTABLE_SEARCH(prop_search, struct key, struct value);

static unsigned int hashfromkey(void * ks) {
	struct key *k = (struct key *) ks;
	unsigned long hash = 5381;
	int i;
	char * ck = strdup(k->key);
	for (i=0; i<strlen(ck); ++i) hash = 33*hash + ck[i];
	//int c;
	//while (c = *ck++) {
	//	hash = ((hash << 5) + hash) + c;
	//}
	free(ck);
	return hash;
}

static int equalskey(void * k1, void * k2) {
	struct key * key1 = (struct key *) k1;
	struct key * key2 = (struct key *) k2;
	return (strcmp(key1->key, key2->key) == 0);
}

char * addEntry(HASHTABLE properties, char * k, char * v);
char * addEntry(HASHTABLE properties, char * k, char * v) {
	struct key * search_key = (struct key *) malloc(sizeof(*search_key));
	struct key * s_key = (struct key *) malloc(sizeof(*s_key));
	struct value * s_value = (struct value *) malloc(sizeof(*s_value));
	struct value * oldValue = NULL;
	s_key->key = k;
	search_key->key = k;
	s_value->value = v;
	oldValue = prop_search(properties, search_key);
	if (oldValue != NULL) {
		prop_remove(properties, search_key);
	}
	free(search_key);
	prop_insert(properties, s_key, s_value);

	return oldValue == NULL ? NULL : oldValue->value;
}

HASHTABLE createProperties(void) {
	return create_hashtable(1, hashfromkey, equalskey);
}

HASHTABLE loadProperties(char * filename) {
	HASHTABLE props = create_hashtable(1, hashfromkey, equalskey);
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
					key = string_trim(string_ndup((char *)line, pos));
					value = string_trim(string_ndup(line+pos+1, strlen(line)));
					if (value != NULL) {
						if (strcmp(cont, string_ndup(value+strlen(value)-1, 1)) == 0) {
							split = 1;
							value = string_ndup(value, strlen(value)-1);
						} else {
							char * old = addEntry(props, key, value);
						}
					}
				}
			} else {
				if (strcmp(cont, string_ndup(line+strlen(line)-1, 1)) == 0) {
					split = 1;
					strcat(value, string_ndup(line, strlen(line)-1));
				} else {
					split = 0;
					strcat(value, string_trim(line));
					char * old = addEntry(props, key, value);
				}
			}
		}
		fclose(file);
		return props;
	}
	free(cont);
	return NULL;
}

/**
 * Header is ignored for now, cannot handle comments yet
 */
void storeProperties(HASHTABLE properties, char * filename, char * header) {
	FILE *file = fopen ( filename, "w+" );
	if (file != NULL) {
		if (hashtable_count(properties) > 0)
		{
			struct hashtable_itr * itr = hashtable_iterator(properties);
			do {
				char * line = NULL;
				struct key * k = hashtable_iterator_key(itr);
				struct value * v = hashtable_iterator_value(itr);

				line = strdup(k->key);
				strcat(line, "=");
				strcat(line, strdup(v->value));
				strcat(line, "\n");
				fputs(line, file);
			} while (hashtable_iterator_advance(itr));
			free(itr);
		}
		fclose(file);
	} else {
		perror("File is null");
	}
}

char * getProperty(HASHTABLE properties, char * key) {
	struct key * s_key = (struct key *) malloc(sizeof(struct key));
	s_key->key = key;
	struct value * value = prop_search(properties, s_key);
	free(s_key);
	return value == NULL ? NULL : value->value;
}

char * getPropertyWithDefault(HASHTABLE properties, char * key, char * defaultValue) {
	char * value = getProperty(properties, key);
	return value == NULL ? defaultValue : value;
}

char * setProperty(HASHTABLE properties, char * key, char * value) {
	return addEntry(properties, key, value);
}
