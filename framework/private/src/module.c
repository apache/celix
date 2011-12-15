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
 * module.c
 *
 *  Created on: Jul 19, 2010
 *      Author: alexanderb
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <apr_strings.h>

#include "module.h"
#include "manifest_parser.h"
#include "linked_list_iterator.h"
#include "capability.h"
#include "wire.h"

struct module {
	LINKED_LIST capabilities;
	LINKED_LIST requirements;
	LINKED_LIST wires;

	ARRAY_LIST dependentImporters;

	VERSION version;
	char * symbolicName;
	bool resolved;

	MANIFEST headerMap;
	char * id;

	struct bundle * bundle;
};

MODULE module_create(MANIFEST headerMap, char * moduleId, BUNDLE bundle) {
    MODULE module;
    MANIFEST_PARSER mp;
    apr_pool_t *pool;

    module = NULL;
    pool = NULL;

    if (headerMap != NULL) {
        module = (MODULE) apr_palloc(bundle->memoryPool, sizeof(*module));
        module->headerMap = headerMap;
        module->id = apr_pstrdup(bundle->memoryPool, moduleId);
        module->bundle = bundle;
        module->resolved = false;

        module->dependentImporters = NULL;
        arrayList_create(bundle->memoryPool, &module->dependentImporters);

        if (apr_pool_create(&pool, bundle->memoryPool) == APR_SUCCESS) {
            if (manifestParser_create(module, headerMap, pool, &mp) == CELIX_SUCCESS) {
            	module->symbolicName = NULL;
            	manifestParser_getSymbolicName(mp, bundle->memoryPool, &module->symbolicName);

                module->version = NULL;
                manifestParser_getBundleVersion(mp, bundle->memoryPool, &module->version);

                module->capabilities = NULL;
                manifestParser_getCapabilities(mp, bundle->memoryPool, &module->capabilities);

                module->requirements = NULL;
                manifestParser_getRequirements(mp, bundle->memoryPool, &module->requirements);

                module->wires = NULL;
            } else {
                apr_pool_destroy(pool);
            }
        }
    }

    return module;
}

MODULE module_createFrameworkModule(BUNDLE bundle) {
    MODULE module;
    apr_pool_t *capabilities_pool;
    apr_pool_t *requirements_pool;
    apr_pool_t *dependentImporters_pool;

	module = (MODULE) apr_palloc(bundle->memoryPool, sizeof(*module));
	if (module) {
	    if (apr_pool_create(&capabilities_pool, bundle->memoryPool) == APR_SUCCESS) {
	        if (apr_pool_create(&requirements_pool, bundle->memoryPool) == APR_SUCCESS) {
	            if (apr_pool_create(&dependentImporters_pool, bundle->memoryPool) == APR_SUCCESS) {
                    module->id = apr_pstrdup(bundle->memoryPool, "0");
                    module->symbolicName = apr_pstrdup(bundle->memoryPool, "framework");
                    module->version = NULL;
                    version_createVersion(bundle->memoryPool, 1, 0, 0, "", &module->version);

                    linkedList_create(capabilities_pool, &module->capabilities);
                    linkedList_create(requirements_pool, &module->requirements);
                    module->dependentImporters = NULL;
                    arrayList_create(bundle->memoryPool, &module->dependentImporters);
                    module->wires = NULL;
                    module->headerMap = NULL;
                    module->resolved = false;
                    module->bundle = bundle;
	            }
	        }
	    }
	}
	return module;
}

void module_destroy(MODULE module) {
	LINKED_LIST_ITERATOR reqIter = linkedListIterator_create(module->requirements, 0);
	while (linkedListIterator_hasNext(reqIter)) {
		REQUIREMENT req = linkedListIterator_next(reqIter);
		requirement_destroy(req);
	}
	linkedListIterator_destroy(reqIter);

	arrayList_destroy(module->dependentImporters);

	if (module->headerMap != NULL) {
		manifest_destroy(module->headerMap);
	}
	module->headerMap = NULL;
}

WIRE module_getWire(MODULE module, char * serviceName) {
	WIRE wire = NULL;
	if (module->wires != NULL) {
		LINKED_LIST_ITERATOR iterator = linkedListIterator_create(module->wires, 0);
		while (linkedListIterator_hasNext(iterator)) {
			WIRE next = linkedListIterator_next(iterator);
			CAPABILITY cap = NULL;
			wire_getCapability(next, &cap);
			char *name;
			capability_getServiceName(cap, &name);
			if (strcasecmp(name, serviceName) == 0) {
				wire = next;
			}
		}
		linkedListIterator_destroy(iterator);
	}
	return wire;
}

VERSION module_getVersion(MODULE module) {
	return module->version;
}

celix_status_t module_getSymbolicName(MODULE module, char **symbolicName) {
	celix_status_t status = CELIX_SUCCESS;

	if (module == NULL || *symbolicName != NULL) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		*symbolicName = module->symbolicName;
	}

	return status;
}

char * module_getId(MODULE module) {
	return module->id;
}

LINKED_LIST module_getWires(MODULE module) {
	return module->wires;
}

void module_setWires(MODULE module, LINKED_LIST wires) {
    int i = 0;
    for (i = 0; (module->wires != NULL) && (i < linkedList_size(module->wires)); i++) {
        WIRE wire = (WIRE) linkedList_get(module->wires, i);
        MODULE exporter = NULL;
        wire_getExporter(wire, &exporter);
        module_removeDependentImporter(exporter, module);
    }

	module->wires = wires;

	for (i = 0; (module->wires != NULL) && (i < linkedList_size(module->wires)); i++) {
        WIRE wire = (WIRE) linkedList_get(module->wires, i);
        MODULE exporter = NULL;
        wire_getExporter(wire, &exporter);
        module_addDependentImporter(exporter, module);
    }
}

bool module_isResolved(MODULE module) {
	return module->resolved;
}

void module_setResolved(MODULE module) {
	module->resolved = true;
}

BUNDLE module_getBundle(MODULE module) {
	return module->bundle;
}

LINKED_LIST module_getRequirements(MODULE module) {
	return module->requirements;
}

LINKED_LIST module_getCapabilities(MODULE module) {
	return module->capabilities;
}

ARRAY_LIST module_getDependentImporters(MODULE module) {
    return module->dependentImporters;
}

void module_addDependentImporter(MODULE module, MODULE importer) {
    if (!arrayList_contains(module->dependentImporters, importer)) {
        arrayList_add(module->dependentImporters, importer);
    }
}

void module_removeDependentImporter(MODULE module, MODULE importer) {
    arrayList_removeElement(module->dependentImporters, importer);
}

ARRAY_LIST module_getDependents(MODULE module) {
    ARRAY_LIST dependents = NULL;
    arrayList_create(module->bundle->memoryPool, &dependents);

    arrayList_addAll(dependents, module->dependentImporters);

    return dependents;
}
