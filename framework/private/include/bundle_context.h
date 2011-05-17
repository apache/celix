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
 * bundle_context.h
 *
 *  Created on: Mar 26, 2010
 *      Author: alexanderb
 */

#ifndef BUNDLE_CONTEXT_H_
#define BUNDLE_CONTEXT_H_

#include "headers.h"

celix_status_t bundleContext_create(FRAMEWORK framework, BUNDLE bundle, BUNDLE_CONTEXT *bundle_context);
celix_status_t bundleContext_destroy(BUNDLE_CONTEXT context);

celix_status_t bundleContext_getBundle(BUNDLE_CONTEXT context, BUNDLE *bundle);
celix_status_t bundleContext_getFramework(BUNDLE_CONTEXT context, FRAMEWORK *framework);
celix_status_t bundleContext_getMemoryPool(BUNDLE_CONTEXT context, apr_pool_t **memory_pool);

celix_status_t bundleContext_installBundle(BUNDLE_CONTEXT context, char * location, BUNDLE *bundle);

SERVICE_REGISTRATION bundleContext_registerService(BUNDLE_CONTEXT context, char * serviceName, void * svcObj, PROPERTIES properties);

ARRAY_LIST bundleContext_getServiceReferences(BUNDLE_CONTEXT context, char * serviceName, char * filter);
SERVICE_REFERENCE bundleContext_getServiceReference(BUNDLE_CONTEXT context, char * serviceName);

void * bundleContext_getService(BUNDLE_CONTEXT context, SERVICE_REFERENCE reference);
bool bundleContext_ungetService(BUNDLE_CONTEXT context, SERVICE_REFERENCE reference);

celix_status_t bundleContext_getBundles(BUNDLE_CONTEXT context, ARRAY_LIST *bundles);
BUNDLE bundleContext_getBundleById(BUNDLE_CONTEXT context, long id);

void bundleContext_addServiceListener(BUNDLE_CONTEXT context, SERVICE_LISTENER listener, char * filter);
void bundleContext_removeServiceListener(BUNDLE_CONTEXT context, SERVICE_LISTENER listener);


#endif /* BUNDLE_CONTEXT_H_ */
