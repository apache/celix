/*
 * activator.c
 *
 *  Created on: Aug 20, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <stdio.h>

#include "bundle_activator.h"

struct userData {
	char * word;
};

void * bundleActivator_create() {
	struct userData * data = malloc(sizeof(*data));
	data->word = "World";
	return data;
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	struct userData * data = (struct userData *) userData;
	printf("Hello %s\n", data->word);

}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {
	struct userData * data = (struct userData *) userData;
	printf("Goodbye %s\n", data->word);
}

void bundleActivator_destroy(void * userData) {

}
