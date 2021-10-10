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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <celix_utils.h>

#include "module.h"
#include "manifest_parser.h"
#include "linked_list_iterator.h"

struct module {
	linked_list_pt capabilities;
	linked_list_pt requirements;
	linked_list_pt wires;

	array_list_pt dependentImporters;

	version_pt version;
	char* symbolicName;
	char* group;
    char* name;
    char* description;
	bool resolved;

	manifest_pt headerMap;
	char * id;

	celix_bundle_t *bundle;
};

module_pt module_create(manifest_pt headerMap, const char * moduleId, bundle_pt bundle) {
    module_pt module = NULL;
    manifest_parser_pt mp;

    if (headerMap != NULL) {
        module = (module_pt) calloc(1, sizeof(*module));
        module->headerMap = headerMap;
        module->id = strdup(moduleId);
        module->bundle = bundle;
        module->resolved = false;

        module->dependentImporters = NULL;
        arrayList_create(&module->dependentImporters);

        if (manifestParser_create(module, headerMap, &mp) == CELIX_SUCCESS) {
            module->symbolicName = NULL;
            manifestParser_getAndDuplicateSymbolicName(mp, &module->symbolicName);

            module->group = NULL;
            manifestParser_getAndDuplicateGroup(mp, &module->group);

            module->name = NULL;
            manifestParser_getAndDuplicateName(mp, &module->name);

            module->description = NULL;
            manifestParser_getAndDuplicateDescription(mp, &module->description);

            module->version = NULL;
            manifestParser_getBundleVersion(mp, &module->version);

            module->capabilities = NULL;
            manifestParser_getCapabilities(mp, &module->capabilities);

            module->requirements = NULL;
            manifestParser_getRequirements(mp, &module->requirements);

            module->wires = NULL;

            manifestParser_destroy(mp);
        }
    }

    return module;
}

module_pt module_createFrameworkModule(bundle_pt bundle) {
    module_pt module;

	module = (module_pt) calloc(1, sizeof(*module));
	if (module) {
        module->id = strdup("0");
        module->symbolicName = celix_utils_strdup("celix_framework");
        module->group = celix_utils_strdup("Celix/Framework");
        module->name = celix_utils_strdup("Celix Framework");
        module->description = celix_utils_strdup("The Celix Framework System Bundle");
        module->version = NULL;
        version_createVersion(1, 0, 0, "", &module->version);

        linkedList_create(&module->capabilities);
        linkedList_create(&module->requirements);
        module->dependentImporters = NULL;
        arrayList_create(&module->dependentImporters);
        module->wires = NULL;
        module->headerMap = NULL;
        module->resolved = false;
        module->bundle = bundle;
	}
	return module;
}

void module_destroy(module_pt module) {
	arrayList_destroy(module->dependentImporters);

	version_destroy(module->version);

	if (module->wires != NULL) {
        linked_list_iterator_pt iter = linkedListIterator_create(module->wires, 0);
        while (linkedListIterator_hasNext(iter)) {
            wire_pt next = linkedListIterator_next(iter);
            linkedListIterator_remove(iter);
            wire_destroy(next);
        }
        linkedListIterator_destroy(iter);
        linkedList_destroy(module->wires);
	}

	if (module->requirements != NULL) {
	    linked_list_iterator_pt iter = linkedListIterator_create(module->requirements, 0);
        while (linkedListIterator_hasNext(iter)) {
            requirement_pt next = linkedListIterator_next(iter);
            linkedListIterator_remove(iter);
            requirement_destroy(next);
        }
        linkedListIterator_destroy(iter);
        linkedList_destroy(module->requirements);
	}

	if (module->capabilities != NULL) {
	    linked_list_iterator_pt iter = linkedListIterator_create(module->capabilities, 0);
        while (linkedListIterator_hasNext(iter)) {
            capability_pt next = linkedListIterator_next(iter);
            linkedListIterator_remove(iter);
            capability_destroy(next);
        }
        linkedListIterator_destroy(iter);
        linkedList_destroy(module->capabilities);
    }

	module->headerMap = NULL;

	free(module->id);
	free(module->symbolicName);
    free(module->name);
    free(module->group);
    free(module->description);
	free(module);
}

wire_pt module_getWire(module_pt module, const char * serviceName) {
	wire_pt wire = NULL;
	if (module->wires != NULL) {
		linked_list_iterator_pt iterator = linkedListIterator_create(module->wires, 0);
		while (linkedListIterator_hasNext(iterator)) {
			const char* name;
			wire_pt next = linkedListIterator_next(iterator);
			capability_pt cap = NULL;
			wire_getCapability(next, &cap);
			capability_getServiceName(cap, &name);
			if (strcasecmp(name, serviceName) == 0) {
				wire = next;
			}
		}
		linkedListIterator_destroy(iterator);
	}
	return wire;
}

version_pt module_getVersion(module_pt module) {
	return module->version;
}

celix_status_t module_getSymbolicName(module_pt module, const char **symbolicName) {
	celix_status_t status = CELIX_SUCCESS;
	if (module == NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*symbolicName = module->symbolicName;
	}
	return status;
}

celix_status_t module_getName(module_pt module, const char **symbolicName) {
    celix_status_t status = CELIX_SUCCESS;
    if (module == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        *symbolicName = module->name;
    }
    return status;
}

celix_status_t module_getGroup(module_pt module, const char **symbolicName) {
    celix_status_t status = CELIX_SUCCESS;
    if (module == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        *symbolicName = module->group;
    }
    return status;
}

celix_status_t module_getDescription(module_pt module, const char **symbolicName) {
    celix_status_t status = CELIX_SUCCESS;
    if (module == NULL) {
        status = CELIX_ILLEGAL_ARGUMENT;
    } else {
        *symbolicName = module->description;
    }
    return status;
}

char * module_getId(module_pt module) {
	return module->id;
}

linked_list_pt module_getWires(module_pt module) {
	return module->wires;
}

void module_setWires(module_pt module, linked_list_pt wires) {
    int i = 0;
    for (i = 0; (module->wires != NULL) && (i < linkedList_size(module->wires)); i++) {
        wire_pt wire = (wire_pt) linkedList_get(module->wires, i);
        module_pt exporter = NULL;
        wire_getExporter(wire, &exporter);
        module_removeDependentImporter(exporter, module);
    }

    if (module->wires != NULL) {
		linked_list_iterator_pt iter = linkedListIterator_create(module->wires, 0);
		while (linkedListIterator_hasNext(iter)) {
			wire_pt next = linkedListIterator_next(iter);
			linkedListIterator_remove(iter);
			wire_destroy(next);
		}
		linkedListIterator_destroy(iter);
		linkedList_destroy(module->wires);
	}

	module->wires = wires;

	for (i = 0; (module->wires != NULL) && (i < linkedList_size(module->wires)); i++) {
        wire_pt wire = (wire_pt) linkedList_get(module->wires, i);
        module_pt exporter = NULL;
        wire_getExporter(wire, &exporter);
        module_addDependentImporter(exporter, module);
    }
}

bool module_isResolved(module_pt module) {
	return module->resolved;
}

void module_setResolved(module_pt module) {
	module->resolved = true;
}

bundle_pt module_getBundle(module_pt module) {
	return module->bundle;
}

linked_list_pt module_getRequirements(module_pt module) {
	return module->requirements;
}

linked_list_pt module_getCapabilities(module_pt module) {
	return module->capabilities;
}

array_list_pt module_getDependentImporters(module_pt module) {
    return module->dependentImporters;
}

void module_addDependentImporter(module_pt module, module_pt importer) {
    if (!arrayList_contains(module->dependentImporters, importer)) {
        arrayList_add(module->dependentImporters, importer);
    }
}

void module_removeDependentImporter(module_pt module, module_pt importer) {
    arrayList_removeElement(module->dependentImporters, importer);
}

//----------------------------------------------------
//TODO add implementation (functions not implemented but already exported)
array_list_pt module_getDependentRequirers(module_pt module) {
	return NULL;
}

void module_addDependentRequirer(module_pt module, module_pt requirer) {
}

void module_removeDependentRequirer(module_pt module, module_pt requirer) {
}
//----------------------------------------------------

array_list_pt module_getDependents(module_pt module) {
    array_list_pt dependents = NULL;
    arrayList_create(&dependents);

    arrayList_addAll(dependents, module->dependentImporters);

    return dependents;
}