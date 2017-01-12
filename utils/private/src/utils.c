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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "utils.h"

unsigned int utils_stringHash(const void* strPtr) {
    const char* string = strPtr;
    unsigned int hc = 5381;
    char ch;
    while((ch = *string++) != '\0'){
        hc = (hc << 5) + hc + ch;
    }

    return hc;
}

int utils_stringEquals(const void* string, const void* toCompare) {
	return strcmp((const char*)string, (const char*)toCompare) == 0;
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
	char* copy = string;

	char *end;
	// Trim leading space
	while (isspace(*copy)) {
		copy++;
	}

	// Trim trailing space
	end = copy + strlen(copy) - 1;
	while(end > copy && isspace(*end)) {
		*(end) = '\0';
		end--;
	}

	if (copy != string) { 
		//beginning whitespaces -> move char in copy to to begin string
		//This to ensure free still works on the same pointer.
		char* nstring = string;
		while(*copy != '\0') {
			*(nstring++) = *(copy++);
		}
		(*nstring) = '\0';
	}

	return string;
}

bool utils_isStringEmptyOrNull(const char * const str) {
	bool empty = true;
	if (str != NULL) {
		int i;
		for (i = 0; i < strnlen(str, 1024 * 1024); i += 1) {
			if (!isspace(str[i])) {
				empty = false;
				break;
			}
		}
	}

	return empty;
}

celix_status_t thread_equalsSelf(celix_thread_t thread, bool *equals) {
	celix_status_t status = CELIX_SUCCESS;

	celix_thread_t self = celixThread_self();
	if (status == CELIX_SUCCESS) {
		*equals = celixThread_equals(self, thread);
	}

	return status;
}

celix_status_t utils_isNumeric(const char *number, bool *ret) {
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


int utils_compareServiceIdsAndRanking(unsigned long servId, long servRank, unsigned long otherServId, long otherServRank) {
	int result;

	if (servId == otherServId) {
		result = 0;
	} else if (servRank != otherServRank) {
		result = servRank < otherServRank ? -1 : 1;
	} else { //equal service rank, compare service ids
		result = servId < otherServId ? 1 : -1;
	}

	return result;
}
