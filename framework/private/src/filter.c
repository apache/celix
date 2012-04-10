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
 * filter.c
 *
 *  Created on: Apr 28, 2010
 *      Author: dk489
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "filter.h"
#include "array_list.h"

typedef enum operand
{
	EQUAL,
	APPROX,
	GREATER,
	LESS,
	PRESENT,
	SUBSTRING,
	AND,
	OR,
	NOT,
} OPERAND;

struct filter {
	OPERAND operand;
	char * attribute;
	int operands;
	void * value;
	char *filterStr;
};

void filter_skipWhiteSpace(char * filterString, int * pos);
FILTER filter_parseFilter(char * filterString, int * pos, apr_pool_t *pool);
FILTER filter_parseFilterComp(char * filterString, int * pos, apr_pool_t *pool);
FILTER filter_parseAnd(char * filterString, int * pos, apr_pool_t *pool);
FILTER filter_parseOr(char * filterString, int * pos, apr_pool_t *pool);
FILTER filter_parseNot(char * filterString, int * pos, apr_pool_t *pool);
FILTER filter_parseItem(char * filterString, int * pos, apr_pool_t *pool);
char * filter_parseAttr(char * filterString, int * pos);
char * filter_parseValue(char * filterString, int * pos);
ARRAY_LIST filter_parseSubstring(char * filterString, int * pos, apr_pool_t *pool);

int filter_compare(OPERAND operand, char * string, void * value2);
int filter_compareString(OPERAND operand, char * string, void * value2);

void filter_skipWhiteSpace(char * filterString, int * pos) {
	int length;
	for (length = strlen(filterString); (*pos < length) && isspace(filterString[*pos]);) {
		(*pos)++;
	}
}

FILTER filter_create(char * filterString, apr_pool_t *pool) {
	FILTER filter = NULL;
	int pos = 0;
	filter = filter_parseFilter(filterString, &pos, pool);
	if (pos != strlen(filterString)) {
		printf("Error: Extraneous trailing characters\n");
		return NULL;
	}
	filter->filterStr = filterString;
	return filter;
}

void filter_destroy(FILTER filter) {
	if (filter != NULL) {
		if (filter->operand == SUBSTRING) {
			arrayList_clear(filter->value);
			arrayList_destroy(filter->value);
			filter->value = NULL;
		} else if (filter->operand == OR) {
			int size = arrayList_size(filter->value);
			int i = 0;
			for (i = 0; i < size; i++) {
				FILTER f = arrayList_get(filter->value, i);
				filter_destroy(f);
			}
			arrayList_destroy(filter->value);
			filter->value = NULL;
		} else {
			free(filter->value);
			filter->value = NULL;
		}
		free(filter->attribute);
		filter->attribute = NULL;
		free(filter);
		filter = NULL;
	}
}

FILTER filter_parseFilter(char * filterString, int * pos, apr_pool_t *pool) {
	FILTER filter;
	filter_skipWhiteSpace(filterString, pos);
	if (filterString[*pos] != '(') {
		printf("Error: Missing '('\n");
		return NULL;
	}
	(*pos)++;

	filter = filter_parseFilterComp(filterString, pos, pool);

	filter_skipWhiteSpace(filterString, pos);

	if (filterString[*pos] != ')') {
		printf("Error: Missing ')'\n");
		return NULL;
	}
	(*pos)++;
	filter_skipWhiteSpace(filterString, pos);

	return filter;
}

FILTER filter_parseFilterComp(char * filterString, int * pos, apr_pool_t *pool) {
	char c;
	filter_skipWhiteSpace(filterString, pos);

	c = filterString[*pos];

	switch (c) {
		case '&': {
			(*pos)++;
			return filter_parseAnd(filterString, pos, pool);
		}
		case '|': {
			(*pos)++;
			return filter_parseOr(filterString, pos, pool);
		}
		case '!': {
			(*pos)++;
			return filter_parseNot(filterString, pos, pool);
		}
	}
	return filter_parseItem(filterString, pos, pool);
}

FILTER filter_parseAnd(char * filterString, int * pos, apr_pool_t *pool) {
	FILTER filter = (FILTER) malloc(sizeof(*filter));
	ARRAY_LIST operands = NULL;
	arrayList_create(pool, &operands);
	filter_skipWhiteSpace(filterString, pos);

	if (filterString[*pos] != '(') {
		printf("Error: Missing '('\n");
		return NULL;
	}

	while(filterString[*pos] == '(') {
		FILTER child = filter_parseFilter(filterString, pos, pool);
		arrayList_add(operands, child);
	}

	filter->operand = AND;
	filter->attribute = NULL;
	filter->value = operands;

	return filter;
}

FILTER filter_parseOr(char * filterString, int * pos, apr_pool_t *pool) {
	FILTER filter = (FILTER) malloc(sizeof(*filter));
	ARRAY_LIST operands = NULL;
	arrayList_create(pool, &operands);
	filter_skipWhiteSpace(filterString, pos);

	if (filterString[*pos] != '(') {
		printf("Error: Missing '('\n");
		return NULL;
	}

	while(filterString[*pos] == '(') {
		FILTER child = filter_parseFilter(filterString, pos, pool);
		arrayList_add(operands, child);
	}

	filter->operand = OR;
	filter->attribute = NULL;
	filter->value = operands;

	return filter;
}

FILTER filter_parseNot(char * filterString, int * pos, apr_pool_t *pool) {
	FILTER child = NULL;
	FILTER filter = (FILTER) malloc(sizeof(*filter));
	filter_skipWhiteSpace(filterString, pos);

	if (filterString[*pos] != '(') {
		printf("Error: Missing '('\n");
		return NULL;
	}

	child = filter_parseFilter(filterString, pos, pool);

	filter->operand = NOT;
	filter->attribute = NULL;
	filter->value = child;

	return filter;
}

FILTER filter_parseItem(char * filterString, int * pos, apr_pool_t *pool) {
	char * attr = filter_parseAttr(filterString, pos);
	filter_skipWhiteSpace(filterString, pos);
	switch(filterString[*pos]) {
		case '~': {
			if (filterString[*pos + 1] == '=') {
				FILTER filter = (FILTER) malloc(sizeof(*filter));
				*pos += 2;
				filter->operand = APPROX;
				filter->attribute = attr;
				filter->value = filter_parseValue(filterString, pos);
				return filter;
			}
			break;
		}
		case '>': {
			if (filterString[*pos + 1] == '=') {
				FILTER filter = (FILTER) malloc(sizeof(*filter));
				*pos += 2;
				filter->operand = GREATER;
				filter->attribute = attr;
				filter->value = filter_parseValue(filterString, pos);
				return filter;
			}
			break;
		}
		case '<': {
			if (filterString[*pos + 1] == '=') {
				FILTER filter = (FILTER) malloc(sizeof(*filter));
				*pos += 2;
				filter->operand = LESS;
				filter->attribute = attr;
				filter->value = filter_parseValue(filterString, pos);
				return filter;
			}
			break;
		}
		case '=': {
			FILTER filter = NULL;
			ARRAY_LIST subs;
			if (filterString[*pos + 1] == '*') {
				int oldPos = *pos;
				*pos += 2;
				filter_skipWhiteSpace(filterString, pos);
				if (filterString[*pos] == ')') {
					FILTER filter = (FILTER) malloc(sizeof(*filter));
					filter->operand = PRESENT;
					filter->attribute = attr;
					filter->value = NULL;
					return filter;
				}
				*pos = oldPos;
			}
			filter = (FILTER) malloc(sizeof(*filter));			
			(*pos)++;
			subs = filter_parseSubstring(filterString, pos, pool);
			if (arrayList_size(subs) == 1) {
				char * string = (char *) arrayList_get(subs, 0);
				if (string != NULL) {
					filter->operand = EQUAL;
					filter->attribute = attr;
					filter->value = string;

					arrayList_clear(subs);
					arrayList_destroy(subs);

					return filter;
				}
			}
			filter->operand = SUBSTRING;
			filter->attribute = attr;
			filter->value = subs;
			return filter;
		}
	}
	printf("Invalid operator\n");
	return NULL;
}

char * filter_parseAttr(char * filterString, int * pos) {
	char c;
	int begin = *pos;
	int end = *pos;
	int length = 0;

	filter_skipWhiteSpace(filterString, pos);
	c = filterString[*pos];

	while (c != '~' && c != '<' && c != '>' && c != '=' && c != '(' && c != ')') {
		(*pos)++;

		if (!isspace(c)) {
			end = *pos;
		}

		c = filterString[*pos];
	}

	length = end - begin;

	if (length == 0) {
		printf("Missing attr");
		return NULL;
	} else {
		char * attr = (char *) malloc(length+1);
		strncpy(attr, filterString+begin, length);
		attr[length] = '\0';
		return attr;
	}
}

char * filter_parseValue(char * filterString, int * pos) {
	char * value = strdup("");
	int keepRunning = 1;

	while (keepRunning) {
		char c = filterString[*pos];

		switch (c) {
			case ')': {
				keepRunning = 0;
				break;
			}
			case '(': {
				printf("Invalid value");
				return NULL;
			}
			case '\\': {
				(*pos)++;
				c = filterString[*pos];
			}
			/* no break */
			default: {
				char ch[2];
				ch[0] = c;
				ch[1] = '\0';
				strcat(value, ch);
				(*pos)++;
				break;
			}
		}
	}

	if (strlen(value) == 0) {
		printf("Missing value");
		return NULL;
	}
	return value;
}

ARRAY_LIST filter_parseSubstring(char * filterString, int * pos, apr_pool_t *pool) {
	char * sub = (char *) malloc(strlen(filterString));
	ARRAY_LIST operands = NULL;
	int keepRunning = 1;
	int size;

	arrayList_create(pool, &operands);
	sub[0] = '\0';
	while (keepRunning) {
		char c = filterString[*pos];
		

		switch (c) {
			case ')': {
				if (strlen(sub) > 0) {
					arrayList_add(operands, strdup(sub));
				}
				keepRunning = 0;
				break;
			}
			case '(': {
				printf("Invalid value");
				return NULL;
			}
			case '*': {
				if (strlen(sub) > 0) {
					arrayList_add(operands, strdup(sub));
				}
				sub[0] = '\0';
				arrayList_add(operands, NULL);
				(*pos)++;
				break;
			}
			case '\\': {
				(*pos)++;
				c = filterString[*pos];
			}
			/* no break */
			default: {
				char ch[2];
				ch[0] = c;
				ch[1] = '\0';
				strcat(sub, ch);
				(*pos)++;
				break;
			}
		}
	}
	free(sub);
	size = arrayList_size(operands);

	if (size == 0) {
		printf("Missing value");
		return NULL;
	}

	return operands;
}

int filter_match(FILTER filter, PROPERTIES properties) {
	switch (filter->operand) {
		case AND: {
			ARRAY_LIST filters = (ARRAY_LIST) filter->value;
			unsigned int i;
			for (i = 0; i < arrayList_size(filters); i++) {
				FILTER sfilter = (FILTER) arrayList_get(filters, i);
				if (!filter_match(sfilter, properties)) {
					return 0;
				}
			}
			return 1;
		}
		case OR: {
			ARRAY_LIST filters = (ARRAY_LIST) filter->value;
			unsigned int i;
			for (i = 0; i < arrayList_size(filters); i++) {
				FILTER sfilter = (FILTER) arrayList_get(filters, i);
				if (filter_match(sfilter, properties)) {
					return 1;
				}
			}
			return 0;
		}
		case NOT: {
			FILTER sfilter = (FILTER) filter->value;
			return !filter_match(sfilter, properties);
		}
		case SUBSTRING :
		case EQUAL :
		case GREATER :
		case LESS :
		case APPROX : {
			char * value = (properties == NULL) ? NULL: properties_get(properties, filter->attribute);

			return filter_compare(filter->operand, (char *) value, filter->value);
		}
		case PRESENT: {
			char * value = (properties == NULL) ? NULL: properties_get(properties, filter->attribute);
			return value != NULL;
		}
	}
	return 0;
}

int filter_compare(OPERAND operand, char * string, void * value2) {
	if (string == NULL) {
		return 0;
	}
	return filter_compareString(operand, string, value2);

}

int filter_compareString(OPERAND operand, char * string, void * value2) {
	switch (operand) {
		case SUBSTRING: {
			ARRAY_LIST subs = (ARRAY_LIST) value2;
			int pos = 0;
			int i;
			int size = arrayList_size(subs);
			for (i = 0; i < size; i++) {
				char * substr = (char *) arrayList_get(subs, i);

				if (i + 1 < size) {
					if (substr == NULL) {
						unsigned int index;
						char * substr2 = (char *) arrayList_get(subs, i + 1);
						if (substr2 == NULL) {
							continue;
						}
						index = strcspn(string+pos, substr2);
						if (index == strlen(string+pos)) {
							return 0;
						}

						pos = index + strlen(substr2);
						if (i + 2 < size) {
							i++;
						}
					} else {
						unsigned int len = strlen(substr);
						char * region = (char *)malloc(len+1);
						strncpy(region, string+pos, len);
						region[len]	= '\0';
						if (strcmp(region, substr) == 0) {
							pos += len;
						} else {
							free(region);
							return 0;
						}
						free(region);
					}
				} else {
					unsigned int len;
					int begin;

					if (substr == NULL) {
						return 1;
					}
					len = strlen(substr);
					begin = strlen(string)-len;
					return (strcmp(string+begin, substr) == 0);
				}
			}
			return 0;
		}
		case APPROX:
		case EQUAL: {
			return (strcmp(string, (char *) value2) == 0);
		}
		case GREATER: {
			return (strcmp(string, (char *) value2) >= 0);
		}
		case LESS: {
			return (strcmp(string, (char *) value2) <= 0);
		}
		case AND:
		case NOT:
		case OR:
		case PRESENT: {
		}
		/* no break */
	}
	return 0;
}

celix_status_t filter_getString(FILTER filter, char **filterStr) {
	if (filter != NULL) {
		*filterStr = filter->filterStr;
	}
	return CELIX_SUCCESS;
}
