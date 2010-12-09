/*
 * attribute.c
 *
 *  Created on: Jul 27, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "attribute.h"

ATTRIBUTE attribute_create(char * key, char * value) {
	ATTRIBUTE attribute = malloc(sizeof(*attribute));
	attribute->key = key;
	attribute->value = value;
	return attribute;
}
