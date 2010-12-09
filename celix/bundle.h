/*
 * bundle.h
 *
 *  Created on: Mar 23, 2010
 *      Author: alexanderb
 */

#ifndef BUNDLE_H_
#define BUNDLE_H_

#include "headers.h"

BUNDLE bundle_create();

BUNDLE bundle_createFromArchive(FRAMEWORK framework, BUNDLE_ARCHIVE archive);

BUNDLE_ARCHIVE bundle_getArchive(BUNDLE bundle);
MODULE bundle_getModule(BUNDLE bundle);
void * bundle_getHandle(BUNDLE bundle);
void bundle_setHandle(BUNDLE bundle, void * handle);
ACTIVATOR bundle_getActivator(BUNDLE bundle);
void bundle_setActivator(BUNDLE bundle, ACTIVATOR activator);
BUNDLE_CONTEXT bundle_getContext(BUNDLE bundle);
void bundle_setContext(BUNDLE bundle, BUNDLE_CONTEXT context);

void startBundle(BUNDLE bundle, int options);

void stopBundle(BUNDLE bundle, int options);

//void updateBundle(BUNDLE bundle, InputStream input);

void uninstallBundle(BUNDLE bundle);

// Service Reference Functions
ARRAY_LIST getUsingBundles(SERVICE_REFERENCE reference);

int compareTo(SERVICE_REFERENCE a, SERVICE_REFERENCE b);


BUNDLE_STATE bundle_getState(BUNDLE bundle);
bool bundle_isLockable(BUNDLE bundle);
pthread_t bundle_getLockingThread(BUNDLE bundle);
bool bundle_lock(BUNDLE bundle);
bool bundle_unlock(BUNDLE bundle);


#endif /* BUNDLE_H_ */
