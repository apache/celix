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

	WIRE providerWire = module_getWire(bundle_getModule(provider), serviceName);
	WIRE requesterWire = module_getWire(bundle_getModule(requester), serviceName);

	if (providerWire == NULL && requesterWire != NULL) {
		allow = (bundle_getModule(provider) == wire_getExporter(requesterWire));
	} else if (providerWire != NULL && requesterWire != NULL) {
		allow = (wire_getExporter(providerWire) == wire_getExporter(requesterWire));
	} else if (providerWire != NULL && requesterWire == NULL) {
		allow = (wire_getExporter(providerWire) == bundle_getModule(requester));
	} else {
		allow = false;
	}

	return allow;
}

