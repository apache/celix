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
 * comopnent_metadata.c
 *
 *  \date       Nov 3, 2015
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>

#include <array_list.h>

#include "component_metadata.h"

struct component {
	apr_pool_t *pool;

	char *name;
	bool enabled;
	bool immediate;
	char *factoryIdentifier;
	char *configurationPolicy;
	char *activate;
	char *deactivate;
	char *modified;

	char *implementationClassName;

	service_t service;

	ARRAY_LIST references;
};

celix_status_t component_create(apr_pool_t *pool, component_t *component) {
	*component = malloc(sizeof(**component));
	(*component)->pool = pool;

	(*component)->name = NULL;
	(*component)->enabled = true;
	(*component)->immediate = false;
	(*component)->factoryIdentifier = NULL;
	(*component)->configurationPolicy = NULL;
	(*component)->activate = NULL;
	(*component)->deactivate = NULL;
	(*component)->modified = NULL;

	(*component)->implementationClassName = NULL;

	(*component)->service = NULL;

	arrayList_create(pool, &(*component)->references);

	return CELIX_SUCCESS;
}

celix_status_t component_getName(component_t component, char **name) {
	*name = component->name;
	return CELIX_SUCCESS;
}

celix_status_t component_setName(component_t component, char *name) {
	component->name = name;
	return CELIX_SUCCESS;
}

celix_status_t component_isEnabled(component_t component, bool *enabled) {
	*enabled = component->enabled;
	return CELIX_SUCCESS;
}

celix_status_t component_setEnabled(component_t component, bool enabled) {
	component->enabled = enabled;
	return CELIX_SUCCESS;
}

celix_status_t component_isImmediate(component_t component, bool *immediate) {
	*immediate = component->immediate;
	return CELIX_SUCCESS;
}

celix_status_t component_setImmediate(component_t component, bool immediate) {
	component->enabled = immediate;
	return CELIX_SUCCESS;
}

celix_status_t component_getFactoryIdentifier(component_t component, char **factoryIdentifier) {
	*factoryIdentifier = component->factoryIdentifier;
	return CELIX_SUCCESS;
}

celix_status_t component_setFactoryIdentifier(component_t component, char *factoryIdentifier) {
	component->factoryIdentifier = factoryIdentifier;
	return CELIX_SUCCESS;
}

celix_status_t component_getConfigurationPolicy(component_t component, char **configurationPolicy) {
	*configurationPolicy = component->configurationPolicy;
	return CELIX_SUCCESS;
}

celix_status_t component_setConfigurationPolicy(component_t component, char *configurationPolicy) {
	component->configurationPolicy = configurationPolicy;
	return CELIX_SUCCESS;
}

celix_status_t component_getActivate(component_t component, char **activate) {
	*activate = component->activate;
	return CELIX_SUCCESS;
}

celix_status_t component_setActivate(component_t component, char *activate) {
	component->activate = activate;
	return CELIX_SUCCESS;
}

celix_status_t component_getDeactivate(component_t component, char **deactivate) {
	*deactivate = component->deactivate;
	return CELIX_SUCCESS;
}

celix_status_t component_setDeactivate(component_t component, char *deactivate) {
	component->deactivate = deactivate;
	return CELIX_SUCCESS;
}

celix_status_t component_getModified(component_t component, char **modified) {
	*modified = component->modified;
	return CELIX_SUCCESS;
}

celix_status_t component_setModified(component_t component, char *modified) {
	component->modified = modified;
	return CELIX_SUCCESS;
}

celix_status_t component_getImplementationClassName(component_t component, char **implementationClassName) {
	*implementationClassName = component->implementationClassName;
	return CELIX_SUCCESS;
}

celix_status_t component_setImplementationClassName(component_t component, char *implementationClassName) {
	component->implementationClassName = implementationClassName;
	return CELIX_SUCCESS;
}

celix_status_t componentMetadata_getService(component_t component, service_t *service) {
	*service = component->service;
	return CELIX_SUCCESS;
}

celix_status_t componentMetadata_setService(component_t component, service_t service) {
	component->service = service;
	return CELIX_SUCCESS;
}

celix_status_t componentMetadata_getDependencies(component_t component, reference_metadata_t *references[], int *size) {
	*size = arrayList_size(component->references);
	reference_metadata_t *referencesA = malloc(*size * sizeof(*referencesA));
	int i;
	for (i = 0; i < *size; i++) {
		reference_metadata_t ref = arrayList_get(component->references, i);
		referencesA[i] = ref;
	}
	*references = referencesA;
	return CELIX_SUCCESS;
}

celix_status_t componentMetadata_addDependency(component_t component, reference_metadata_t reference) {
	arrayList_add(component->references, reference);
	return CELIX_SUCCESS;
}
