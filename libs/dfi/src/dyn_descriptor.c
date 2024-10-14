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

#include "dyn_descriptor.h"
#include "celix_err.h"
#include "celix_stdlib_cleanup.h"

#include <assert.h>

static const int OK = 0;
static const int ERROR = 1;

static int celix_dynDescriptor_parseTypes(celix_descriptor_t* descriptor, FILE *stream) {
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
        if ((status = dynType_parseOfName(stream, celix_steal_ptr(name), &descriptor->types, &type)) != OK) {
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
        TAILQ_INSERT_TAIL(&descriptor->types, entry, entries);

        peek = fgetc(stream);
    }
    if (peek != EOF) {
        ungetc(peek, stream);
    }

    return OK;
}

static int celix_dynDescriptor_parseSection(celix_descriptor_t* descriptor, FILE *stream,
                                            int (*parseSection)(celix_descriptor_t* descriptor, const char* secName, FILE *stream)) {
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
        status = dynCommon_parseNameValueSection(stream, &descriptor->header);
    } else if (strcmp("annotations", sectionName) == 0) {
        status = dynCommon_parseNameValueSection(stream, &descriptor->annotations);
    } else if (strcmp("types", sectionName) == 0) {
        status = celix_dynDescriptor_parseTypes(descriptor, stream);
    } else {
        status = parseSection(descriptor, sectionName, stream);
    }

    return status;
}

static int celix_dynDescriptor_checkInterface(celix_descriptor_t* descriptor) {
    //check header section
    bool foundType = false;
    bool foundVersion = false;
    bool foundName = false;
    struct namval_entry *entry = NULL;
    TAILQ_FOREACH(entry, &descriptor->header, entries) {
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

    return OK;
}

int celix_dynDescriptor_parse(celix_descriptor_t* descriptor, FILE* stream,
                              int (*parseSection)(celix_descriptor_t* descriptor, const char* secName, FILE *stream)) {
    int status = OK;

    int peek = fgetc(stream);
    while (peek == ':') {
        ungetc(peek, stream);
        if ((status = celix_dynDescriptor_parseSection(descriptor, stream, parseSection)) != OK) {
            return status;
        }
        peek = fgetc(stream);
    }

    if (peek != EOF) {
        celix_err_pushf("Descriptor does not start with ':'");
        return ERROR;
    }

    if ((status = celix_dynDescriptor_checkInterface(descriptor)) != OK) {
        return status;
    }

    // We are sure that version field is present in the header
    const char* version;
    status = dynCommon_getEntryForHead(&descriptor->header, "version", &version);
    assert(status == OK);
    assert(version != NULL);
    descriptor->version = celix_version_createVersionFromString(version);
    if (descriptor->version == NULL) {
        celix_err_pushf("Invalid version (%s) in parsed descriptor\n",version);
        return ERROR;
    }
    return OK;
}

void celix_dynDescriptor_destroy(celix_descriptor_t* descriptor) {
    dynCommon_clearNamValHead(&descriptor->header);
    dynCommon_clearNamValHead(&descriptor->annotations);

    struct type_entry *tInfo = TAILQ_FIRST(&descriptor->types);
    while (tInfo != NULL) {
        struct type_entry *tmp = tInfo;
        tInfo = TAILQ_NEXT(tInfo, entries);
        dynType_destroy(tmp->type);
        free(tmp);
    }

    if(descriptor->version!=NULL){
        celix_version_destroy(descriptor->version);
    }

    free(descriptor);
}
