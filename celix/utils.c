/*
 * utils.c
 *
 *  Created on: Jul 27, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <string.h>

#include "utils.h"

unsigned int string_hash(void * string) {
	char * str = (char *) string;

	unsigned int hash = 1315423911;
	unsigned int i    = 0;
	int len = strlen(str);

	for(i = 0; i < len; str++, i++)
	{
	  hash ^= ((hash << 5) + (*str) + (hash >> 2));
	}

	return hash;
}

int string_equals(void * string, void * toCompare) {
	return strcmp((char *)string, (char *) toCompare) == 0;
}

char * string_ndup(const char *s, size_t n) {
	size_t len = strlen(s);
	char *ret;

	if (len <= n) {
		return strdup(s);
	}

	ret = malloc(n + 1);
	strncpy(ret, s, n);
	ret[n] = '\0';
	return ret;
}

char * string_trim(char * string) {
	char *end;
	// Trim leading space
	while(isspace(*string)) string++;

	// Trim trailing space
	end = string + strlen(string) - 1;
	while(end > string && isspace(*end)) end--;

	// Write new null terminator
	*(end+1) = 0;

	return string;
}
