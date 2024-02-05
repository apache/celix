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
#include "dyn_common.h"
#include "dyn_type.h"
#include "dyn_interface_common.h"
#include "celix_err.h"
#include "celix_stdlib_cleanup.h"

#include <assert.h>
#include <stdlib.h>
#include <string.h>



static const int OK = 0;
static const int ERROR = 1;

static int dynInterface_checkInterface(dyn_interface_type* intf);
static int dynInterface_parseSection(celix_descriptor_t* desc, const char* secName, FILE* stream);
static int dynInterface_parseMethods(dyn_interface_type* intf, FILE* stream);

int dynInterface_parse(FILE* descriptor, dyn_interface_type** out) {
    int status = OK;

    celix_autoptr(dyn_interface_type) intf = calloc(1, sizeof(*intf));
    if (intf == NULL) {
        celix_err_pushf("Error allocating memory for dynamic interface");
        return ERROR;
    }

    TAILQ_INIT(&intf->header);
    TAILQ_INIT(&intf->annotations);
    TAILQ_INIT(&intf->types);
    TAILQ_INIT(&intf->methods);

    if ((status = celix_dynDescriptor_parse((celix_descriptor_t*)intf, descriptor, dynInterface_parseSection)) != OK) {
        return status;
    }

    if ((status = dynInterface_checkInterface(intf)) != OK) {
        return status;
    }

    *out = celix_steal_ptr(intf);
    return OK;
}

static int dynInterface_checkInterface(dyn_interface_type* intf) {
    struct method_entry* mEntry = NULL;
    TAILQ_FOREACH(mEntry, &intf->methods, entries) {
        const dyn_type* type = dynFunction_returnType(mEntry->dynFunc);
        int descriptor = dynType_descriptorType(type);
        if (descriptor != 'N') {
            celix_err_pushf("Parse Error. Got return type '%c' rather than 'N' (native int) for method %s (%d)",
                            descriptor, mEntry->id, mEntry->index);
            return ERROR;
        }
        const struct dyn_function_arguments_head* args = dynFunction_arguments(mEntry->dynFunc);
        const dyn_function_argument_type* first = TAILQ_FIRST(args);
        const dyn_function_argument_type* last = TAILQ_LAST(args, dyn_function_arguments_head);
        if (first == NULL || first->argumentMeta != DYN_FUNCTION_ARGUMENT_META__HANDLE) {
            celix_err_pushf("Parse Error. The first argument must be handle for method %s (%d)", mEntry->id, mEntry->index);
            return ERROR;
        }
        const dyn_function_argument_type* argEntry = NULL;
        TAILQ_FOREACH(argEntry, args, entries) {
            if (argEntry->argumentMeta == DYN_FUNCTION_ARGUMENT_META__HANDLE) {
                if (argEntry != first) {
                    celix_err_pushf("Parse Error. Handle argument is only allowed as the first argument for method %s (%d)",
                                    mEntry->id, mEntry->index);
                    return ERROR;
                }
            } else if (argEntry->argumentMeta == DYN_FUNCTION_ARGUMENT_META__OUTPUT ||
                       argEntry->argumentMeta == DYN_FUNCTION_ARGUMENT_META__PRE_ALLOCATED_OUTPUT) {
                if (argEntry != last) {
                    celix_err_pushf("Parse Error. Output argument is only allowed as the last argument for method %s (%d)",
                                    mEntry->id, mEntry->index);
                    return ERROR;
                }
            }
        }
    }

    return OK;
}

static int dynInterface_parseSection(celix_descriptor_t* desc, const char* secName, FILE* stream) {
    dyn_interface_type* intf = (dyn_interface_type*)desc;
    if (strcmp("methods", secName) != 0) {
        celix_err_pushf("unsupported section '%s'", secName);
        return ERROR;
    }
    return dynInterface_parseMethods(intf, stream);
}

static int dynInterface_parseMethods(dyn_interface_type* intf, FILE* stream) {
    int status = OK;

    //expected input (Name)=<Method>\n
    int peek = fgetc(stream);
    int index = 0;
    while (peek != ':' && peek != EOF) {
        ungetc(peek, stream);

        celix_autofree char* id = NULL;
        if ((status = dynCommon_parseNameAlsoAccept(stream, ".();[{}/", &id)) != OK) {
            return status;
        }

        if ((status = dynCommon_eatChar(stream, '=')) != OK) {
            return status;
        }

        celix_autoptr(dyn_function_type) func = NULL;
        if ((status = dynFunction_parse(stream, &intf->types, &func)) != OK) {
            return status;
        }

        if ((status = dynCommon_eatChar(stream, '\n')) != OK) {
            return status;
        }

        struct method_entry *entry = NULL;
        entry = calloc(1, sizeof(*entry));
        if (entry == NULL) {
            celix_err_pushf("Error allocating memory for method entry");
            return ERROR;
        }
        entry->index = index++;
        entry->id = celix_steal_ptr(id);
        entry->dynFunc = celix_steal_ptr(func);
        TAILQ_INSERT_TAIL(&intf->methods, entry, entries);

        peek = fgetc(stream);
    }
    if (peek != EOF) {
        ungetc(peek, stream);
    }

    return status;
}

void dynInterface_destroy(dyn_interface_type* intf) {
    if (intf != NULL) {
        struct method_entry* mInfo = TAILQ_FIRST(&intf->methods);
        while (mInfo != NULL) {
            struct method_entry* mTmp = mInfo;
            mInfo = TAILQ_NEXT(mInfo, entries);
            
            if (mTmp->id != NULL) {
                free(mTmp->id);
            }
            if (mTmp->dynFunc != NULL) {
                dynFunction_destroy(mTmp->dynFunc);
            }
            free(mTmp);
        }
        celix_dynDescriptor_destroy((celix_descriptor_t*)intf);
    }
}

const char* dynInterface_getName(const dyn_interface_type* intf) {
    const char* name = NULL;
    // celix_dynDescriptor_checkInterface ensures that the name is present
    (void)dynCommon_getEntryForHead(&intf->header, "name", &name);
    return name;
}

const celix_version_t* dynInterface_getVersion(const dyn_interface_type* intf){
    // celix_dynDescriptor_checkInterface ensures that version is present
    return intf->version;
}

const char* dynInterface_getVersionString(const dyn_interface_type* intf) {
    const char* version = NULL;
    // celix_dynDescriptor_checkInterface ensures that the version is present
    (void)dynCommon_getEntryForHead(&intf->header, "version", &version);
    return version;
}

int dynInterface_getHeaderEntry(const dyn_interface_type* intf, const char* name, const char** value) {
    return dynCommon_getEntryForHead(&intf->header, name, value);
}

int dynInterface_getAnnotationEntry(const dyn_interface_type* intf, const char* name, const char** value) {
    return dynCommon_getEntryForHead(&intf->annotations, name, value);
}

const struct methods_head* dynInterface_methods(const dyn_interface_type* intf) {
    return &intf->methods;
}

int dynInterface_nrOfMethods(const dyn_interface_type* intf) {
    struct method_entry* last = TAILQ_LAST(&intf->methods, methods_head);
    return last == NULL ? 0 : (last->index+1);
}

const struct method_entry* dynInterface_findMethod(const dyn_interface_type* intf, const char* id) {
    const struct method_entry* entry = NULL;
    TAILQ_FOREACH(entry, &intf->methods, entries) {
        if (strcmp(entry->id, id) == 0) {
            break;
        }
    }
    return entry;
}