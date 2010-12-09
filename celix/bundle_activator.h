/*
 * activator.h
 *
 *  Created on: Mar 18, 2010
 *      Author: alexanderb
 */

#ifndef BUNDLE_ACTIVATOR_H_
#define BUNDLE_ACTIVATOR_H_

#include "headers.h"

void * bundleActivator_create();
void bundleActivator_start(void * userData, BUNDLE_CONTEXT context);
void bundleActivator_stop(void * userData, BUNDLE_CONTEXT context);
void bundleActivator_destroy(void * userData);

#endif /* BUNDLE_ACTIVATOR_H_ */
