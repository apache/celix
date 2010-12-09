/*
 * bundle_context.h
 *
 *  Created on: Mar 26, 2010
 *      Author: alexanderb
 */

#ifndef BUNDLE_CONTEXT_H_
#define BUNDLE_CONTEXT_H_

#include "headers.h"

BUNDLE_CONTEXT bundleContext_create(FRAMEWORK framework, BUNDLE bundle);

FRAMEWORK bundleContext_getFramework(BUNDLE_CONTEXT context);

BUNDLE bundleContext_installBundle(BUNDLE_CONTEXT context, char * location);

SERVICE_REGISTRATION bundleContext_registerService(BUNDLE_CONTEXT context, char * serviceName, void * svcObj, HASHTABLE properties);

ARRAY_LIST getServiceReferences(BUNDLE_CONTEXT context, char * serviceName, char * filter);
SERVICE_REFERENCE bundleContext_getServiceReference(BUNDLE_CONTEXT context, char * serviceName);

void * bundleContext_getService(BUNDLE_CONTEXT context, SERVICE_REFERENCE reference);
bool bundleContext_ungetService(BUNDLE_CONTEXT context, SERVICE_REFERENCE reference);

ARRAY_LIST bundleContext_getBundles(BUNDLE_CONTEXT context);
BUNDLE bundleContext_getBundleById(BUNDLE_CONTEXT context, long id);

void addServiceListener(BUNDLE_CONTEXT context, SERVICE_LISTENER listener, char * filter);
void removeServiceListener(BUNDLE_CONTEXT context, SERVICE_LISTENER listener);


#endif /* BUNDLE_CONTEXT_H_ */
