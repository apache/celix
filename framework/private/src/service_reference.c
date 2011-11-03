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
 * service_reference.c
 *
 *  Created on: Jul 20, 2010
 *      Author: alexanderb
 */
#include <stdio.h>
#include "service_reference.h"
#include "module.h"
#include "wire.h"
#include "bundle.h"

bool serviceReference_isAssignableTo(SERVICE_REFERENCE reference, BUNDLE requester, char * serviceName) {
	bool allow = true;

	BUNDLE provider = reference->bundle;
	if (requester == provider) {
		return allow;
	}

//	WIRE providerWire = module_getWire(bundle_getCurrentModule(provider), serviceName);
//	WIRE requesterWire = module_getWire(bundle_getCurrentModule(requester), serviceName);
//
//	if (providerWire == NULL && requesterWire != NULL) {
//		allow = (bundle_getCurrentModule(provider) == wire_getExporter(requesterWire));
//	} else if (providerWire != NULL && requesterWire != NULL) {
//		allow = (wire_getExporter(providerWire) == wire_getExporter(requesterWire));
//	} else if (providerWire != NULL && requesterWire == NULL) {
//		allow = (wire_getExporter(providerWire) == bundle_getCurrentModule(requester));
//	} else {
//		allow = false;
//	}

	return allow;
}

