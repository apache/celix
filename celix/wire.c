/*
 * wire.c
 *
 *  Created on: Jul 19, 2010
 *      Author: alexanderb
 */
#include <stdlib.h>
#include <string.h>

#include "wire.h"

struct wire {
	MODULE importer;
	REQUIREMENT requirement;
	MODULE exporter;
	CAPABILITY capability;
};

WIRE wire_create(MODULE importer, REQUIREMENT requirement,
		MODULE exporter, CAPABILITY capability) {
	WIRE wire = (WIRE) malloc(sizeof(*wire));
	wire->importer = importer;
	wire->requirement = requirement;
	wire->exporter = exporter;
	wire->capability = capability;

	return wire;
}

CAPABILITY wire_getCapability(WIRE wire) {
	return wire->capability;
}

REQUIREMENT wire_getRequirement(WIRE wire) {
	return wire->requirement;
}

MODULE wire_getImporter(WIRE wire) {
	return wire->importer;
}

MODULE wire_getExporter(WIRE wire) {
	return wire->exporter;
}
