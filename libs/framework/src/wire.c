/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * wire.c
 *
 *  \date       Jul 19, 2010
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>

#include "wire.h"

struct wire {
    module_pt importer;
    requirement_pt requirement;
    module_pt exporter;
    capability_pt capability;
};

//LCOV_EXCL_START
celix_status_t wire_create(
    module_pt importer, requirement_pt requirement, module_pt exporter, capability_pt capability, wire_pt* wire) {
        celix_status_t status = CELIX_FRAMEWORK_EXCEPTION;

        framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Cannot create wire");

        return status;
}

celix_status_t wire_destroy(wire_pt wire) {
	return CELIX_FRAMEWORK_EXCEPTION;
}

celix_status_t wire_getCapability(wire_pt wire, capability_pt *capability) {
	return CELIX_FRAMEWORK_EXCEPTION;
}

celix_status_t wire_getRequirement(wire_pt wire, requirement_pt *requirement) {
    return CELIX_FRAMEWORK_EXCEPTION;
}

celix_status_t wire_getImporter(wire_pt wire, module_pt *importer) {
    return CELIX_FRAMEWORK_EXCEPTION;
}

celix_status_t wire_getExporter(wire_pt wire, module_pt *exporter) {
    return CELIX_FRAMEWORK_EXCEPTION;
}
//LCOV_EXCL_STOP
