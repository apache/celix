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
