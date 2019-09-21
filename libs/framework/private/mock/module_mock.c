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
 * module_mock.c
 *
 *  \date       Feb 11, 2013
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include "CppUTestExt/MockSupport_c.h"

#include "module.h"

module_pt module_create(manifest_pt headerMap, const char * moduleId, bundle_pt bundle) {
	mock_c()->actualCall("module_create")
			->withPointerParameters("headerMap", headerMap)
			->withStringParameters("moduleId", moduleId)
			->withPointerParameters("bundle", bundle);
	return mock_c()->returnValue().value.pointerValue;
}

module_pt module_createFrameworkModule(bundle_pt bundle) {
	mock_c()->actualCall("module_createFrameworkModule")
			->withPointerParameters("bundle", bundle);
	return mock_c()->returnValue().value.pointerValue;
}

void module_destroy(module_pt module) {
	mock_c()->actualCall("module_destroy");
}

unsigned int module_hash(void * module) {
	mock_c()->actualCall("module_hash");
	return mock_c()->returnValue().value.intValue;
}

int module_equals(void * module, void * compare) {
	mock_c()->actualCall("module_equals");
	return mock_c()->returnValue().value.intValue;
}

wire_pt module_getWire(module_pt module, const char * serviceName) {
	mock_c()->actualCall("module_getwire");
	return mock_c()->returnValue().value.pointerValue;
}

version_pt module_getVersion(module_pt module) {
	mock_c()->actualCall("module_getVersion")
		->withPointerParameters("module", module);
	return mock_c()->returnValue().value.pointerValue;
}

celix_status_t module_getSymbolicName(module_pt module, const char **symbolicName) {
	mock_c()->actualCall("module_getSymbolicName")
		->withPointerParameters("module", module)
		->withOutputParameter("symbolicName", (const char **) symbolicName);
	return mock_c()->returnValue().value.intValue;
}

celix_status_t module_getGroup(module_pt module, const char **group) {
	mock_c()->actualCall("module_getGroup")
			->withPointerParameters("module", module)
			->withOutputParameter("group", (const char **) group);
	return mock_c()->returnValue().value.intValue;
}

char * module_getId(module_pt module) {
	mock_c()->actualCall("module_getId");
	return (char *) mock_c()->returnValue().value.stringValue;
}

linked_list_pt module_getWires(module_pt module) {
	mock_c()->actualCall("module_getWires");
	return mock_c()->returnValue().value.pointerValue;
}

void module_setWires(module_pt module, linked_list_pt wires) {
	mock_c()->actualCall("module_setWires");
}

bool module_isResolved(module_pt module) {
	mock_c()->actualCall("module_isResolved");
	return mock_c()->returnValue().value.intValue;
}

void module_setResolved(module_pt module) {
	mock_c()->actualCall("module_setResolved");
}

bundle_pt module_getBundle(module_pt module) {
	mock_c()->actualCall("module_getBundle");
	return mock_c()->returnValue().value.pointerValue;
}

linked_list_pt module_getRequirements(module_pt module) {
	mock_c()->actualCall("module_getRequirements");
	return mock_c()->returnValue().value.pointerValue;
}

linked_list_pt module_getCapabilities(module_pt module) {
	mock_c()->actualCall("module_getCapabilities");
	return mock_c()->returnValue().value.pointerValue;
}

array_list_pt module_getDependentImporters(module_pt module) {
	mock_c()->actualCall("module_getDependentImporters");
	return mock_c()->returnValue().value.pointerValue;
}

void module_addDependentImporter(module_pt module, module_pt importer) {
	mock_c()->actualCall("module_addDependentImporter");
}

void module_removeDependentImporter(module_pt module, module_pt importer) {
	mock_c()->actualCall("module_removeDependentImporter");
}

array_list_pt module_getDependentRequirers(module_pt module) {
	mock_c()->actualCall("module_getDependentRequirers");
	return mock_c()->returnValue().value.pointerValue;
}

void module_addDependentRequirer(module_pt module, module_pt requirer) {
	mock_c()->actualCall("module_addDependentRequirer");
}

void module_removeDependentRequirer(module_pt module, module_pt requirer) {
	mock_c()->actualCall("module_removeDependentRequirer");
}

array_list_pt module_getDependents(module_pt module) {
	mock_c()->actualCall("module_getDependents");
	return mock_c()->returnValue().value.pointerValue;
}




