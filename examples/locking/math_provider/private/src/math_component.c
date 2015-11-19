/*
 * math.c
 *
 *  Created on: Oct 8, 2014
 *      Author: dl436
 */

#include <stdlib.h>
#include "math_component.h"

struct math_component {
	//emtpy
};

celix_status_t mathComponent_create(math_component_pt *math) {
	(*math) = calloc(1, sizeof(struct math_component));
	return CELIX_SUCCESS;
}

celix_status_t mathComponent_destroy(math_component_pt math) {
	free(math);
	return CELIX_SUCCESS;
}

int mathComponent_calc(math_component_pt math __attribute__((unused)), int arg1, int arg2) {
	return arg1 * arg2 + arg2;
}


