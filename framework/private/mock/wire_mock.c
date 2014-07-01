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
 * wire_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */
#include "CppUTestExt/MockSupport_c.h"

#include "wire.h"

celix_status_t wire_create(module_pt importer, requirement_pt requirement,
		module_pt exporter, capability_pt capability, wire_pt *wire) {
	mock_c()->actualCall("requirement_create");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t wire_destroy(wire_pt wire) {
    mock_c()->actualCall("wire_destroy");
    return mock_c()->returnValue().value.intValue;
}

celix_status_t wire_getCapability(wire_pt wire, capability_pt *capability) {
	mock_c()->actualCall("requirement_create");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t wire_getRequirement(wire_pt wire, requirement_pt *requirement) {
	mock_c()->actualCall("requirement_create");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t wire_getImporter(wire_pt wire, module_pt *importer) {
	mock_c()->actualCall("requirement_create");
	return mock_c()->returnValue().value.intValue;
}

celix_status_t wire_getExporter(wire_pt wire, module_pt *exporter) {
	mock_c()->actualCall("requirement_create");
	return mock_c()->returnValue().value.intValue;
}

