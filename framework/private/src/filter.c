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
 *  \date       Apr 28, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>

#include "filter_private.h"
#include "array_list.h"

void filter_skipWhiteSpace(char * filterString, int * pos);
filter_pt filter_parseFilter(char * filterString, int * pos);
filter_pt filter_parseFilterComp(char * filterString, int * pos);
filter_pt filter_parseAnd(char * filterString, int * pos);
filter_pt filter_parseOr(char * filterString, int * pos);
filter_pt filter_parseNot(char * filterString, int * pos);
filter_pt filter_parseItem(char * filterString, int * pos);
char * filter_parseAttr(char * filterString, int * pos);
char * filter_parseValue(char * filterString, int * pos);
array_list_pt filter_parseSubstring(char * filterString, int * pos);

celix_status_t filter_compare(OPERAND operand, char * string, void * value2, bool *result);
celix_status_t filter_compareString(OPERAND operand, char * string, void * value2, bool *result);

void filter_skipWhiteSpace(char * filterString, int * pos) {
	int length;
	for (length = strlen(filterString); (*pos < length) && isspace(filterString[*pos]);) {
		(*pos)++;
	}
}

filter_pt filter_create(char * filterString) {
	filter_pt filter = NULL;
	int pos = 0;
	filter = filter_parseFilter(filterString, &pos);
	if (pos != strlen(filterString)) {
		printf("Error: Extraneous trailing characters\n");
		return NULL;
	}
	filter->filterStr = filterString;
	return filter;
}

void filter_destroy(filter_pt filter) {
	if (filter != NULL) {
		if (filter->operand == SUBSTRING) {
			arrayList_clear(filter->value);
			arrayList_destroy(filter->value);
			filter->value = NULL;
		} else if (filter->operand == OR) {
			int size = arrayList_size(filter->value);
			int i = 0;
			for (i = 0; i < size; i++) {
				filter_pt f = arrayList_get(filter->value, i);
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

filter_pt filter_parseFilter(char * filterString, int * pos) {
	filter_pt filter;
	filter_skipWhiteSpace(filterString, pos);
	if (filterString[*pos] != '(') {
		printf("Error: Missing '('\n");
		return NULL;
	}
	(*pos)++;

	filter = filter_parseFilterComp(filterString, pos);

	filter_skipWhiteSpace(filterString, pos);

	if (filterString[*pos] != ')') {
		printf("Error: Missing ')'\n");
		return NULL;
	}
	(*pos)++;
	filter_skipWhiteSpace(filterString, pos);

	return filter;
}

filter_pt filter_parseFilterComp(char * filterString, int * pos) {
	char c;
	filter_skipWhiteSpace(filterString, pos);

	c = filterString[*pos];

	switch (c) {
		case '&': {
			(*pos)++;
			return filter_parseAnd(filterString, pos);
		}
		case '|': {
			(*pos)++;
			return filter_parseOr(filterString, pos);
		}
		case '!': {
			(*pos)++;
			return filter_parseNot(filterString, pos);
		}
	}
	return filter_parseItem(filterString, pos);
}

filter_pt filter_parseAnd(char * filterString, int * pos) {
	filter_pt filter = (filter_pt) malloc(sizeof(*filter));
	array_list_pt operands = NULL;
	arrayList_create(&operands);
	filter_skipWhiteSpace(filterString, pos);

	if (filterString[*pos] != '(') {
		printf("Error: Missing '('\n");
		return NULL;
	}

	while(filterString[*pos] == '(') {
		filter_pt child = filter_parseFilter(filterString, pos);
		arrayList_add(operands, child);
	}

	filter->operand = AND;
	filter->attribute = NULL;
	filter->value = operands;

	return filter;
}

filter_pt filter_parseOr(char * filterString, int * pos) {
	filter_pt filter = (filter_pt) malloc(sizeof(*filter));
	array_list_pt operands = NULL;
	arrayList_create(&operands);
	filter_skipWhiteSpace(filterString, pos);

	if (filterString[*pos] != '(') {
		printf("Error: Missing '('\n");
		return NULL;
	}

	while(filterString[*pos] == '(') {
		filter_pt child = filter_parseFilter(filterString, pos);
		arrayList_add(operands, child);
	}

	filter->operand = OR;
	filter->attribute = NULL;
	filter->value = operands;

	return filter;
}

filter_pt filter_parseNot(char * filterString, int * pos) {
	filter_pt child = NULL;
	filter_pt filter = (filter_pt) malloc(sizeof(*filter));
	filter_skipWhiteSpace(filterString, pos);

	if (filterString[*pos] != '(') {
		printf("Error: Missing '('\n");
		return NULL;
	}

	child = filter_parseFilter(filterString, pos);

	filter->operand = NOT;
	filter->attribute = NULL;
	filter->value = child;

	return filter;
}

filter_pt filter_parseItem(char * filterString, int * pos) {
	char * attr = filter_parseAttr(filterString, pos);
	filter_skipWhiteSpace(filterString, pos);
	switch(filterString[*pos]) {
		case '~': {
			if (filterString[*pos + 1] == '=') {
				filter_pt filter = (filter_pt) malloc(sizeof(*filter));
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
				filter_pt filter = (filter_pt) malloc(sizeof(*filter));
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
				filter_pt filter = (filter_pt) malloc(sizeof(*filter));
				*pos += 2;
				filter->operand = LESS;
				filter->attribute = attr;
				filter->value = filter_parseValue(filterString, pos);
				return filter;
			}
			break;
		}
		case '=': {
			filter_pt filter = NULL;
			array_list_pt subs;
			if (filterString[*pos + 1] == '*') {
				int oldPos = *pos;
				*pos += 2;
				filter_skipWhiteSpace(filterString, pos);
				if (filterString[*pos] == ')') {
					filter_pt filter = (filter_pt) malloc(sizeof(*filter));
					filter->operand = PRESENT;
					filter->attribute = attr;
					filter->value = NULL;
					return filter;
				}
				*pos = oldPos;
			}
			filter = (filter_pt) malloc(sizeof(*filter));			
			(*pos)++;
			subs = filter_parseSubstring(filterString, pos);
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

array_list_pt filter_parseSubstring(char * filterString, int * pos) {
	char * sub = (char *) malloc(strlen(filterString));
	array_list_pt operands = NULL;
	int keepRunning = 1;
	int size;

	arrayList_create(&operands);
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

celix_status_t filter_match(filter_pt filter, properties_pt properties, bool *result) {
	switch (filter->operand) {
		case AND: {
			array_list_pt filters = (array_list_pt) filter->value;
			unsigned int i;
			for (i = 0; i < arrayList_size(filters); i++) {
				filter_pt sfilter = (filter_pt) arrayList_get(filters, i);
				bool mresult;
				filter_match(sfilter, properties, &mresult);
				if (!mresult) {
					*result = 0;
					return CELIX_SUCCESS;
				}
			}
			*result = 1;
			return CELIX_SUCCESS;
		}
		case OR: {
			array_list_pt filters = (array_list_pt) filter->value;
			unsigned int i;
			for (i = 0; i < arrayList_size(filters); i++) {
				filter_pt sfilter = (filter_pt) arrayList_get(filters, i);
				bool mresult;
				filter_match(sfilter, properties, &mresult);
				if (mresult) {
					*result = 1;
					return CELIX_SUCCESS;
				}
			}
			*result = 0;
			return CELIX_SUCCESS;
		}
		case NOT: {
			filter_pt sfilter = (filter_pt) filter->value;
			bool mresult;
			filter_match(sfilter, properties, &mresult);
			*result = !mresult;
			return CELIX_SUCCESS;
		}
		case SUBSTRING :
		case EQUAL :
		case GREATER :
		case LESS :
		case APPROX : {
			char * value = (properties == NULL) ? NULL: properties_get(properties, filter->attribute);

			return filter_compare(filter->operand, (char *) value, filter->value, result);
		}
		case PRESENT: {
			char * value = (properties == NULL) ? NULL: properties_get(properties, filter->attribute);
			*result = value != NULL;
			return CELIX_SUCCESS;
		}
	}
	*result = 0;
	return CELIX_SUCCESS;
}

celix_status_t filter_compare(OPERAND operand, char * string, void * value2, bool *result) {
	if (string == NULL) {
		*result = 0;
		return CELIX_SUCCESS;
	}
	return filter_compareString(operand, string, value2, result);

}

celix_status_t filter_compareString(OPERAND operand, char * string, void * value2, bool *result) {
	switch (operand) {
		case SUBSTRING: {
			array_list_pt subs = (array_list_pt) value2;
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
							*result = false;
							return CELIX_SUCCESS;
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
							*result = false;
							return CELIX_SUCCESS;
						}
						free(region);
					}
				} else {
					unsigned int len;
					int begin;

					if (substr == NULL) {
						*result = true;
						return CELIX_SUCCESS;
					}
					len = strlen(substr);
					begin = strlen(string)-len;
					*result = (strcmp(string+begin, substr) == 0);
					return CELIX_SUCCESS;
				}
			}
			*result = true;
			return CELIX_SUCCESS;
		}
		case APPROX:
		case EQUAL: {
			*result = (strcmp(string, (char *) value2) == 0);
			return CELIX_SUCCESS;
		}
		case GREATER: {
			*result = (strcmp(string, (char *) value2) >= 0);
			return CELIX_SUCCESS;
		}
		case LESS: {
			*result = (strcmp(string, (char *) value2) <= 0);
			return CELIX_SUCCESS;
		}
		case AND:
		case NOT:
		case OR:
		case PRESENT: {
		}
		/* no break */
	}
	*result = false;
	return CELIX_SUCCESS;
}

celix_status_t filter_getString(filter_pt filter, char **filterStr) {
	if (filter != NULL) {
		*filterStr = filter->filterStr;
	}
	return CELIX_SUCCESS;
}
