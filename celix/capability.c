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
 * capability.c
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>

#include "capability.h"
#include "attribute.h"

struct capability {
	char * serviceName;
	MODULE module;
	VERSION version;
	HASH_MAP attributes;
	HASH_MAP directives;
};

CAPABILITY capability_create(MODULE module, HASH_MAP directives, HASH_MAP attributes) {
	CAPABILITY capability = (CAPABILITY) malloc(sizeof(*capability));

	capability->module = module;
	capability->attributes = attributes;
	capability->directives = directives;

	capability->version = version_createEmptyVersion();
	ATTRIBUTE versionAttribute = (ATTRIBUTE) hashMap_get(attributes, "version");
	if (versionAttribute != NULL) {
		capability->version = version_createVersionFromString(versionAttribute->value);
	}
	ATTRIBUTE serviceAttribute = (ATTRIBUTE) hashMap_get(attributes, "service");
	capability->serviceName = serviceAttribute->value;

	return capability;
}

char * capability_getServiceName(CAPABILITY capability) {
	return capability->serviceName;
}

VERSION capability_getVersion(CAPABILITY capability) {
	return capability->version;
}

MODULE capability_getModule(CAPABILITY capability) {
	return capability->module;
}
