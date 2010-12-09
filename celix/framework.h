/*
 * framework.h
 *
 *  Created on: Mar 23, 2010
 *      Author: alexanderb
 */

#ifndef FRAMEWORK_H_
#define FRAMEWORK_H_

#include "headers.h"
#include "manifest.h"
#include "wire.h"
#include "hash_map.h"
#include "array_list.h"

FRAMEWORK framework_create();
void fw_init(FRAMEWORK framework);

BUNDLE fw_installBundle(FRAMEWORK framework, char * location);

void fw_startBundle(FRAMEWORK framework, BUNDLE bundle, int options);
void fw_stopBundle(FRAMEWORK framework, BUNDLE bundle, int options);

SERVICE_REGISTRATION fw_registerService(FRAMEWORK framework, BUNDLE bundle, char * serviceName, void * svcObj, HASHTABLE properties);
void fw_unregisterService(SERVICE_REGISTRATION registration);

ARRAY_LIST fw_getServiceReferences(FRAMEWORK framework, BUNDLE bundle, char * serviceName, char * filter);
void * fw_getService(FRAMEWORK framework, BUNDLE bundle, SERVICE_REFERENCE reference);
bool framework_ungetService(FRAMEWORK framework, BUNDLE bundle, SERVICE_REFERENCE reference);

void fw_addServiceListener(BUNDLE bundle, SERVICE_LISTENER listener, char * filter);
void fw_removeServiceListener(BUNDLE bundle, SERVICE_LISTENER listener);
void fw_serviceChanged(SERVICE_EVENT event, HASHTABLE oldprops);

//BUNDLE_ARCHIVE fw_createArchive(long id, char * location);
//void revise(BUNDLE_ARCHIVE archive, char * location);
MANIFEST getManifest(BUNDLE_ARCHIVE archive);

BUNDLE findBundle(BUNDLE_CONTEXT context);
SERVICE_REGISTRATION findRegistration(SERVICE_REFERENCE reference);

SERVICE_REFERENCE listToArray(ARRAY_LIST list);
void framework_markResolvedModules(FRAMEWORK framework, HASH_MAP wires);

ARRAY_LIST framework_getBundles();

void framework_waitForStop();

ARRAY_LIST framework_getBundles(FRAMEWORK framework);
BUNDLE framework_getBundle(FRAMEWORK framework, char * location);
BUNDLE framework_getBundleById(FRAMEWORK framework, long id);

#endif /* FRAMEWORK_H_ */
