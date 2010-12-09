/*
 * utils.h
 *
 *  Created on: Jul 27, 2010
 *      Author: alexanderb
 */

#ifndef UTILS_H_
#define UTILS_H_

#include <ctype.h>

unsigned int string_hash(void * string);
int string_equals(void * string, void * toCompare);
char * string_ndup(const char *s, size_t n);
char * string_trim(char * string);

#endif /* UTILS_H_ */
