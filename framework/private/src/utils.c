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
	char * copy = string;

	char *end;
	// Trim leading space
	while(isspace(*copy)) copy++;

	// Trim trailing space
	end = copy + strlen(copy) - 1;
	while(end > copy && isspace(*end)) end--;

	// Write new null terminator
	*(end+1) = 0;

	return copy;
}
