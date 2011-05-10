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
void wire_destroy(WIRE wire);

CAPABILITY wire_getCapability(WIRE wire);
REQUIREMENT wire_getRequirement(WIRE wire);
MODULE wire_getImporter(WIRE wire);
MODULE wire_getExporter(WIRE wire);

#endif /* WIRE_H_ */
