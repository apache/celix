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
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <ctype.h>
#include <assert.h>
#include <utils.h>

#include "celix_log.h"
#include "filter.h"

static void filter_skipWhiteSpace(char* filterString, int* pos);
static celix_filter_t * filter_parseFilter(char* filterString, int* pos);
static celix_filter_t * filter_parseFilterComp(char* filterString, int* pos);
static celix_filter_t * filter_parseAndOrOr(char* filterString, celix_filter_operand_t andOrOr, int* pos);
static celix_filter_t * filter_parseNot(char* filterString, int* pos);
static celix_filter_t * filter_parseItem(char* filterString, int* pos);
static char * filter_parseAttr(char* filterString, int* pos);
static char * filter_parseValue(char* filterString, int* pos);
static array_list_pt filter_parseSubstring(char* filterString, int* pos);

static celix_status_t filter_compare(celix_filter_t* filter, const char *propertyValue, bool *result);

static void filter_skipWhiteSpace(char * filterString, int * pos) {
	int length;
	for (length = strlen(filterString); (*pos < length) && isspace(filterString[*pos]);) {
		(*pos)++;
	}
}

celix_filter_t * filter_create(const char* filterString) {
	celix_filter_t * filter = NULL;
	char* filterStr = string_ndup(filterString, 1024*1024);
	int pos = 0;
	filter = filter_parseFilter(filterStr, &pos);
	if (filter != NULL && pos != strlen(filterStr)) {
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR,  "Error: Extraneous trailing characters.");
		filter_destroy(filter);
		filter = NULL;
	} else if (filter != NULL) {
        if (filter->operand != CELIX_FILTER_OPERAND_OR && filter->operand != CELIX_FILTER_OPERAND_AND &&
            filter->operand != CELIX_FILTER_OPERAND_NOT && filter->operand != CELIX_FILTER_OPERAND_SUBSTRING &&
            filter->operand != CELIX_FILTER_OPERAND_PRESENT) {
            if (filter->attribute == NULL || filter->value == NULL) {
                filter_destroy(filter);
                filter = NULL;
            }
        }
    }

	if (filter == NULL) {
		free(filterStr);
	} else {
		filter->filterStr = filterStr;
	}

	return filter;
}

void filter_destroy(celix_filter_t * filter) {
	if (filter != NULL) {
		if(filter->children != NULL){
			if (filter->operand == CELIX_FILTER_OPERAND_SUBSTRING) {
				int size = arrayList_size(filter->children);
				int i = 0;
				for (i = 0; i < size; i++) {
					char *operand = arrayList_get(filter->children, i);
					free(operand);
				}
				arrayList_destroy(filter->children);
				filter->children = NULL;
			} else if (filter->operand == CELIX_FILTER_OPERAND_OR || filter->operand == CELIX_FILTER_OPERAND_AND || filter->operand == CELIX_FILTER_OPERAND_NOT) {
                int size = arrayList_size(filter->children);
                int i = 0;
                for (i = 0; i < size; i++) {
                    celix_filter_t *f = arrayList_get(filter->children, i);
                    filter_destroy(f);
                }
                arrayList_destroy(filter->children);
                filter->children = NULL;
            } else {
                fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR,  "Error: Corrupt filter. childern has a value, but not an expected operand");
            }
		}
        free((char*)filter->value);
        filter->value = NULL;
		free((char*)filter->attribute);
		filter->attribute = NULL;
		free(filter);
        free((char*)filter->filterStr);
        filter->filterStr = NULL;
	}
}

static celix_filter_t * filter_parseFilter(char * filterString, int * pos) {
	celix_filter_t * filter;
	filter_skipWhiteSpace(filterString, pos);
	if (filterString[*pos] != '(') {
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Error: Missing '(' in filter string '%s'.", filterString);
		return NULL;
	}
	(*pos)++;

	filter = filter_parseFilterComp(filterString, pos);

	filter_skipWhiteSpace(filterString, pos);

	if (filterString[*pos] != ')') {
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Error: Missing ')' in filter string '%s'.", filterString);
		if(filter!=NULL){
			filter_destroy(filter);
		}
		return NULL;
	}
	(*pos)++;
	filter_skipWhiteSpace(filterString, pos);

	return filter;
}

static celix_filter_t * filter_parseFilterComp(char * filterString, int * pos) {
	char c;
	filter_skipWhiteSpace(filterString, pos);

	c = filterString[*pos];

	switch (c) {
		case '&': {
			(*pos)++;
			return filter_parseAndOrOr(filterString, CELIX_FILTER_OPERAND_AND, pos);
		}
		case '|': {
			(*pos)++;
			return filter_parseAndOrOr(filterString, CELIX_FILTER_OPERAND_OR, pos);
		}
		case '!': {
			(*pos)++;
			return filter_parseNot(filterString, pos);
		}
	}
	return filter_parseItem(filterString, pos);
}

static celix_filter_t * filter_parseAndOrOr(char * filterString, celix_filter_operand_t andOrOr, int * pos) {

	array_list_pt children = NULL;
	filter_skipWhiteSpace(filterString, pos);
	bool failure = false;

	if (filterString[*pos] != '(') {
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Error: Missing '('.");
		return NULL;
	}

	arrayList_create(&children);
	while(filterString[*pos] == '(') {
		celix_filter_t * child = filter_parseFilter(filterString, pos);
		if(child == NULL) {
			failure = true;
			break;
		}
		arrayList_add(children, child);
	}

	if(failure == true){
        int i;
		for (i = 0; i < arrayList_size(children); ++i) {
			celix_filter_t * f = arrayList_get(children, i);
			filter_destroy(f);
		}
		arrayList_destroy(children);
        children = NULL;
	}

	celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
	filter->operand = andOrOr;
    filter->children = children;

	return filter;
}

static celix_filter_t * filter_parseNot(char * filterString, int * pos) {
	celix_filter_t * child = NULL;
	filter_skipWhiteSpace(filterString, pos);

	if (filterString[*pos] != '(') {
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Error: Missing '('.");
		return NULL;
	}

	child = filter_parseFilter(filterString, pos);

    array_list_t* children = NULL;
    arrayList_create(&children);
    arrayList_add(children, child);

	celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
	filter->operand = CELIX_FILTER_OPERAND_NOT;
	filter->children = children;

	return filter;
}

static celix_filter_t * filter_parseItem(char * filterString, int * pos) {
	char * attr = filter_parseAttr(filterString, pos);
	if(attr == NULL){
		return NULL;
	}

	filter_skipWhiteSpace(filterString, pos);
	switch(filterString[*pos]) {
		case '~': {
			if (filterString[*pos + 1] == '=') {
				celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
				*pos += 2;
				filter->operand = CELIX_FILTER_OPERAND_APPROX;
				filter->attribute = attr;
				filter->value = filter_parseValue(filterString, pos);
				return filter;
			}
			break;
		}
		case '>': {
			if (filterString[*pos + 1] == '=') {
				celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
				*pos += 2;
				filter->operand = CELIX_FILTER_OPERAND_GREATEREQUAL;
				filter->attribute = attr;
				filter->value = filter_parseValue(filterString, pos);
				return filter;
			}
			else {
                celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
                *pos += 1;
                filter->operand = CELIX_FILTER_OPERAND_GREATER;
                filter->attribute = attr;
                filter->value = filter_parseValue(filterString, pos);
                return filter;
			}
			break;
		}
		case '<': {
			if (filterString[*pos + 1] == '=') {
				celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
				*pos += 2;
				filter->operand = CELIX_FILTER_OPERAND_LESSEQUAL;
				filter->attribute = attr;
				filter->value = filter_parseValue(filterString, pos);
				return filter;
			}
			else {
                celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
                *pos += 1;
                filter->operand = CELIX_FILTER_OPERAND_LESS;
                filter->attribute = attr;
                filter->value = filter_parseValue(filterString, pos);
                return filter;
			}
			break;
		}
		case '=': {
			celix_filter_t * filter = NULL;
			array_list_pt subs;
			if (filterString[*pos + 1] == '*') {
				int oldPos = *pos;
				*pos += 2;
				filter_skipWhiteSpace(filterString, pos);
				if (filterString[*pos] == ')') {
					celix_filter_t * filter = (celix_filter_t *) calloc(1, sizeof(*filter));
					filter->operand = CELIX_FILTER_OPERAND_PRESENT;
					filter->attribute = attr;
					filter->value = NULL;
					return filter;
				}
				*pos = oldPos;
			}
			filter = (celix_filter_t *) calloc(1, sizeof(*filter));			
			(*pos)++;
			subs = filter_parseSubstring(filterString, pos);
			if(subs!=NULL){
				if (arrayList_size(subs) == 1) {
					char * string = (char *) arrayList_get(subs, 0);
					if (string != NULL) {
						filter->operand = CELIX_FILTER_OPERAND_EQUAL;
						filter->attribute = attr;
						filter->value = string;

						arrayList_clear(subs);
						arrayList_destroy(subs);

						return filter;
					}
				}
			}
			filter->operand = CELIX_FILTER_OPERAND_SUBSTRING;
			filter->attribute = attr;
            filter->children = subs;
			return filter;
		}
	}
	fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Invalid operator.");
	free(attr);
	return NULL;
}

static char * filter_parseAttr(char * filterString, int * pos) {
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
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Missing attr.");
		return NULL;
	} else {
		char * attr = (char *) calloc(1, length+1);
		strncpy(attr, filterString+begin, length);
		attr[length] = '\0';
		return attr;
	}
}

static char * filter_parseValue(char * filterString, int * pos) {
	char *value = calloc(strlen(filterString) + 1, sizeof(*value));
	int keepRunning = 1;

	while (keepRunning) {
		char c = filterString[*pos];

		switch (c) {
			case ')': {
				keepRunning = 0;
				break;
			}
			case '(': {
				fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Invalid value.");
				free(value);
				return NULL;
			}
			case '\0':{
				fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Unclosed bracket.");
				free(value);
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
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Missing value.");
		free(value);
		return NULL;
	}
	return value;
}

static array_list_pt filter_parseSubstring(char * filterString, int * pos) {
	char *sub = calloc(strlen(filterString) + 1, sizeof(*sub));
	array_list_pt operands = NULL;
	int keepRunning = 1;

	arrayList_create(&operands);
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
			case '\0':{
				fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Unclosed bracket.");
				keepRunning = false;
				break;
			}
			case '(': {
				fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Invalid value.");
				keepRunning = false;
				break;
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

	if (arrayList_size(operands) == 0) {
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Missing value.");
		arrayList_destroy(operands);
		return NULL;
	}

	return operands;
}

celix_status_t filter_match(celix_filter_t * filter, properties_pt properties, bool *result) {
	switch (filter->operand) {
		case CELIX_FILTER_OPERAND_AND: {
			array_list_pt children = filter->children;
			unsigned int i;
			for (i = 0; i < arrayList_size(children); i++) {
				celix_filter_t * sfilter = (celix_filter_t *) arrayList_get(children, i);
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
		case CELIX_FILTER_OPERAND_OR: {
			array_list_pt children = filter->children;
			unsigned int i;
			for (i = 0; i < arrayList_size(children); i++) {
				celix_filter_t * sfilter = (celix_filter_t *) arrayList_get(children, i);
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
		case CELIX_FILTER_OPERAND_NOT: {
			celix_filter_t * sfilter = arrayList_get(filter->children, 0);
			bool mresult;
			filter_match(sfilter, properties, &mresult);
			*result = !mresult;
			return CELIX_SUCCESS;
		}
		case CELIX_FILTER_OPERAND_SUBSTRING :
		case CELIX_FILTER_OPERAND_EQUAL :
		case CELIX_FILTER_OPERAND_GREATER :
        case CELIX_FILTER_OPERAND_GREATEREQUAL :
		case CELIX_FILTER_OPERAND_LESS :
        case CELIX_FILTER_OPERAND_LESSEQUAL :
		case CELIX_FILTER_OPERAND_APPROX : {
			char * value = (properties == NULL) ? NULL: (char*)properties_get(properties, filter->attribute);

			return filter_compare(filter, value, result);
		}
		case CELIX_FILTER_OPERAND_PRESENT: {
			char * value = (properties == NULL) ? NULL: (char*)properties_get(properties, filter->attribute);
			*result = value != NULL;
			return CELIX_SUCCESS;
		}
	}
	*result = 0;
	return CELIX_SUCCESS;
}

static celix_status_t filter_compare(celix_filter_t* filter, const char *propertyValue, bool *out) {
    celix_status_t  status = CELIX_SUCCESS;
    bool result = false;

    if (filter == NULL || propertyValue == NULL) {
        *out = false;
        return status;
    }

    switch (filter->operand) {
        case CELIX_FILTER_OPERAND_SUBSTRING: {
            int pos = 0;
            unsigned int i;
            int size = arrayList_size(filter->children);
            for (i = 0; i < size; i++) {
                char * substr = (char *) arrayList_get(filter->children, i);

                if (i + 1 < size) {
                    if (substr == NULL) {
                        unsigned int index;
                        char * substr2 = (char *) arrayList_get(filter->children, i + 1);
                        if (substr2 == NULL) {
                            continue;
                        }
                        index = strcspn(propertyValue+pos, substr2);
                        if (index == strlen(propertyValue+pos)) {
                            *out = false;
                            return CELIX_SUCCESS;
                        }

                        pos = index + strlen(substr2);
                        if (i + 2 < size) {
                            i++;
                        }
                    } else {
                        unsigned int len = strlen(substr);
                        char * region = (char *)calloc(1, len+1);
                        strncpy(region, propertyValue+pos, len);
                        region[len]	= '\0';
                        if (strcmp(region, substr) == 0) {
                            pos += len;
                        } else {
                            free(region);
                            *out = false;
                            return CELIX_SUCCESS;
                        }
                        free(region);
                    }
                } else {
                    unsigned int len;
                    int begin;

                    if (substr == NULL) {
                        *out = true;
                        return CELIX_SUCCESS;
                    }
                    len = strlen(substr);
                    begin = strlen(propertyValue)-len;
                    *out = (strcmp(propertyValue+begin, substr) == 0);
                    return CELIX_SUCCESS;
                }
            }
            *out = true;
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_APPROX: //TODO: Implement strcmp with ignorecase and ignorespaces
        case CELIX_FILTER_OPERAND_EQUAL: {
            *out = (strcmp(propertyValue, filter->value) == 0);
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_GREATER: {
            *out = (strcmp(propertyValue, filter->value) > 0);
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_GREATEREQUAL: {
            *out = (strcmp(propertyValue, filter->value) >= 0);
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_LESS: {
            *out = (strcmp(propertyValue, filter->value) < 0);
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_LESSEQUAL: {
            *out = (strcmp(propertyValue, filter->value) <= 0);
            return CELIX_SUCCESS;
        }
        case CELIX_FILTER_OPERAND_AND:
        case CELIX_FILTER_OPERAND_NOT:
        case CELIX_FILTER_OPERAND_OR:
        case CELIX_FILTER_OPERAND_PRESENT: {
        }
            /* no break */
    }

    if (status == CELIX_SUCCESS && out != NULL) {
        *out = result;
    }
    return status;
}

celix_status_t filter_getString(celix_filter_t * filter, const char **filterStr) {
	if (filter != NULL) {
		*filterStr = filter->filterStr;
	}
	return CELIX_SUCCESS;
}

celix_status_t filter_match_filter(celix_filter_t *src, celix_filter_t *dest, bool *out) {
    celix_status_t  status = CELIX_SUCCESS;
	bool result = false;

    if (src == dest) {
        result = true; //NOTE. also means NULL filter are equal
    } else if (src != NULL && dest != NULL && src->operand == dest->operand) {
        if (src->operand == CELIX_FILTER_OPERAND_AND || src->operand == CELIX_FILTER_OPERAND_OR || src->operand == CELIX_FILTER_OPERAND_NOT) {
            assert(src->children != NULL);
            assert(dest->children != NULL);
            int sizeSrc = arrayList_size(src->children);
            int sizeDest = arrayList_size(dest->children);
            if (sizeSrc == sizeDest) {
                int i;
                int k;
                int sameCount = 0;
                for (i =0; i < sizeSrc; ++i) {
                    bool same = false;
                    celix_filter_t *srcPart = arrayList_get(src->children, i);
                    for (k = 0; k < sizeDest; ++k) {
                        celix_filter_t *destPart = arrayList_get(dest->children, k);
                        filter_match_filter(srcPart, destPart, &same);
                        if (same) {
                            sameCount += 1;
                            break;
                        }
                    }
                }
                result = sameCount == sizeSrc;
            }
        } else { //compare attr and value
            bool attrSame = false;
            bool valSame = false;
            if (src->attribute == NULL && dest->attribute == NULL) {
                attrSame = true;
            } else if (src->attribute != NULL && dest->attribute != NULL) {
                attrSame = strncmp(src->attribute, dest->attribute, 1024 * 1024) == 0;
            }

            if (src->value == NULL  && dest->value == NULL) {
                valSame = true;
            } else if (src->value != NULL && dest->value != NULL) {
                valSame = strncmp(src->value, dest->value, 1024 * 1024) == 0;
            }

            result = attrSame && valSame;
        }
    }

    if (status == CELIX_SUCCESS && out != NULL) {
        *out = result;
    }

	return status;
}
