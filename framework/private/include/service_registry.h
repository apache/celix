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
 * service_registry.h
 *
 *  Created on: Aug 6, 2010
 *      Author: alexanderb
 */

#ifndef SERVICE_REGISTRY_H_
#define SERVICE_REGISTRY_H_

#include <pthread.h>

#include "headers.h"
#include "properties.h"
#include "filter.h"

SERVICE_REGISTRY serviceRegistry_create(void (*serviceChanged)(SERVICE_EVENT, PROPERTIES));
ARRAY_LIST serviceRegistry_getRegisteredServices(SERVICE_REGISTRY registry, BUNDLE bundle);
SERVICE_REGISTRATION serviceRegistry_registerService(SERVICE_REGISTRY registry, BUNDLE bundle, char * serviceName, void * serviceObject, PROPERTIES dictionary);
void serviceRegistry_unregisterService(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REGISTRATION registration);
void serviceRegistry_unregisterServices(SERVICE_REGISTRY registry, BUNDLE bundle);
ARRAY_LIST serviceRegistry_getServiceReferences(SERVICE_REGISTRY registry, char * serviceName, FILTER filter);
void * serviceRegistry_getService(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REFERENCE reference);
bool serviceRegistry_ungetService(SERVICE_REGISTRY registry, BUNDLE bundle, SERVICE_REFERENCE reference);
void serviceRegistry_ungetServices(SERVICE_REGISTRY registry, BUNDLE bundle);
ARRAY_LIST serviceRegistry_getUsingBundles(SERVICE_REGISTRY registry, SERVICE_REFERENCE reference);
SERVICE_REGISTRATION serviceRegistry_findRegistration(SERVICE_REGISTRY registry, SERVICE_REFERENCE reference);

#endif /* SERVICE_REGISTRY_H_ */
