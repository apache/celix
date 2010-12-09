/*
 * wire.h
 *
 *  Created on: Jul 12, 2010
 *      Author: alexanderb
 */

#ifndef WIRE_H_
#define WIRE_H_

#include "requirement.h"
#include "capability.h"
#include "module.h"
#include "linkedlist.h"
#include "headers.h"

WIRE wire_create(MODULE importer, REQUIREMENT requirement,
		MODULE exporter, CAPABILITY capability);

CAPABILITY wire_getCapability(WIRE wire);
REQUIREMENT wire_getRequirement(WIRE wire);
MODULE wire_getImporter(WIRE wire);
MODULE wire_getExporter(WIRE wire);

#endif /* WIRE_H_ */
