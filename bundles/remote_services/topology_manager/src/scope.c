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
#include "scope.h"
#include "tm_scope.h"
#include "topology_manager.h"
#include "utils.h"
#include "filter.h"

struct scope_item {
    celix_properties_t *props;
};

struct scope {
    void *manager;	// owner of the scope datastructure
    celix_thread_mutex_t exportScopeLock;
    hash_map_pt exportScopes;           // key is filter, value is scope_item (properties set)

    celix_thread_mutex_t importScopeLock;
    array_list_pt importScopes;			// list of filters

    celix_status_t (*exportScopeChangedHandler)(void* manager, char *filter);
    celix_status_t (*importScopeChangedHandler)(void* manager, char *filter);
};

static celix_status_t import_equal(const void *, const void *, bool *equals);

/*
 * SERVICES
 */

celix_status_t tm_addExportScope(void *handle, char *filter, celix_properties_t *props) {
    celix_status_t status = CELIX_SUCCESS;
    scope_pt scope = (scope_pt) handle;
    celix_properties_t *present;

    if (handle == NULL)
        return CELIX_ILLEGAL_ARGUMENT;

    if (celixThreadMutex_lock(&scope->exportScopeLock) == CELIX_SUCCESS) {
        // For now we just don't allow two exactly the same filters
        // TODO: What we actually need is the following
        // If part of the new filter is already present in any of the filters in exportScopes
        // we have to assure that the new filter defines other property keys than the property keys
        // in the already defined filter!
        present = (celix_properties_t *) hashMap_get(scope->exportScopes, filter);
        if (present == NULL) {
            struct scope_item *item = calloc(1, sizeof(*item));
            if (item == NULL) {
                status = CELIX_ENOMEM;
            } else {
                item->props = props;
                hashMap_put(scope->exportScopes, (void*) strdup(filter), (void*) item);
            }
        } else {
            // don't allow the same filter twice
            celix_properties_destroy(props);
            status = CELIX_ILLEGAL_ARGUMENT;
        }
        celixThreadMutex_unlock(&scope->exportScopeLock);
    }

    if (scope->exportScopeChangedHandler != NULL) {
        status = CELIX_DO_IF(status, scope->exportScopeChangedHandler(scope->manager, filter));
    }

    return status;
}

celix_status_t tm_removeExportScope(void *handle, char *filter) {
    celix_status_t status = CELIX_SUCCESS;
    scope_pt scope = (scope_pt) handle;

    if (handle == NULL)
        return CELIX_ILLEGAL_ARGUMENT;

    if (celixThreadMutex_lock(&scope->exportScopeLock) == CELIX_SUCCESS) {
        struct scope_item *present = (struct scope_item *) hashMap_get(scope->exportScopes, filter);
        if (present == NULL) {
            status = CELIX_ILLEGAL_ARGUMENT;
        } else {
            celix_properties_destroy(present->props);
            hashMap_remove(scope->exportScopes, filter); // frees also the item!
        }
        celixThreadMutex_unlock(&scope->exportScopeLock);
    }
    if (scope->exportScopeChangedHandler != NULL) {
        status = CELIX_DO_IF(status, scope->exportScopeChangedHandler(scope->manager, filter));
    }
    return status;
}

celix_status_t tm_addImportScope(void *handle, char *filter) {
    celix_status_t status = CELIX_SUCCESS;
    scope_pt scope = (scope_pt) handle;

    if (handle == NULL)
        return CELIX_ILLEGAL_ARGUMENT;
    celix_autoptr(celix_filter_t) new = celix_filter_create(filter);
    if (new == NULL) {
        return CELIX_ILLEGAL_ARGUMENT; // filter not parsable
    }
    if (celixThreadMutex_lock(&scope->importScopeLock) == CELIX_SUCCESS) {
        int index = arrayList_indexOf(scope->importScopes, new);
        filter_pt present = (filter_pt) arrayList_get(scope->importScopes, index);
        if (present == NULL) {
            arrayList_add(scope->importScopes, celix_steal_ptr(new));
        } else {
            status = CELIX_ILLEGAL_ARGUMENT;
        }

        celixThreadMutex_unlock(&scope->importScopeLock);
    }
    if (scope->importScopeChangedHandler != NULL) {
        status = CELIX_DO_IF(status, scope->importScopeChangedHandler(scope->manager, filter));
    }
    return status;
}

celix_status_t tm_removeImportScope(void *handle, char *filter) {
    celix_status_t status = CELIX_SUCCESS;
    scope_pt scope = (scope_pt) handle;
    filter_pt new;

    if (handle == NULL)
        return CELIX_ILLEGAL_ARGUMENT;

    new = filter_create(filter);
    if (new == NULL) {
        return CELIX_ILLEGAL_ARGUMENT; // filter not parsable
    }

    if (celixThreadMutex_lock(&scope->importScopeLock) == CELIX_SUCCESS) {
        int index = arrayList_indexOf(scope->importScopes, new);
        filter_pt present = (filter_pt) arrayList_get(scope->importScopes, index);
        if (present == NULL)
            status = CELIX_ILLEGAL_ARGUMENT;
        else {
            arrayList_removeElement(scope->importScopes, present);
            filter_destroy(present);
        }
        celixThreadMutex_unlock(&scope->importScopeLock);
    }
    if (scope->importScopeChangedHandler != NULL) {
        status = CELIX_DO_IF(status, scope->importScopeChangedHandler(scope->manager, filter));
    }
    filter_destroy(new);
    return status;
}

/*****************************************************************************
 * GLOBAL FUNCTIONS
 *****************************************************************************/

void scope_setExportScopeChangedCallback(scope_pt scope, celix_status_t (*changed)(void *handle, char *servName)) {
    scope->exportScopeChangedHandler = changed;
}

void scope_setImportScopeChangedCallback(scope_pt scope, celix_status_t (*changed)(void *handle, char *servName)) {
    scope->importScopeChangedHandler = changed;
}

celix_status_t scope_scopeCreate(void *handle, scope_pt *scope) {
    celix_status_t status = CELIX_SUCCESS;

    *scope = calloc(1, sizeof **scope);

    if (*scope == NULL) {
        return CELIX_ENOMEM;
    }

    (*scope)->manager = handle;
    celixThreadMutex_create(&(*scope)->exportScopeLock, NULL);
    celixThreadMutex_create(&(*scope)->importScopeLock, NULL);

    (*scope)->exportScopes = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    arrayList_createWithEquals(import_equal, &((*scope)->importScopes));
    (*scope)->exportScopeChangedHandler = NULL;

    return status;
}

celix_status_t scope_scopeDestroy(scope_pt scope) {
    celix_status_t status = CELIX_SUCCESS;

    if (celixThreadMutex_lock(&scope->exportScopeLock) == CELIX_SUCCESS) {
        hash_map_iterator_pt iter = hashMapIterator_create(scope->exportScopes);
        while (hashMapIterator_hasNext(iter)) {
            hash_map_entry_pt scopedEntry = hashMapIterator_nextEntry(iter);
            struct scope_item *item = (struct scope_item*) hashMapEntry_getValue(scopedEntry);
            celix_properties_destroy(item->props);
        }
        hashMapIterator_destroy(iter);
        hashMap_destroy(scope->exportScopes, true, true); // free keys, free values
        celixThreadMutex_unlock(&scope->exportScopeLock);
    }

    if (celixThreadMutex_lock(&scope->importScopeLock) == CELIX_SUCCESS) {
        array_list_iterator_pt imp_iter = arrayListIterator_create(scope->importScopes);
        while (arrayListIterator_hasNext(imp_iter)) {
            filter_pt element = (filter_pt) arrayListIterator_next(imp_iter);
            filter_destroy(element);
            // no need to call arrayList_removeElement(element) because complete list is destroyed
        }
        arrayListIterator_destroy(imp_iter);
        arrayList_destroy(scope->importScopes);
        celixThreadMutex_unlock(&scope->importScopeLock);
    }

    celixThreadMutex_destroy(&scope->exportScopeLock);
    celixThreadMutex_destroy(&scope->importScopeLock);
    free(scope);
    return status;
}

/*****************************************************************************
 * STATIC FUNCTIONS
 *****************************************************************************/
static celix_status_t import_equal(const void *src, const void *dest, bool *equals) {
    celix_status_t status;

    filter_pt src_filter = (filter_pt) src;
    filter_pt dest_filter = (filter_pt) dest;
    status = filter_match_filter(src_filter, dest_filter, equals);
    return status;
}

bool scope_allowImport(scope_pt scope, endpoint_description_t *endpoint) {
    bool allowImport = false;
    array_list_iterator_pt iter;

    if (celixThreadMutex_lock(&(scope->importScopeLock)) == CELIX_SUCCESS) {
        if (arrayList_size(scope->importScopes) == 0) {
            allowImport = true;
        } else {
            iter = arrayListIterator_create(scope->importScopes);
            while ((allowImport == false) && arrayListIterator_hasNext(iter)) {
                filter_pt element = (filter_pt) arrayListIterator_next(iter);
                filter_match(element, endpoint->properties, &allowImport);
            }
            arrayListIterator_destroy(iter);
        }
        celixThreadMutex_unlock(&scope->importScopeLock);
    }
    return allowImport;
}

celix_status_t scope_getExportProperties(scope_pt scope, service_reference_pt reference, celix_properties_t **props) {
    celix_status_t status = CELIX_SUCCESS;
    unsigned int size = 0;
    char **keys;
    bool found = false;

    *props = NULL;
    celix_properties_t *serviceProperties = celix_properties_create();  // GB: not sure if a copy is needed
                                                                        // or serviceReference_getProperties() is
                                                                        // is acceptable

    serviceReference_getPropertyKeys(reference, &keys, &size);
    for (int i = 0; i < size; i++) {
        char *key = keys[i];
        const char* value = NULL;

        if (serviceReference_getProperty(reference, key, &value) == CELIX_SUCCESS) {
//        		&& strcmp(key, (char*) OSGI_RSA_SERVICE_EXPORTED_INTERFACES) != 0
//        		&& strcmp(key, (char*) CELIX_FRAMEWORK_SERVICE_NAME) != 0) {
            celix_properties_set(serviceProperties, key, value);
        }

    }

    free(keys);

    if (celixThreadMutex_lock(&(scope->exportScopeLock)) == CELIX_SUCCESS) {
        hash_map_iterator_pt scopedPropIter = hashMapIterator_create(scope->exportScopes);
        // TODO: now stopping if first filter matches, alternatively we could build up
        //       the additional output properties for each filter that matches?
        while ((!found) && hashMapIterator_hasNext(scopedPropIter)) {
            hash_map_entry_pt scopedEntry = hashMapIterator_nextEntry(scopedPropIter);
            char *filterStr = (char *) hashMapEntry_getKey(scopedEntry);
            celix_autoptr(celix_filter_t) filter = celix_filter_create(filterStr);
            if (filter != NULL) {
                // test if the scope filter matches the exported service properties
                status = filter_match(filter, serviceProperties, &found);
                if (found) {
                    struct scope_item *item = (struct scope_item *) hashMapEntry_getValue(scopedEntry);
                    *props = item->props;
                }
            }
        }
        hashMapIterator_destroy(scopedPropIter);
        celix_properties_destroy(serviceProperties);

        celixThreadMutex_unlock(&(scope->exportScopeLock));
    }

    return status;
}
