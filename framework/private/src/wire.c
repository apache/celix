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
 *  \date       Jul 19, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <string.h>

#include "wire.h"

struct wire {
	module_t importer;
	requirement_t requirement;
	module_t exporter;
	capability_t capability;
};

apr_status_t wire_destroy(void *wireP);

celix_status_t wire_create(apr_pool_t *pool, module_t importer, requirement_t requirement,
		module_t exporter, capability_t capability, wire_t *wire) {
	celix_status_t status = CELIX_SUCCESS;

	(*wire) = (wire_t) apr_palloc(pool, sizeof(**wire));
	if (!*wire) {
		status = CELIX_ENOMEM;
	} else {
		apr_pool_pre_cleanup_register(pool, *wire, wire_destroy);

		(*wire)->importer = importer;
		(*wire)->requirement = requirement;
		(*wire)->exporter = exporter;
		(*wire)->capability = capability;
	}

	return status;
}

apr_status_t wire_destroy(void *wireP) {
	wire_t wire = wireP;
	wire->importer = NULL;
	wire->requirement = NULL;
	wire->exporter = NULL;
	wire->capability = NULL;
	return APR_SUCCESS;
}

celix_status_t wire_getCapability(wire_t wire, capability_t *capability) {
	*capability = wire->capability;
	return CELIX_SUCCESS;
}

celix_status_t wire_getRequirement(wire_t wire, requirement_t *requirement) {
	*requirement = wire->requirement;
	return CELIX_SUCCESS;
}

celix_status_t wire_getImporter(wire_t wire, module_t *importer) {
	*importer = wire->importer;
	return CELIX_SUCCESS;
}

celix_status_t wire_getExporter(wire_t wire, module_t *exporter) {
	*exporter = wire->exporter;
	return CELIX_SUCCESS;
}
