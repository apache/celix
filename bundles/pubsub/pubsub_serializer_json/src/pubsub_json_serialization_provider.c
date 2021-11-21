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

#include "pubsub_json_serialization_provider.h"

#include <stdlib.h>
#include <stdarg.h>
#include <string.h>

#include "json_serializer.h"
#include "dyn_message.h"
#include "celix_log_helper.h"
#include "pubsub_message_serialization_service.h"

static void dfi_log(void *handle, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    celix_log_helper_t *log = handle;
    char *logStr = NULL;
    va_start(ap, msg);
    vasprintf(&logStr, msg, ap);
    va_end(ap);
    celix_logHelper_log(log, level, "FILE:%s, LINE:%i, MSG:%s", file, line, logStr);
    free(logStr);
}


static celix_status_t pubsub_jsonSerializationProvider_serialize(pubsub_serialization_entry_t* entry, const void* msg, struct iovec** output, size_t* outputIovLen) {
    celix_status_t status = CELIX_SUCCESS;

    if (*output == NULL) {
        *output = calloc(1, sizeof(struct iovec));
        *outputIovLen = 1;
    } else {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    char *jsonOutput = NULL;
    dyn_type* dynType;
    dynMessage_getMessageType(entry->msgType, &dynType);

    if (jsonSerializer_serialize(dynType, msg, &jsonOutput) != 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    }

    if (status == CELIX_SUCCESS) {
        (**output).iov_base = (void*)jsonOutput;
        (**output).iov_len  = strlen(jsonOutput);
    }

    return status;
}

static void pubsub_jsonSerializationProvider_freeSerializeMsg(pubsub_serialization_entry_t* entry, struct iovec* input, size_t inputIovLen) {
    if (input != NULL) {
        if (entry->msgType != NULL) {
            for (int i = 0; i < inputIovLen; i++) {
                if (input[i].iov_base) {
                    free(input[i].iov_base);
                }
                input[i].iov_base = NULL;
                input[i].iov_len = 0;
            }
        }
        free(input);
    }
}

static celix_status_t pubsub_jsonSerializationProvider_deserialize(pubsub_serialization_entry_t* entry, const struct iovec* input, size_t inputIovLen __attribute__((unused)), void **out) {
    celix_status_t status = CELIX_SUCCESS;
    if (input == NULL) return CELIX_BUNDLE_EXCEPTION;
    void *msg = NULL;
    dyn_type* dynType;
    dynMessage_getMessageType(entry->msgType, &dynType);

    if (jsonSerializer_deserialize(dynType, (const char*)input->iov_base, input->iov_len, &msg) != 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else{
        *out = msg;
    }

    return status;
}

static void pubsub_jsonSerializationProvider_freeDeserializeMsg(pubsub_serialization_entry_t* entry, void *msg) {
    if (entry->msgType != NULL) {
        dyn_type* dynType;
        dynMessage_getMessageType(entry->msgType, &dynType);
        dynType_free(dynType, msg);
    }
}

pubsub_serialization_provider_t* pubsub_jsonSerializationProvider_create(celix_bundle_context_t* ctx)  {
    pubsub_serialization_provider_t* provider = pubsub_serializationProvider_create(ctx, "json", true, 0, pubsub_jsonSerializationProvider_serialize, pubsub_jsonSerializationProvider_freeSerializeMsg, pubsub_jsonSerializationProvider_deserialize, pubsub_jsonSerializationProvider_freeDeserializeMsg);
    jsonSerializer_logSetup(dfi_log, pubsub_serializationProvider_getLogHelper(provider), 1);;
    return provider;
}

void pubsub_jsonSerializationProvider_destroy(pubsub_serialization_provider_t* provider) {
    pubsub_serializationProvider_destroy(provider);
}