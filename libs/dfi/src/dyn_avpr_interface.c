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

#include "dyn_interface.h"
#include "dyn_interface_common.h"

#include "jansson.h"

#include <stdlib.h>
#include <string.h>

#include "dyn_common.h"
#include "dyn_type.h"
#include "dyn_function.h"

DFI_SETUP_LOG(dynAvprInterface)

#define STR_LENGTH 1024
#define FQN_SIZE 256
#define ARG_SIZE 32

// Section: function definitions
inline static dyn_interface_type * dynAvprInterface_initializeInterface();
inline static bool dynAvprInterface_createHeader(dyn_interface_type* intf, json_t * const root);
inline static bool dynAvprInterface_createAnnotations(dyn_interface_type* intf, json_t * const root);
inline static bool dynAvprInterface_createTypes(dyn_interface_type* intf, json_t * const root, const char* parent_ns);
inline static bool dynAvprInterface_createMethods(dyn_interface_type* intf, json_t * const root, const char* parent_ns);
static bool dynAvprInterface_insertNamValEntry(struct namvals_head *head, const char* name, const char* value);
static struct namval_entry * dynAvprInterface_createNamValEntry(const char* name, const char* value);

// Section: extern function definitions
ffi_type * dynType_ffiType(dyn_type *type);
dyn_type * dynAvprType_parseFromJson(json_t * const root, const char * fqn);
dyn_function_type * dynAvprFunction_parseFromJson(json_t * const root, const char * fqn);
int dynInterface_checkInterface(dyn_interface_type *intf);
void dynAvprType_constructFqn(char *destination, size_t size, const char *possibleFqn, const char *ns);

// Function definitions
dyn_interface_type * dynInterface_parseAvprWithStr(const char * avpr) {
    FILE *avprStream = fmemopen((char*)avpr, strlen(avpr), "r");

    if (!avprStream) {
        LOG_ERROR("interface_parseAvprWithStr: Error creating mem stream for descriptor string. %s", strerror(errno)); 
        return NULL;
    }

    dyn_interface_type *returnValue = dynInterface_parseAvpr(avprStream);
    fclose(avprStream);
    return returnValue;
}

dyn_interface_type * dynInterface_parseAvpr(FILE * avprStream) {
    dyn_interface_type *intf = NULL;
    bool valid = true;
    json_error_t error;
    json_t *root = json_loadf(avprStream, JSON_REJECT_DUPLICATES, &error);
    if (!root) {
        LOG_ERROR("interface_parseAvpr: Error decoding json: line %d: %s", error.line, error.text);
        valid = false;
    }

    if (valid && !json_is_object(root)) {
        LOG_ERROR("interface_parseAvpr: Error decoding json, root should be an object");
        valid = false;
    }

    const char *parent_ns = json_string_value(json_object_get(root, "namespace"));
    if (valid && !parent_ns) {
        LOG_ERROR("interface_parseAvpr: No namespace found in root, or it is null!");
        valid = false;
    }

    json_t * const messagesObject = json_object_get(root, "messages");
    if (valid && !json_is_object(messagesObject)) {
        LOG_ERROR("interface_parseAvpr: No messages, or it is not an object");
        valid = false;
    }

    if (valid) {
        intf = dynAvprInterface_initializeInterface();
        valid = intf != NULL;
    }

    valid = valid && dynAvprInterface_createHeader(intf, root);
    valid = valid && dynAvprInterface_createAnnotations(intf, root);
    valid = valid && dynAvprInterface_createTypes(intf, root, parent_ns);
    valid = valid && dynAvprInterface_createMethods(intf, root, parent_ns);

    valid = valid && 0 == dynInterface_checkInterface(intf);

    json_decref(root);
    if (valid) {
        LOG_DEBUG("Parsing of avpr interface successful");
        return intf;
    }
    else {
        LOG_ERROR("Parsing of avpr interface failed");
        dynInterface_destroy(intf);
        return NULL;
    }
}

inline static dyn_interface_type * dynAvprInterface_initializeInterface() {
    dyn_interface_type *intf = calloc(1, sizeof(*intf));

    TAILQ_INIT(&intf->header);
    TAILQ_INIT(&intf->annotations);
    TAILQ_INIT(&intf->types);
    TAILQ_INIT(&intf->methods);

    return intf;
}

inline static bool dynAvprInterface_createHeader(dyn_interface_type* intf, json_t * const root) {
    // type
    if (!dynAvprInterface_insertNamValEntry(&intf->header, "type", "interface")) {
        return false;
    }

    // name (protocol in avpr-speak)
    if (!dynAvprInterface_insertNamValEntry(&intf->header, "name", json_string_value(json_object_get(root, "protocol")))) {
        return false;
    }

    // version
    if (!dynAvprInterface_insertNamValEntry(&intf->header, "version", json_string_value(json_object_get(root, "version")))) {
        return false;
    }

    // Also parse version in the intf->version for cel
    char* version = NULL;
    dynInterface_getVersionString(intf, &version);
    if (!version) {
        LOG_ERROR("No version string in header");
        return false;
    }

    if (CELIX_SUCCESS != version_createVersionFromString(version, &(intf->version))) {
        LOG_ERROR("Invalid version (%s) in parsed descriptor\n", version);
        return false;
    }

    return true;
}

inline static bool dynAvprInterface_createAnnotations(dyn_interface_type* intf, json_t * const root) {
    // namespace
    if (!dynAvprInterface_insertNamValEntry(&intf->annotations, "namespace", json_string_value(json_object_get(root, "namespace")))) {
        return false;
    }

    // TODO: other annotations?

    return true;
}

inline static bool dynAvprInterface_createTypes(dyn_interface_type* intf, json_t * const root, const char* parent_ns) {
    json_t const * const types_array = json_object_get(root, "types");
    if (!types_array || !json_is_array(types_array)) {
        LOG_ERROR("json: types is not an array or it does not exists!");
        return false;
    }

    char name_buffer[FQN_SIZE];
    json_t const * entry;
    json_t const * local_ns;
    char const * name;
    size_t index;
    struct type_entry *t_entry = NULL;
    json_array_foreach(types_array, index, entry) {
        local_ns = json_object_get(entry, "namespace");
        name = json_string_value(json_object_get(entry, "name"));
        if (!name) {
            LOG_ERROR("Type entry %llu has no name", index);
            return false;
        }
        dynAvprType_constructFqn(name_buffer, FQN_SIZE, name, json_is_string(local_ns) ? json_string_value(local_ns) : parent_ns);

        t_entry = calloc(1, sizeof(*t_entry));
        t_entry->type = dynAvprType_parseFromJson(root, name_buffer);
        if (!t_entry->type) {
            free(t_entry);
            return false;
        }

        TAILQ_INSERT_TAIL(&intf->types, t_entry, entries);
    }

    return true;
}

inline static bool dynAvprInterface_createMethods(dyn_interface_type* intf, json_t * const root, const char* parent_ns) {
    json_t * const messages_object = json_object_get(root, "messages");
    if (!messages_object || !json_is_object(messages_object)) {
        LOG_ERROR("json: messages is not an object or it does not exist!");
        return false;
    }

    char name_buffer[FQN_SIZE];
    json_t const * func_entry;
    json_t const * local_ns;
    json_t const * j_index;
    char const * func_name;
    struct method_entry *m_entry = NULL;
    json_object_foreach(messages_object, func_name, func_entry) {
        local_ns = json_object_get(func_entry, "namespace");
        dynAvprType_constructFqn(name_buffer, FQN_SIZE, func_name, json_is_string(local_ns) ? json_string_value(local_ns) : parent_ns);

        m_entry = calloc(1, sizeof(*m_entry));
        m_entry->dynFunc = dynAvprFunction_parseFromJson(root, name_buffer);
        if (!m_entry->dynFunc) {
            free(m_entry);
            return false;
        }

        j_index = json_object_get(func_entry, "index");
        if (!json_is_integer(j_index)) {
            LOG_ERROR("For parsing an interface annotate all functions with an integer index");
            dynFunction_destroy(m_entry->dynFunc);
            free(m_entry);
            return false;
        }

        m_entry->index = json_integer_value(j_index);
        m_entry->id = strndup(func_name, STR_LENGTH);
        if (!m_entry->id) {
            LOG_ERROR("Could not allocate memory for method entry name %s", func_name);
            dynFunction_destroy(m_entry->dynFunc);
            free(m_entry);
            return false;
        }

        m_entry->name = strndup(func_name, STR_LENGTH); // For avpr these are the same
        if (!m_entry->id) {
            LOG_ERROR("Could not allocate memory for method entry id %s", func_name);
            dynFunction_destroy(m_entry->dynFunc);
            free(m_entry);
            return false;
        }
        TAILQ_INSERT_TAIL(&intf->methods, m_entry, entries);
    }

    // Parsed all methods
    return true;
}

static bool dynAvprInterface_insertNamValEntry(struct namvals_head *head, const char* name, const char* value) {
    struct namval_entry *entry = dynAvprInterface_createNamValEntry(name, value);
    if (!entry) {
        return false;
    }
    else {
        TAILQ_INSERT_TAIL(head, entry, entries);
        return true;
    }
}

static struct namval_entry * dynAvprInterface_createNamValEntry(const char* name, const char* value) {
    if (!name) {
        LOG_ERROR("Passed null-ptr to name of name-value entry creation function, unknown what type to create");
        return NULL;
    }

    if (!value) {
        LOG_ERROR("Received a name (%s) but no value, did the look-up for a value fail?", name);
        return NULL;
    }

    struct namval_entry *entry = calloc(1, sizeof(*entry));
    entry->name = strndup(name, STR_LENGTH);
    entry->value = strndup(value, STR_LENGTH);
    return entry;
}

