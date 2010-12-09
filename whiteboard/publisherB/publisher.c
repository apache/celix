/*
 * publisher.c
 *
 *  Created on: Aug 23, 2010
 *      Author: alexanderb
 */
#include <stdio.h>

#include "publisher_private.h"

void publisher_invoke(PUBLISHER publisher, char * text) {
	printf("Hello from B %s\n", text);
}

