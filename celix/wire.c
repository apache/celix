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
