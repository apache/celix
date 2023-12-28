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

#include "dyn_message.h"
#include "dyn_descriptor.h"
#include "celix_err.h"

#include <stdlib.h>
#include <string.h>

#include "dyn_common.h"
#include "dyn_type.h"

struct _dyn_message_type {
    CELIX_DESCRIPTOR_FIELDS
    dyn_type* msgType;
};

static const int OK = 0;
static const int ERROR = 1;

static int dynMessage_parseSection(celix_descriptor_t* desc, const char* secName, FILE* stream);
static int dynMessage_parseMessage(dyn_message_type* msg, FILE* stream);

int dynMessage_parse(FILE* descriptor, dyn_message_type** out) {
    int status = OK;

    celix_autoptr(dyn_message_type) msg = calloc(1, sizeof(*msg));
    if (msg == NULL) {
        celix_err_push("Error allocating memory for dynamic message");
        return ERROR;
    }
    TAILQ_INIT(&msg->header);
    TAILQ_INIT(&msg->annotations);
    TAILQ_INIT(&msg->types);

    if ((status = celix_dynDescriptor_parse((celix_descriptor_t*)msg, descriptor, dynMessage_parseSection)) != OK) {
        return status;
    }

    *out = celix_steal_ptr(msg);
    return status;
}

static int dynMessage_parseSection(celix_descriptor_t* desc, const char* secName, FILE* stream) {
    dyn_message_type* msg = (dyn_message_type*)desc;
    if (strcmp("message", secName) != 0) {
        celix_err_pushf("unsupported section '%s'", secName);
        return ERROR;
    }
    return dynMessage_parseMessage(msg, stream);
}

static int dynMessage_parseMessage(dyn_message_type* msg, FILE* stream) {
    int status;

    //expected input <dynType>\n
    const char* name = NULL;
    if ((status = dynCommon_getEntryForHead(&msg->header, "name", &name)) != OK) {
        return status;
    }

    if ((status = dynType_parse(stream, name, &(msg->types), &(msg->msgType))) != OK) {
        return status;
    }

    if ((status = dynCommon_eatChar(stream, '\n')) != OK) {
        return status;
    }
    return status;
}

void dynMessage_destroy(dyn_message_type* msg) {
    if (msg != NULL) {
        if (msg->msgType != NULL) {
            dynType_destroy(msg->msgType);
        }
        celix_dynDescriptor_destroy((celix_descriptor_t*)msg);
    }
}

const char* dynMessage_getName(const dyn_message_type* msg) {
    const char* name = NULL;
    // celix_dynDescriptor_checkInterface ensures that the name is present
    (void)dynCommon_getEntryForHead(&msg->header, "name", &name);
    return name;
}

const celix_version_t* dynMessage_getVersion(const dyn_message_type* msg){
    // celix_dynDescriptor_checkInterface ensures that version is present
    return msg->version;
}

const char* dynMessage_getVersionString(const dyn_message_type* msg) {
    const char* version = NULL;
    // celix_dynDescriptor_checkInterface ensures that the version is present
    (void)dynCommon_getEntryForHead(&msg->header, "version", &version);
    return version;
}

int dynMessage_getHeaderEntry(dyn_message_type* msg, const char* name, const char** value) {
    return dynCommon_getEntryForHead(&msg->header, name, value);
}

int dynMessage_getAnnotationEntry(dyn_message_type* msg, const char* name, const char** value) {
    return dynCommon_getEntryForHead(&msg->annotations, name, value);
}

const dyn_type* dynMessage_getMessageType(dyn_message_type* msg) {
    return msg->msgType;
}
