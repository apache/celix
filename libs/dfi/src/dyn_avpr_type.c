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

#include "dyn_type.h"

#include "jansson.h"

#include <ctype.h>
#include <stdlib.h>
#include <string.h>
#include <assert.h>
#include <errno.h>
#include <ffi.h>

#include "dyn_type.h"
#include "dyn_type_common.h"
#include "version.h"

DFI_SETUP_LOG(dynAvprType)

#define STR_LENGTH 1024
#define FQN_SIZE 256

enum JsonTypeType {INVALID=0, SIMPLE=1, ARRAY=2, REFERENCE=3, SELF_REFERENCE=4};

dyn_type * dynAvprType_parseFromJson(json_t * const root, const char * fqn);
dyn_type * dynAvprType_createNestedForFunction(dyn_type * store_type, const char* fqn);
dyn_type * dynAvprType_parseFromTypedJson(json_t * const root, json_t const * const type_entry, const char * namespace);
void dynAvprType_constructFqn(char* destination, int size, const char* possibleFqn, const char* ns);

// Any
static json_t const * dynAvprType_findType(char const * const fqn, json_t const * const array_object, const char * ns);
static dyn_type * dynAvprType_parseAny(dyn_type * root, dyn_type * parent, json_t const * const jsonObject, json_t const * const array_object, const char * parent_ns);
static dyn_type * dynAvprType_initializeType(dyn_type * parent);

// Record
static dyn_type * dynAvprType_parseRecord(dyn_type * root, dyn_type * parent, json_t const * const record_obj, json_t const * const array_object, const char* parent_ns, dyn_type * ref_type);
static inline dyn_type * dynAvprType_prepareRecord(dyn_type * parent, json_t const ** fields, json_t const * const record_obj);
static inline bool dynAvprType_finalizeRecord(dyn_type * type, const size_t size);
static inline struct complex_type_entry* dynAvprType_prepareRecordEntry(dyn_type * parent, json_t const * const entry_object);
static inline struct complex_type_entry * dynAvprType_parseRecordEntry(dyn_type * root, dyn_type * parent, json_t const * const entry_object, json_t const * const array_object, const char * fqn_parent, const char * parent_ns, const char * record_ns, dyn_type * ref_type);
static inline enum JsonTypeType dynAvprType_getRecordEntryType(json_t const * const entry_object, const char * fqn_parent, char * name_buffer, const char * namespace);

// Reference
static dyn_type * dynAvprType_parseReference(dyn_type * root, const char * name_buffer, json_t const * const array_object, const char * parent_ns);
static dyn_type * dynAvprType_parseSelfReference(dyn_type * root, dyn_type * parent, const char * name_buffer, json_t const * const array_object, const char * parent_ns, dyn_type * ref_type);
static dyn_type * dynAvprType_initializeReferenceType(dyn_type * type_pointer);
static inline struct type_entry * dynAvprType_prepareNestedEntry(dyn_type * const parent, const char * fqn, bool allocate);

// Enum
static dyn_type * dynAvprType_parseEnum(dyn_type * parent, json_t const * const enum_obj, const char * parent_ns);
static inline struct meta_entry * dynAvprType_createMetaEntry(const char* enum_entry_name);
static inline bool dynAvprType_metaEntrySetValue(struct meta_entry * meta_entry_ptr, json_t const * const values_array, size_t index);
static inline void dynAvprType_parseEnumValue(char** enumvalue);

// Array
static dyn_type * dynAvprType_parseArray(dyn_type * root, dyn_type * parent, json_t const * const array_entry_obj, json_t const * const array_object, char * name_buffer, const char * parent_ns, const char * record_ns);

// Fixed
static dyn_type * dynAvprType_parseFixed(dyn_type * parent, json_t const * const fixed_object, const char * parent_ns);
static inline int dynAvprType_fixedTypeToFfi(json_t const * const simple_obj, ffi_type ** output_ffi_type);

// General
static char * dynAvprType_createFqnFromJson(json_t const * const jsonObject, const char * namespace);
static inline enum JsonTypeType dynAvprType_getType(json_t const * const json_type);
static inline void dynAvprType_createVersionMetaEntry(dyn_type * type, json_t const * const root, json_t const * const array_object, const char* parent_ns);
static inline void dynAvprType_createAnnotationEntries(dyn_type * type, json_t const * const array_object, const char* parent_ns);

// fqn = fully qualified name
dyn_type * dynType_parseAvpr(FILE* avprStream, const char *fqn) {
    json_error_t error;
    json_t *root = json_loadf(avprStream, JSON_REJECT_DUPLICATES, &error);
    if (!root) {
        LOG_ERROR("parseAvpr: Error decoding json: line %d: %s", error.line, error.text);
        return NULL;
    }

    dyn_type * type = dynAvprType_parseFromJson(root, fqn);
    json_decref(root);
    return type;
}

dyn_type * dynType_parseAvprWithStr(const char* avpr, const char *fqn) {
    FILE *avprStream = fmemopen((char*)avpr, strlen(avpr), "r");

    if (!avprStream) {
        LOG_ERROR("parseAvprWithStr: Error creating mem stream for descriptor string. %s", strerror(errno)); 
        return NULL;
    }

    dyn_type *returnValue = dynType_parseAvpr(avprStream, fqn);
    fclose(avprStream);
    return returnValue;
}

// Based on the fqn passed in, try to find an entry in the array_object with that name, the current namespace is defined by the parameter ns
static json_t const * dynAvprType_findType(char const * const fqn, json_t const * const array_object, const char * ns) {
    char fqn_buffer[FQN_SIZE];
    size_t index;
    json_t const * type_entry;

    json_array_foreach(array_object, index, type_entry) {
        const char* entry_name = json_string_value(json_object_get(type_entry, "name"));
        if (entry_name) {
            const char* entry_ns = json_string_value(json_object_get(type_entry, "namespace"));
            snprintf(fqn_buffer, FQN_SIZE, "%s.%s", entry_ns ? entry_ns : ns, entry_name);
            if (strcmp(fqn, fqn_buffer) == 0) {
                return type_entry; // Found
            }
        }
        else {
            LOG_INFO("FindType: found a type with no name, check your json configuration, skipping this entry for now...");
        }
    }
    return NULL; // Not found
}

dyn_type * dynAvprType_parseFromJson(json_t * const root, const char * fqn) {
    // Initialize type
    bool valid = true;
    if (!json_is_object(root)) {
        LOG_ERROR("parseAvpr: Error decoding json, root should be an object");
        valid = false;
    }

    // Get base namespace
    const char *parent_ns = json_string_value(json_object_get(root, "namespace"));
    if (valid && !parent_ns) {
        LOG_ERROR("parseAvpr: No namespace found in root, or it is null!");
        valid = false;
    }

    // Get types array
    json_t* typesArrayObject = json_object_get(root, "types");
    if (valid && !json_is_array(typesArrayObject)) {
        LOG_ERROR("parseAvpr: types should be an array!");
        valid = false;
    }

    // Try to find the fqn and parse if it is found, if any error occurs during parsing, return NULL
    dyn_type * type = NULL;
    if (valid) {
        type = dynAvprType_parseAny(NULL, NULL, dynAvprType_findType(fqn, typesArrayObject, parent_ns), typesArrayObject, parent_ns);
        valid = type != NULL;
    }

    // Add version as a meta entry
    if (valid) {
    	LOG_DEBUG("parseAvpr: Parsing successful, adding version entry and annotations");
        dynAvprType_createVersionMetaEntry(type, root, typesArrayObject, parent_ns);
        // Add any other annotation fields that are not in [type, name, fields, version, alias]
        dynAvprType_createAnnotationEntries(type, typesArrayObject, parent_ns);

    }

    if (valid) {
        LOG_DEBUG("Found %s", fqn);
    }
    else {
        LOG_ERROR("Not found %s", fqn);
    }

    return type;
}

dyn_type * dynAvprType_createNestedForFunction(dyn_type * store_type, const char* fqn) {
    if (!store_type) {
        return NULL;
    }

    // Created dyn_type for reference argument in function
    dyn_type * entry_ref_type = dynAvprType_initializeType(NULL);
    if (!entry_ref_type) {
        dynType_destroy(store_type);
        return NULL;
    }

    // Create pointer type and enter type in nested types
    struct type_entry *entry = dynAvprType_prepareNestedEntry(entry_ref_type, fqn, false);
    if (!entry) {
        free(entry_ref_type);
        return NULL;
    }

    entry->type = store_type;
    entry_ref_type->type = DYN_TYPE_TYPED_POINTER;
    entry_ref_type->descriptor = '*';
    entry_ref_type->ffiType = &ffi_type_pointer;

    // Create reference type for the sub type
    dyn_type * sub_type = dynAvprType_initializeType(entry_ref_type);
    if (!sub_type) {
        dynType_destroy(store_type);
        free(entry_ref_type);
        free(entry);
        return NULL;
    }

    entry_ref_type->typedPointer.typedType = sub_type;
    sub_type->type = DYN_TYPE_REF;
    sub_type->descriptor = 'l';
    sub_type->ref.ref = entry->type;

    return entry_ref_type;
}

// To be used for dyn_function parsing of return and parameter arrays
dyn_type * dynAvprType_parseFromTypedJson(json_t * const root, json_t const * const type_entry, const char * namespace) {
    if (!type_entry) {
        LOG_ERROR("Need a type entry to parse, but got a NULL pointer");
        return NULL;
    }

    dyn_type * ret_type = NULL;
    enum JsonTypeType elType = dynAvprType_getType(type_entry);
    char name_buffer[FQN_SIZE];
    switch (elType) {
        case SIMPLE:
            dynAvprType_constructFqn(name_buffer, FQN_SIZE, json_string_value(type_entry), namespace);
            ret_type = dynAvprType_parseFromJson(root, name_buffer);
            break;

        case ARRAY:
            ret_type = dynAvprType_parseArray(ret_type, NULL, type_entry, json_object_get(root, "types"), name_buffer, namespace, namespace);
            if (!ret_type) {
                LOG_ERROR("Error allocating memory for type");
                return NULL;
            }
            break;

        case INVALID:
            /* fall through */
        case REFERENCE:
            /* fall through */
        default:
            LOG_ERROR("ParseFromTypedJson: Received illegal type : %d", elType);
            break;
    }

    return ret_type;
}

void dynAvprType_constructFqn(char* destination, int size, const char* possibleFqn, const char* ns) {
    const bool isAlreadyFqn = strchr(possibleFqn, '.') != NULL;
    snprintf(destination, size, "%s%s%s", isAlreadyFqn ? "" : ns, isAlreadyFqn ? "" : ".", possibleFqn);
}

static dyn_type * dynAvprType_parseAny(dyn_type * root, dyn_type * parent, json_t const * const jsonObject, json_t const * const array_object, const char * parent_ns) {
    if (!jsonObject) {
        LOG_WARNING("Any: Received NULL, type not found, nothing to parse");
        return NULL;
    }

    const char* type_name = json_string_value(json_object_get(jsonObject, "type"));
    if (!type_name) {
        LOG_ERROR("Any: \"type\" entry is not a string or does not exist");
        return NULL;
    }

    dyn_type * type = NULL;
    LOG_DEBUG("Any: Parsing a %s", type_name);
    if (strcmp(type_name, "record") == 0) {
        type = dynAvprType_parseRecord(root, parent, jsonObject, array_object, parent_ns, NULL);
    }
    else if (strcmp(type_name, "fixed") == 0) {
        type = dynAvprType_parseFixed(parent, jsonObject, parent_ns);
    }
    else if (strcmp(type_name, "enum") == 0) {
        type = dynAvprType_parseEnum(parent, jsonObject, parent_ns);
    }
    else {
        LOG_ERROR("Any: Unrecognized type: %s", type_name);
    }

    return type;
}

static dyn_type * dynAvprType_initializeType(dyn_type * parent) {
    dyn_type * type = calloc(1, sizeof(*type));
    TAILQ_INIT(&type->nestedTypesHead);
    TAILQ_INIT(&type->metaProperties);
    type->parent = parent;
    return type;
}

static dyn_type * dynAvprType_parseRecord(dyn_type * root, dyn_type * parent, json_t const * const record_obj, json_t const * const array_object, const char * parent_ns, dyn_type * ref_type) {
    json_t const * fields = NULL;
    dyn_type * type = dynAvprType_prepareRecord(parent, &fields, record_obj); // also sets fields correctly
    if (!type) {
        return NULL;
    }

    // Check if namespace is present, otherwise set it equal to parent
    const char* record_ns = json_string_value(json_object_get(record_obj, "namespace"));
    if (!record_ns) {
        record_ns = parent_ns;
    }

    // Create namespace + name for storage in type->name (to preserve namespacing)
    char fqn_buffer[FQN_SIZE];
    snprintf(fqn_buffer, FQN_SIZE, "%s.%s", record_ns, json_string_value(json_object_get(record_obj, "name")));
    type->name = strndup(fqn_buffer, STR_LENGTH);
    if (!type->name) {
        LOG_ERROR("Record: failed to allocate memory for type->name");
        dynType_destroy(type);
        return NULL;
    }

    // If root is NULL, this is the root type
    if (!root) {
        root = type;
    }

    // Loop over json_fields and parse each entry
    size_t counter;
    json_t *element;
    json_array_foreach(fields, counter, element) {
        LOG_DEBUG("Record: parsing field [%s](%s) at %d", json_string_value(json_object_get(element, "type")), json_string_value(json_object_get(element, "name")), counter);

        struct complex_type_entry * entry = dynAvprType_parseRecordEntry(root, type, element, array_object, fqn_buffer, parent_ns, record_ns, ref_type);
        if (!entry) {
            LOG_ERROR("Record: Parsing record entry %d failed", counter);
            dynType_destroy(type);
            return NULL;
        }

        TAILQ_INSERT_TAIL(&type->complex.entriesHead, entry, entries);
    }

    return dynAvprType_finalizeRecord(type, counter) ? type : NULL;
}

// Initializes the type pointer with the correct information for a record (complex type)
static inline dyn_type * dynAvprType_prepareRecord(dyn_type * parent, json_t const ** fields, json_t const * const record_obj) {
    const char * name = json_string_value(json_object_get(record_obj, "name"));
    if (!name) {
        LOG_ERROR("Record: \"name\" entry is missing");
        return NULL;
    }

    *fields = json_object_get(record_obj, "fields");
    if (!json_is_array(*fields)) {
        LOG_WARNING("Record: \"fields\" is not an array or does not exist");
        return NULL;
    }

    dyn_type * type = dynAvprType_initializeType(parent);
    if (!type) {
        return NULL;
    }

    type->type = DYN_TYPE_COMPLEX;
    type->descriptor = '{';
    type->ffiType = &type->complex.structType;
    type->complex.structType.type = FFI_TYPE_STRUCT;
    TAILQ_INIT(&type->complex.entriesHead);
    return type;
}

// Create the ffi types for quick access and link the dyn_types
static inline bool dynAvprType_finalizeRecord(dyn_type * type, const size_t size) {
    type->complex.structType.elements = calloc(size+1, sizeof(ffi_type*));
    type->complex.structType.elements[size] = NULL;
    struct complex_type_entry *entry = NULL;
    int index = 0;
    TAILQ_FOREACH(entry, &type->complex.entriesHead, entries) {
        type->complex.structType.elements[index++] = dynType_ffiType(entry->type);
    }

    // set types for every element for access
    index = 0;
    type->complex.types = calloc(size, sizeof(dyn_type*));
    TAILQ_FOREACH(entry, &type->complex.entriesHead, entries) {
        type->complex.types[index++] = entry->type;
    }

    dynType_prepCif(type->ffiType);
    return true;
}

static inline struct complex_type_entry * dynAvprType_parseRecordEntry(dyn_type * root, dyn_type * parent, json_t const * const entry_object, json_t const * const array_object, const char * fqn_parent, const char * parent_ns, const char * record_ns, dyn_type * ref_type) {
    struct complex_type_entry *entry = dynAvprType_prepareRecordEntry(parent, entry_object);
    if (!entry) {
        return NULL;
    }

    char name_buffer[FQN_SIZE];
    switch (dynAvprType_getRecordEntryType(entry_object, fqn_parent, name_buffer, record_ns)) {
        case SIMPLE:
                LOG_DEBUG("RecordEntry: Looking for type: %s", name_buffer);
                entry->type = dynAvprType_parseAny(root, parent, dynAvprType_findType(name_buffer, array_object, parent_ns), array_object, parent_ns);
                break;

        case REFERENCE:
                LOG_DEBUG("RecordEntry: Found a ptr type, creating a reference %s", name_buffer);
                entry->type = dynAvprType_parseReference(root, name_buffer, array_object, parent_ns);
                break;

        case SELF_REFERENCE:
                LOG_DEBUG("RecordEntry: Creating a self_reference for %s", name_buffer);
                entry->type = dynAvprType_parseSelfReference(root, parent, name_buffer, array_object, parent_ns, ref_type);
                break;

        case ARRAY:
                LOG_DEBUG("RecordEntry: Parsing array: %s", name_buffer);
                entry->type = dynAvprType_parseArray(root, parent, json_object_get(entry_object, "type"), array_object, name_buffer, parent_ns, record_ns);
                break;

        case INVALID:
            /* fall through */
        default:
                LOG_ERROR("RecordEntry: Illegal record entry for %s", json_string_value(json_object_get(entry_object, "name")));
                entry->type = NULL;
                break;
    }

    if (entry->type) {
        return entry;
    }
    else {
        free(entry->name);
        free(entry);
        return NULL;
    }
}

static inline struct complex_type_entry* dynAvprType_prepareRecordEntry(dyn_type * parent, json_t const * const entry_object) {
    struct complex_type_entry *entry = NULL;

    const char * entry_name = json_string_value(json_object_get(entry_object, "name"));
    if (!entry_name) {
        LOG_ERROR("Could not find name in entry");
        return NULL;
    }

    entry = calloc(1, sizeof(*entry));
    entry->name = strndup(entry_name, STR_LENGTH);
    return entry;
}

// Returns correct type and sets the fqn in the buffer if it is a simple type (not for array because arrays can be nested 
static inline enum JsonTypeType dynAvprType_getRecordEntryType(json_t const * const entry_object, const char * fqn_parent, char * name_buffer, const char * namespace) {
    json_t const * const json_type = json_object_get(entry_object, "type");
    if (!json_type) {
        LOG_WARNING("RecordEntry: No type entry");
        return INVALID;
    }

    enum JsonTypeType elType = dynAvprType_getType(json_type);
    if (elType == SIMPLE) {
        dynAvprType_constructFqn(name_buffer, FQN_SIZE, json_string_value(json_type), namespace);
        if (strcmp(name_buffer, fqn_parent) == 0) {
            elType = SELF_REFERENCE;
        } else if (json_is_true(json_object_get(entry_object, "ptr"))) {
            elType = REFERENCE;
        }
    }

    return elType;
}

static dyn_type * dynAvprType_parseReference(dyn_type * root, const char * name_buffer, json_t const * const array_object, const char * parent_ns) {
    dyn_type * type = dynAvprType_initializeType(NULL);
    if (!type) {
        return NULL;
    }

    dyn_type *subType = dynAvprType_initializeReferenceType(type);
    if (!subType) {
        free(type);
        return NULL;
    }

    // First try to find if it already exists
    LOG_DEBUG("Reference: looking for %s", name_buffer);
    dyn_type *ref = dynType_findType(root, (char*) name_buffer);
    if (ref) {
        if (ref->type == DYN_TYPE_INVALID) {
            LOG_ERROR("Reference: found incomplete type");
            free(type);
            return NULL;
        }
        else {
            subType->ref.ref = ref;
            return type;
        }
    }

    // if not found, Generate the reference type in root
    LOG_DEBUG("Reference: looking for %s in array", name_buffer);
    json_t const * const refType = dynAvprType_findType(name_buffer, array_object, parent_ns);
    if (!refType) {
        LOG_ERROR("ParseReference: Could not find %s", name_buffer);
        free(subType);
        free(type);
        return NULL;
    }

    bool success = true;
    struct type_entry *entry = dynAvprType_prepareNestedEntry(root, name_buffer, true);
    dyn_type * tmp = entry->type;
    if (!entry) {
        LOG_ERROR("Reference: could not create nested entry");
        success = false;
    }

    if (success) {
        entry->type = dynAvprType_parseAny(root, NULL, refType, array_object, parent_ns); 
        if (!entry->type) {
            free(entry);
            success = false;
        }
    }

    free(tmp->name);
    free(tmp);

    if (success) {
        LOG_DEBUG("Added new type to root:");
        subType->ref.ref = entry->type;
        return type;
    }
    else {
        dynType_destroy(subType);
        dynType_destroy(type);
        return NULL;
    }
}

static dyn_type * dynAvprType_parseSelfReference(dyn_type * root, dyn_type * parent, const char * name_buffer, json_t const * const array_object, const char * parent_ns, dyn_type * ref_type) {
    dyn_type * type = dynAvprType_initializeType(parent);
    if (!type) {
        return NULL;
    }

    dyn_type * sub_type = dynAvprType_initializeReferenceType(type);
    if (!sub_type) {
        free(type);
        return NULL;
    }

    sub_type->ref.ref = parent;
    return type;
}

// Assumes type_pointer is already present and alloc'd!
static dyn_type * dynAvprType_initializeReferenceType(dyn_type * type_pointer) {
    type_pointer->type = DYN_TYPE_TYPED_POINTER;
    type_pointer->descriptor = '*';
    type_pointer->ffiType = &ffi_type_pointer;

    dyn_type *sub_type = dynAvprType_initializeType(type_pointer);
    if (!sub_type) {
        LOG_ERROR("Could not create reference type");
        return NULL;
    }

    type_pointer->typedPointer.typedType = sub_type;
    sub_type->type = DYN_TYPE_REF;
    sub_type->descriptor = 'l';
    return sub_type;

}

static inline struct type_entry * dynAvprType_prepareNestedEntry(dyn_type * const parent, const char * fqn, bool allocate) {
    struct type_entry *entry = calloc(1, sizeof(*entry));
    if (allocate) {
        entry->type = calloc(1, sizeof(*entry->type));
        entry->type->name = strndup(fqn, STR_LENGTH);

        // Initialize information
        entry->type->parent = parent;
        entry->type->type = DYN_TYPE_INVALID;
        TAILQ_INIT(&entry->type->nestedTypesHead);
        TAILQ_INIT(&entry->type->metaProperties);
    }
    
    TAILQ_INSERT_TAIL(&parent->nestedTypesHead, entry, entries);
    return entry;
}

static dyn_type * dynAvprType_parseEnum(dyn_type * parent, json_t const * const enum_obj, const char * parent_ns) {
    dyn_type * type = dynAvprType_initializeType(parent);
    if (!type) {
        return NULL;
    }

    char * fqn = dynAvprType_createFqnFromJson(enum_obj, parent_ns);
    if (!fqn) {
        free(type);
        return NULL;
    }

    json_t * symbolArray = json_object_get(enum_obj, "symbols");
    if (!json_is_array(symbolArray)) {
        LOG_WARNING("Expected \"symbols\" to be an array for enum %s", fqn);
        free(fqn);
        free(type);
        return NULL;
    }

    json_t * values_array = json_object_get(enum_obj, "EnumValues");
    if (json_is_array(values_array) && json_array_size(values_array) != json_array_size(symbolArray)) {
        LOG_WARNING("Expected \"symbols\" and \"EnumValues\" arrays to be of the same size");
        free(fqn);
        free(type);
        return NULL;
    }

    const size_t max = json_array_size(symbolArray);
    bool valid = true;
    size_t counter = 0;
    json_t *enumeta_entry_ptr;
	for (; valid && counter < max && (enumeta_entry_ptr = json_array_get(symbolArray, counter)); counter++) {
        struct meta_entry * meta_entry_ptr = dynAvprType_createMetaEntry(json_string_value(enumeta_entry_ptr));
        if (meta_entry_ptr && dynAvprType_metaEntrySetValue(meta_entry_ptr, values_array, counter)) {
            LOG_DEBUG("Added enum entry with name: %s and value: %s", meta_entry_ptr->name, meta_entry_ptr->value);
            TAILQ_INSERT_TAIL(&type->metaProperties, meta_entry_ptr, entries);
        }
        else {
            valid = false;
        }
    }

    if (valid) {
        type->name = fqn;
        type->type = DYN_TYPE_SIMPLE;
        type->descriptor = 'E';
        type->ffiType = &ffi_type_sint32;
        return type;
    }
    else {
        free(fqn);
        dynType_destroy(type);
        return NULL;
    }
}

static inline struct meta_entry * dynAvprType_createMetaEntry(const char* enum_entry_name) {
    if (!enum_entry_name) {
        LOG_ERROR("CreateMetaEntry: passed null pointer, no name present?");
        return NULL;
    }

    struct meta_entry *entry = calloc(1, sizeof(*entry));
    entry->name = strndup(enum_entry_name, STR_LENGTH);
    return entry;
}

static inline bool dynAvprType_metaEntrySetValue(struct meta_entry * meta_entry_ptr, json_t const * const values_array, size_t index) {
    if (!meta_entry_ptr) {
        return false;
    }

    if (values_array) {
        const char* enumValue = json_string_value(json_array_get(values_array, index));
        if (!enumValue) {
            LOG_WARNING("MetaEntrySetValue: could not get the enum value from the \"EnumValues\" list at index %lu", index);
            return false;
        }

        meta_entry_ptr->value = strndup(enumValue, STR_LENGTH);
        if (!meta_entry_ptr->value) {
            LOG_ERROR("MetaEntrySetValue: could not allocate memory for meta_entry->value");
            return false;
        }
        dynAvprType_parseEnumValue(&meta_entry_ptr->value);
    }
    else {
        const int bufsize = sizeof(index) * CHAR_BIT + 1;
        meta_entry_ptr->value = calloc(bufsize, sizeof(char));

        snprintf(meta_entry_ptr->value, bufsize, "%lu", index);
    }

    return true;
}

static inline void dynAvprType_parseEnumValue(char** enumvalue) {
    char* str = *enumvalue;
    if (!str) {
        LOG_ERROR("Do not pass null pointer to this function");
        return;
    }

    // Remove whitespace
    int i, j = 0;
    for (i = 0; str[i] != '\0'; i++) {
        if (!isspace((unsigned char) str[i])) {
            str[j++] = str[i];
        }
    }
    str[j] = '\0';

    //create the substring if an equals sign is found
    char* equalssign = strrchr(str, '=');
    if (equalssign) {
        memmove(str, equalssign+1, strlen(equalssign-1));
    }
}

static ffi_type *seq_types[] = {&ffi_type_uint32, &ffi_type_uint32, &ffi_type_pointer, NULL};

static dyn_type * dynAvprType_parseArray(dyn_type * root, dyn_type * parent, json_t const * const array_entry_obj, json_t const * const array_object, char * name_buffer, const char * parent_ns, const char * record_ns) {
    dyn_type * type = dynAvprType_initializeType(parent);
    if (!type) {
        return NULL;
    }

    type->type = DYN_TYPE_SEQUENCE;
    type->descriptor = '[';

    type->sequence.seqType.elements = seq_types;
    type->sequence.seqType.type = FFI_TYPE_STRUCT;
    type->sequence.seqType.size = 0;
    type->sequence.seqType.alignment = 0;
    type->sequence.itemType = NULL;

    json_t const * const itemsEntry = json_object_get(array_entry_obj, "items");
    if (json_is_object(itemsEntry)) { // is nested?
        LOG_DEBUG("ParseArray: Found nested array");
        json_t const * const nested_array_object = json_object_get(array_entry_obj, "items");
        type->sequence.itemType = dynAvprType_parseArray(root, type, nested_array_object, array_object, name_buffer, parent_ns, record_ns);
    }
    else if (json_is_string(itemsEntry)) { // is a named item?
        LOG_DEBUG("ParseArray: Found type, parsing");
        dynAvprType_constructFqn(name_buffer, FQN_SIZE, json_string_value(itemsEntry), record_ns);
        type->sequence.itemType = dynAvprType_parseAny(root, type, dynAvprType_findType(name_buffer, array_object, parent_ns), array_object, parent_ns);
    }
    else {
        LOG_ERROR("ParseArray: Unrecognized array structure: \"items\" is neither a nested array nor a type");
    }

    if (type->sequence.itemType) {
        type->ffiType = &type->sequence.seqType;
        dynType_prepCif(&type->sequence.seqType);
    }
    else {
        free(type);
        type = NULL;
    }

    return type;
}

static dyn_type * dynAvprType_parseFixed(dyn_type * parent, json_t const * const fixed_object, const char * parent_ns) {
    dyn_type * type = dynAvprType_initializeType(parent);
    if (!type) {
        return NULL;
    }

    ffi_type * ffiType = NULL;
    int descriptor = dynAvprType_fixedTypeToFfi(fixed_object, &ffiType);

    if (!ffiType || descriptor == '0') {
        LOG_ERROR("Invalid simple type, received wrong ffi type!");
        free(type);
        return NULL;
    }

    if (!json_is_string(json_object_get(fixed_object, "name"))) {
        LOG_ERROR("Invalid simple type, need a name");
        free(type);
        return NULL;
    }

    type->name = dynAvprType_createFqnFromJson(fixed_object, parent_ns);
    if (!type->name) {
        free(type);
        return NULL;
    }

    type->type = (descriptor == 't') ? DYN_TYPE_TEXT : DYN_TYPE_SIMPLE;
    type->descriptor = descriptor;
    type->ffiType = ffiType;
    return type;
}

static inline int dynAvprType_fixedTypeToFfi(json_t const * const simple_obj, ffi_type ** output_ffi_type) {
    const int size = json_integer_value(json_object_get(simple_obj, "size"));
    const bool sign = json_is_true(json_object_get(simple_obj, "signed"));
    const char * alias = json_string_value(json_object_get(simple_obj, "alias"));
    const int s_size = sign ? size : -size;

    LOG_DEBUG("Simple fixed type: size = %d, signed = %s, alias = %s", size, sign ? "true" : "false", alias ? alias : "none");

    if (!alias) {
        switch (s_size) {
            case 1:
                *output_ffi_type = &ffi_type_sint8;
                return 'B';
            case 2:
                *output_ffi_type = &ffi_type_sint16;
                return 'S';
            case 4:
                *output_ffi_type = &ffi_type_sint32;
                return 'I';
            case 8:
                *output_ffi_type = &ffi_type_sint64;
                return 'J';
            case -1:
                *output_ffi_type = &ffi_type_uint8;
                return 'b';
            case -2:
                *output_ffi_type = &ffi_type_uint16;
                return 's';
            case -4:
                *output_ffi_type = &ffi_type_uint32;
                return 'i';
            case -8:
                *output_ffi_type = &ffi_type_uint64;
                return 'j';
            default:
                LOG_ERROR("Unrecognized size = %d", size);
                return '0';
        }
    
    }
    else {
        if (strcmp(alias, "float") == 0) {
            *output_ffi_type = &ffi_type_float;
            return 'F';
        }
        else if (strcmp(alias, "double") == 0) {
            *output_ffi_type = &ffi_type_double;
            return 'D';
        }
        else if (strcmp(alias, "string") == 0) {
            *output_ffi_type = &ffi_type_pointer;
            return 't';
        }
        else if (strcmp(alias, "boolean") == 0) {
            *output_ffi_type = &ffi_type_uint8;
            return 'Z';
        }
        else if (strcmp(alias, "void") == 0) {
            *output_ffi_type = &ffi_type_void;
            return 'V';
        }
        else if (strcmp(alias, "void_ptr") == 0) {
            *output_ffi_type = &ffi_type_pointer;
            return 'P';
        }
        else if (strcmp(alias, "native_int") == 0) {
            *output_ffi_type = &ffi_type_sint;
            return 'N';
        }
        else {
            LOG_ERROR("Unrecognized alias = %s", alias);
            return '0';
        }
    }
}

static char * dynAvprType_createFqnFromJson(json_t const * const jsonObject, const char * namespace) {
    const char *name = json_string_value(json_object_get(jsonObject, "name"));
    if (!name) {
        LOG_ERROR("This object appears to not have a name!!");
        return NULL;
    }

    const bool local_ns = json_is_string(json_object_get(jsonObject, "namespace"));
    const char *ns =  local_ns ? json_string_value(json_object_get(jsonObject, "namespace")) : namespace;
    if (!ns) {
        LOG_ERROR("Need a namespace because the jsonObject does not have a namespace!");
        return NULL;
    }

    // Create fqn
    const int length = strlen(ns) + strlen(name) + 2;
    char * fqn = calloc(length, sizeof(char));

    snprintf(fqn, length, "%s.%s", ns, name);
    return fqn;
}

static inline enum JsonTypeType dynAvprType_getType(json_t const * const json_type) {
    if (json_is_string(json_type)) {
        return SIMPLE;
    }
    else if (json_is_object(json_type) && json_is_string(json_object_get(json_type, "type")) && 0 == strcmp("array", json_string_value(json_object_get(json_type, "type")))) {
        return ARRAY;
    }
    else {
        return INVALID;
    }
}

static inline void dynAvprType_createVersionMetaEntry(dyn_type * type, json_t const * const root, json_t const * const array_object, const char* parent_ns) {
    struct meta_entry * m_entry = dynAvprType_createMetaEntry("version");

    json_t const * json_type_version = json_object_get(dynAvprType_findType(dynType_getName(type), array_object, parent_ns), "version");

    // If version is not available check root version
    if (!json_is_string(json_type_version)) {
        json_type_version = json_object_get(root, "version");
    }

    // If version is available use it (either from record or root)
    if (json_is_string(json_type_version)) {
        const char* version_str = json_string_value(json_type_version);
        // Check if a valid version was given
        version_pt v = NULL;
        celix_status_t status = version_createVersionFromString(version_str, &v);
        if (CELIX_SUCCESS == status) {
            m_entry->value = strndup(version_str, STR_LENGTH);
            version_destroy(v);
        }
        else {
            m_entry->value = strndup("0.0.0", STR_LENGTH);
            LOG_WARNING("parseAvpr: Did not find valid version, set version to 0.0.0");
        }
    }
    else {
        // If no version or an invalid version is available, default to 0.0.0
        m_entry->value = strndup("0.0.0", STR_LENGTH);
        LOG_WARNING("parseAvpr: Did not find version entry, set version to 0.0.0");
    }

    // Insert into type
    TAILQ_INSERT_TAIL(&type->metaProperties, m_entry, entries);
}

static inline void dynAvprType_createAnnotationEntries(dyn_type * type, json_t const * const array_object, const char* parent_ns) {
    json_t const * json_type = dynAvprType_findType(dynType_getName(type), array_object, parent_ns);
    const char* const not_of_interest[5] = {"type", "name", "fields", "version", "alias"};
    const char* key;
    json_t const * value;
    json_object_foreach((json_t*)json_type, key, value) {
        if (! ( strcmp(key, not_of_interest[0]) == 0
             || strcmp(key, not_of_interest[1]) == 0
             || strcmp(key, not_of_interest[2]) == 0
             || strcmp(key, not_of_interest[3]) == 0
             || strcmp(key, not_of_interest[4]) == 0))
        {
            struct meta_entry * m_entry = dynAvprType_createMetaEntry(key);

            switch (json_typeof(value)) {
                case JSON_STRING:
                    m_entry->value = strndup(json_string_value(value), STR_LENGTH);
                    break;
                case JSON_INTEGER:
                    asprintf(&m_entry->value, "%" JSON_INTEGER_FORMAT, json_integer_value(value));
                    break;
                case JSON_REAL:
                    asprintf(&m_entry->value, "%f", json_real_value(value));
                    break;
                case JSON_TRUE:
                    m_entry->value = strdup("true");
                    break;
                case JSON_FALSE:
                    m_entry->value = strdup("false");
                    break;
                case JSON_NULL:
                    m_entry->value = strdup("null");
                    break;
                case JSON_OBJECT:
                    /* fall through */
                case JSON_ARRAY:
                    /* fall through */
                default:
                    LOG_WARNING("createAnnotationEntries: encountered an annotation of an unknown type : %s", key);
                    free(m_entry->name);
                    free(m_entry);
                    continue; // Do not add the invalid entry as a meta-entry
            }

            TAILQ_INSERT_TAIL(&type->metaProperties, m_entry, entries);
        }
    }
}
