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
 *  \date       Mar 26, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#ifndef BUNDLE_CONTEXT_H_
#define BUNDLE_CONTEXT_H_

/**
 * A bundle's execution context within the Framework. The context is used to
 * grant access to other methods so that this bundle can interact with the
 * Framework.
 */
typedef struct bundleContext *bundle_context_t;

#include "service_factory.h"
#include "service_listener.h"
#include "bundle_listener.h"
#include "properties.h"
#include "array_list.h"

FRAMEWORK_EXPORT celix_status_t bundleContext_create(FRAMEWORK framework, BUNDLE bundle, bundle_context_t *bundle_context);
FRAMEWORK_EXPORT celix_status_t bundleContext_destroy(bundle_context_t context);

FRAMEWORK_EXPORT celix_status_t bundleContext_getBundle(bundle_context_t context, BUNDLE *bundle);
FRAMEWORK_EXPORT celix_status_t bundleContext_getFramework(bundle_context_t context, FRAMEWORK *framework);
FRAMEWORK_EXPORT celix_status_t bundleContext_getMemoryPool(bundle_context_t context, apr_pool_t **memory_pool);

FRAMEWORK_EXPORT celix_status_t bundleContext_installBundle(bundle_context_t context, char * location, BUNDLE *bundle);
FRAMEWORK_EXPORT celix_status_t bundleContext_installBundle2(bundle_context_t context, char * location, char *inputFile, BUNDLE *bundle);

FRAMEWORK_EXPORT celix_status_t bundleContext_registerService(bundle_context_t context, char * serviceName, void * svcObj,
        PROPERTIES properties, SERVICE_REGISTRATION *service_registration);
FRAMEWORK_EXPORT celix_status_t bundleContext_registerServiceFactory(bundle_context_t context, char * serviceName, service_factory_t factory,
        PROPERTIES properties, SERVICE_REGISTRATION *service_registration);

FRAMEWORK_EXPORT celix_status_t bundleContext_getServiceReferences(bundle_context_t context, const char * serviceName, char * filter, ARRAY_LIST *service_references);
FRAMEWORK_EXPORT celix_status_t bundleContext_getServiceReference(bundle_context_t context, char * serviceName, SERVICE_REFERENCE *service_reference);

FRAMEWORK_EXPORT celix_status_t bundleContext_getService(bundle_context_t context, SERVICE_REFERENCE reference, void **service_instance);
FRAMEWORK_EXPORT celix_status_t bundleContext_ungetService(bundle_context_t context, SERVICE_REFERENCE reference, bool *result);

FRAMEWORK_EXPORT celix_status_t bundleContext_getBundles(bundle_context_t context, ARRAY_LIST *bundles);
FRAMEWORK_EXPORT celix_status_t bundleContext_getBundleById(bundle_context_t context, long id, BUNDLE *bundle);

FRAMEWORK_EXPORT celix_status_t bundleContext_addServiceListener(bundle_context_t context, SERVICE_LISTENER listener, char * filter);
FRAMEWORK_EXPORT celix_status_t bundleContext_removeServiceListener(bundle_context_t context, SERVICE_LISTENER listener);

FRAMEWORK_EXPORT celix_status_t bundleContext_addBundleListener(bundle_context_t context, bundle_listener_t listener);
FRAMEWORK_EXPORT celix_status_t bundleContext_removeBundleListener(bundle_context_t context, bundle_listener_t listener);

FRAMEWORK_EXPORT celix_status_t bundleContext_getProperty(bundle_context_t context, const char *name, char **value);

#endif /* BUNDLE_CONTEXT_H_ */
