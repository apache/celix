/**
 *Licensed to the Apache Software Foundation (ASF) under one
 *or more contributor license agreements.  See the NOTICE file
 *distributed with this work for additional information
 *regarding copyright ownership.  The ASF licenses this file
 *to you under the Apache License, Version 2.0 (the
 *"License"); you may not use this file except in compliance
 *with the License.  You may obtain a copy of the License at
 *
 *  http://www.apache.org/licenses/LICENSE-2.0
 *
 *Unless required by applicable law or agreed to in writing,
 *software distributed under the License is distributed on an
 *"AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 * KIND, either express or implied.  See the License for the
 *specific language governing permissions and limitations
 *under the License.
 */
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
#include "celix_errno.h"
#include "service_factory.h"

celix_status_t framework_create(FRAMEWORK *framework, apr_pool_t *memoryPool);
celix_status_t framework_destroy(FRAMEWORK framework);

celix_status_t fw_init(FRAMEWORK framework);
celix_status_t framework_start(FRAMEWORK framework);
void framework_stop(FRAMEWORK framework);

celix_status_t fw_installBundle(FRAMEWORK framework, BUNDLE * bundle, char * location);
celix_status_t fw_uninstallBundle(FRAMEWORK framework, BUNDLE bundle);

celix_status_t framework_getBundleEntry(FRAMEWORK framework, BUNDLE bundle, char *name, apr_pool_t *pool, char **entry);

celix_status_t fw_startBundle(FRAMEWORK framework, BUNDLE bundle, int options);
celix_status_t framework_updateBundle(FRAMEWORK framework, BUNDLE bundle, char *inputFile);
void fw_stopBundle(FRAMEWORK framework, BUNDLE bundle, bool record);

celix_status_t fw_registerService(FRAMEWORK framework, SERVICE_REGISTRATION * registration, BUNDLE bundle, char * serviceName, void * svcObj, PROPERTIES properties);
celix_status_t fw_registerServiceFactory(FRAMEWORK framework, SERVICE_REGISTRATION * registration, BUNDLE bundle, char * serviceName, service_factory_t factory, PROPERTIES properties);
void fw_unregisterService(SERVICE_REGISTRATION registration);

celix_status_t fw_getServiceReferences(FRAMEWORK framework, ARRAY_LIST *references, BUNDLE bundle, char * serviceName, char * filter);
void * fw_getService(FRAMEWORK framework, BUNDLE bundle, SERVICE_REFERENCE reference);
bool framework_ungetService(FRAMEWORK framework, BUNDLE bundle, SERVICE_REFERENCE reference);

void fw_addServiceListener(FRAMEWORK framework, BUNDLE bundle, SERVICE_LISTENER listener, char * filter);
void fw_removeServiceListener(FRAMEWORK framework, BUNDLE bundle, SERVICE_LISTENER listener);
void fw_serviceChanged(FRAMEWORK framework, SERVICE_EVENT event, PROPERTIES oldprops);

//BUNDLE_ARCHIVE fw_createArchive(long id, char * location);
//void revise(BUNDLE_ARCHIVE archive, char * location);
celix_status_t getManifest(BUNDLE_ARCHIVE archive, MANIFEST *manifest);

BUNDLE findBundle(BUNDLE_CONTEXT context);
SERVICE_REGISTRATION findRegistration(SERVICE_REFERENCE reference);

SERVICE_REFERENCE listToArray(ARRAY_LIST list);
celix_status_t framework_markResolvedModules(FRAMEWORK framework, HASH_MAP wires);

ARRAY_LIST framework_getBundles(void);

celix_status_t framework_waitForStop(void);

ARRAY_LIST framework_getBundles(FRAMEWORK framework);
BUNDLE framework_getBundle(FRAMEWORK framework, char * location);
BUNDLE framework_getBundleById(FRAMEWORK framework, long id);

#endif /* FRAMEWORK_H_ */
