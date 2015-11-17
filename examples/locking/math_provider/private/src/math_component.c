/*
 * math.c
 *
 *  Created on: Oct 8, 2014
 *      Author: dl436
 */

#include "math_component.h"

struct math_component {
	//emtpy
};

celix_status_t mathComponent_create(math_component_pt *math) {
	(*math) = malloc(sizeof(struct math_component));
	return CELIX_SUCCESS;
}

celix_status_t mathComponent_destroy(math_component_pt math) {
	free(math);
	return CELIX_SUCCESS;
}

int mathComponent_calc(math_component_pt math, int arg1, int arg2) {
	return arg1 * arg2 + arg2;
}


