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
 *  \date       Jul 27, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "utils.h"
#include "celix_log.h"
#include "celix_threads.h"

unsigned int utils_stringHash(void * string) {
	char * str = (char *) string;

	unsigned int hash = 1315423911;
	unsigned int i    = 0;
	unsigned int len = strlen(str);

	for(i = 0; i < len; str++, i++)
	{
	  hash ^= ((hash << 5) + (*str) + (hash >> 2));
	}

	return hash;
}

int utils_stringEquals(void * string, void * toCompare) {
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

char * utils_stringTrim(char * string) {
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

celix_status_t thread_equalsSelf(celix_thread_t thread, bool *equals) {
	celix_status_t status = CELIX_SUCCESS;

	celix_thread_t self = celixThread_self();
	if (status == CELIX_SUCCESS) {
		*equals = celixThread_equals(self, thread);
	}

	return status;
}

celix_status_t utils_isNumeric(char *number, bool *ret) {
	celix_status_t status = CELIX_SUCCESS;
	*ret = true;
	while(*number) {
		if(!isdigit(*number) && *number != '.') {
			*ret = false;
			break;
		}
		number++;
	}
	return status;
}
