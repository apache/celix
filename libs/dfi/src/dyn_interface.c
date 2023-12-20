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

static int dynInterface_checkInterface(dyn_interface_type *intf);
static int dynInterface_parseSection(dyn_interface_type *intf, FILE *stream);
static int dynInterface_parseAnnotations(dyn_interface_type *intf, FILE *stream);
static int dynInterface_parseTypes(dyn_interface_type *intf, FILE *stream);
static int dynInterface_parseMethods(dyn_interface_type *intf, FILE *stream);
static int dynInterface_parseHeader(dyn_interface_type *intf, FILE *stream);
static int dynInterface_getEntryForHead(const struct namvals_head *head, const char *name, const char **value);

int dynInterface_parse(FILE *descriptor, dyn_interface_type **out) {
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

    char peek = (char)fgetc(descriptor);
    while (peek == ':') {
        ungetc(peek, descriptor);
        if ((status = dynInterface_parseSection(intf, descriptor)) != OK) {
            return status;
        }
        peek = (char)fgetc(descriptor);
    }

    if (peek != EOF) {
        celix_err_pushf("Descriptor does not start with ':'");
        return ERROR;
    }

    if ((status = dynInterface_checkInterface(intf)) != OK) {
        return status;
    }

    // We are sure that version field is present in the header
    const char* version = dynInterface_getVersionString(intf);
    assert(version != NULL);
    intf->version = celix_version_createVersionFromString(version);
    if (intf->version == NULL) {
        celix_err_pushf("Invalid version (%s) in parsed descriptor\n",version);
        return ERROR;
    }
    *out = celix_steal_ptr(intf);
    return OK;
}

static int dynInterface_checkInterface(dyn_interface_type *intf) {
    //check header section
    bool foundType = false;
    bool foundVersion = false;
    bool foundName = false;
    struct namval_entry *entry = NULL;
    TAILQ_FOREACH(entry, &intf->header, entries) {
        if (strcmp(entry->name, "type") == 0) {
            foundType = true;
        } else if (strcmp(entry->name, "version") == 0) {
            foundVersion = true;
        } else if (strcmp(entry->name, "name") == 0) {
            foundName = true;
        }
    }

    if (!foundType || !foundVersion || !foundName) {
        celix_err_pushf("Parse Error. There must be a header section with a type, version and name entry");
        return ERROR;
    }

    struct method_entry *mEntry = NULL;
    TAILQ_FOREACH(mEntry, &intf->methods, entries) {
        dyn_type *type = dynFunction_returnType(mEntry->dynFunc);
        int descriptor = dynType_descriptorType(type);
        if (descriptor != 'N') {
            celix_err_pushf("Parse Error. Only method with a return type 'N' (native int) are supported. Got return type '%c'\n", descriptor);
            return ERROR;
        }
    }

    return OK;
}

static int dynInterface_parseSection(dyn_interface_type *intf, FILE *stream) {
    int status = OK;
    celix_autofree char *sectionName = NULL;

    if ((status = dynCommon_eatChar(stream, ':')) != OK) {
        return status;
    }

    if ((status = dynCommon_parseName(stream, &sectionName)) != OK) {
        return status;
    }

    if ((status = dynCommon_eatChar(stream, '\n')) != OK) {
        return status;
    }

    if (strcmp("header", sectionName) == 0) {
        status = dynInterface_parseHeader(intf, stream);
    } else if (strcmp("annotations", sectionName) == 0) {
        status = dynInterface_parseAnnotations(intf, stream);
    } else if (strcmp("types", sectionName) == 0) {
        status = dynInterface_parseTypes(intf, stream);
    } else if (strcmp("methods", sectionName) == 0) {
        status = dynInterface_parseMethods(intf, stream);
    } else {
        status = ERROR;
        celix_err_pushf("unsupported section '%s'", sectionName);
    }

    return status;
}

static int dynInterface_parseHeader(dyn_interface_type *intf, FILE *stream) {
    return dynCommon_parseNameValueSection(stream, &intf->header);
}

static int dynInterface_parseAnnotations(dyn_interface_type *intf, FILE *stream) {
    return dynCommon_parseNameValueSection(stream, &intf->annotations);
}

static int dynInterface_parseTypes(dyn_interface_type *intf, FILE *stream) {
    int status;

    //expected input (Name)=<Type>\n
    int peek = fgetc(stream);
    while (peek != ':' && peek != EOF) {
        ungetc(peek, stream);

        celix_autofree char* name = NULL;
        if ((status = dynCommon_parseName(stream, &name)) != OK) {
            return status;
        }

        if ((status = dynCommon_eatChar(stream, '=')) != OK) {
            return status;
        }

        celix_autoptr(dyn_type) type = NULL;
        if ((status = dynType_parse(stream, name, &intf->types, &type)) != OK) {
            return status;
        }
        if ((status = dynCommon_eatChar(stream, '\n')) != OK) {
            return status;
        }

        struct type_entry *entry = NULL;
        entry = calloc(1, sizeof(*entry));
        if (entry == NULL) {
            celix_err_pushf("Error allocating memory for type entry");
            return ERROR;
        }
        entry->type = celix_steal_ptr(type);
        TAILQ_INSERT_TAIL(&intf->types, entry, entries);

        peek = fgetc(stream);
    }
    if (peek != EOF) {
        ungetc(peek, stream);
    }

    return OK;
}

static int dynInterface_parseMethods(dyn_interface_type *intf, FILE *stream) {
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
        dynCommon_clearNamValHead(&intf->header);
        dynCommon_clearNamValHead(&intf->annotations);

        struct method_entry *mInfo = TAILQ_FIRST(&intf->methods);
        while (mInfo != NULL) {
            struct method_entry *mTmp = mInfo;
            mInfo = TAILQ_NEXT(mInfo, entries);
            
            if (mTmp->id != NULL) {
                free(mTmp->id);
            }
            if (mTmp->dynFunc != NULL) {
                dynFunction_destroy(mTmp->dynFunc);
            }
            free(mTmp);
        }

        struct type_entry *tInfo = TAILQ_FIRST(&intf->types);
        while (tInfo != NULL) {
            struct type_entry *tmp = tInfo;
            tInfo = TAILQ_NEXT(tInfo, entries);
            dynType_destroy(tmp->type);
            free(tmp);
        }

        if(intf->version!=NULL){
            celix_version_destroy(intf->version);
        }

        free(intf);
    } 
}

const char* dynInterface_getName(const dyn_interface_type* intf) {
    const char* name = NULL;
    // dynInterface_checkInterface ensures that the name is present
    (void)dynInterface_getEntryForHead(&intf->header, "name", &name);
    return name;
}

const celix_version_t* dynInterface_getVersion(const dyn_interface_type* intf){
    // dynInterface_checkInterface ensures that version is present
    return intf->version;
}

const char* dynInterface_getVersionString(const dyn_interface_type* intf) {
    const char* version = NULL;
    // dynInterface_checkInterface ensures that the version is present
    (void)dynInterface_getEntryForHead(&intf->header, "version", &version);
    return version;
}

int dynInterface_getHeaderEntry(const dyn_interface_type* intf, const char* name, const char** value) {
    return dynInterface_getEntryForHead(&intf->header, name, value);
}

int dynInterface_getAnnotationEntry(const dyn_interface_type *intf, const char *name, const char **value) {
    return dynInterface_getEntryForHead(&intf->annotations, name, value);
}

static int dynInterface_getEntryForHead(const struct namvals_head *head, const char *name, const char **out) {
    int status = OK;
    char *value = NULL;
    struct namval_entry *entry = NULL;
    TAILQ_FOREACH(entry, head, entries) {
        if (strcmp(name, entry->name) == 0) {
            value = entry->value;
            break;
        }
    }
    if (value != NULL) {
        *out = value;
    } else {
        status = ERROR;
        celix_err_pushf("Cannot find '%s' in list", name);
    }
    return status;
}

const struct methods_head* dynInterface_methods(const dyn_interface_type* intf) {
    return &intf->methods;
}

int dynInterface_nrOfMethods(const dyn_interface_type* intf) {
    int count = 0;
    struct method_entry *entry = NULL;
    TAILQ_FOREACH(entry, &intf->methods, entries) {
        count +=1;
    }
    return count;
}
