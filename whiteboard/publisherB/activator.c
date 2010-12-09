/*
 * activator.c
 *
 *  Created on: Aug 23, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "bundle_activator.h"
#include "bundle_context.h"
#include "publisher_private.h"

void * bundleActivator_create() {
	return NULL;
}

void bundleActivator_start(void * userData, BUNDLE_CONTEXT context) {
	PUBLISHER_SERVICE ps = malloc(sizeof(*ps));
	PUBLISHER pub = malloc(sizeof(*pub));
	ps->invoke = publisher_invoke;
	ps->publisher = pub;

	bundleContext_registerService(context, PUBLISHER_NAME, ps, NULL);
}

void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context) {

}

void bundleActivator_destroy(void * userData) {

}
