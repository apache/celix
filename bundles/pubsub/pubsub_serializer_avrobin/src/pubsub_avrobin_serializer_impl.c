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

#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <unistd.h>
#include <dirent.h>
#include <inttypes.h>

#include "utils.h"
#include "hash_map.h"
#include "bundle_context.h"

#include "celix_log_helper.h"

#include "avrobin_serializer.h"

#include "pubsub_avrobin_serializer_impl.h"

#define SYSTEM_BUNDLE_ARCHIVE_PATH "CELIX_FRAMEWORK_EXTENDER_PATH"
#define MAX_PATH_LEN 1024

typedef enum {
    FIT_INVALID = 0,
    FIT_DESCRIPTOR = 1,
    FIT_AVPR = 2
} FILE_INPUT_TYPE;

struct pubsub_avrobin_serializer {
    celix_bundle_context_t *bundle_context;
    celix_log_helper_t *loghelper;
};

static celix_status_t pubsubMsgAvrobinSerializer_serialize(void *handle, const void *msg, struct iovec** output, size_t* outputIovLen);
static celix_status_t pubsubMsgAvrobinSerializer_deserialize(void *handle, const struct iovec *input, size_t inputIovLen, void **out);
static void pubsubMsgAvrobinSerializer_freeDeserializeMsg(void *handle, void *msg);
static void pubsubMsgAvrobinSerializer_freeSerializeMsg(void* handle, struct iovec* input, size_t inputIovLen);
static FILE* openFileStream(FILE_INPUT_TYPE file_input_type, const char* filename, const char* root, /*output*/ char* avpr_fqn, /*output*/ char* path);
static FILE_INPUT_TYPE getFileInputType(const char* filename);
static bool readPropertiesFile(const char* properties_file_name, const char* root, /*output*/ char* avpr_fqn, /*output*/ char* path);

typedef struct pubsub_avrobin_msg_serializer_impl {
    dyn_message_type *msgType;
    unsigned int msgId;
    const char *msgName;
    version_pt msgVersion;
} pubsub_avrobin_msg_serializer_impl_t;

static char *pubsubAvrobinSerializer_getMsgDescriptionDir(const celix_bundle_t *bundle);
static void pubsubAvrobinSerializer_addMsgSerializerFromBundle(const char *root, const celix_bundle_t *bundle, hash_map_pt msgTypesMap);
static void pubsubAvrobinSerializer_fillMsgSerializerMap(hash_map_pt msgTypesMap, const celix_bundle_t *bundle);

static int pubsubMsgAvrobinSerializer_convertDescriptor(FILE* file_ptr, pubsub_msg_serializer_t* serializer);
static int pubsubMsgAvrobinSerializer_convertAvpr(FILE* file_ptr, pubsub_msg_serializer_t* serializer, const char* fqn);

static void dfi_log(void *handle, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    pubsub_avrobin_serializer_t *serializer = handle;
    char *logStr = NULL;
    va_start(ap, msg);
    vasprintf(&logStr, msg, ap);
    va_end(ap);
    celix_logHelper_log(serializer->loghelper, level, "FILE:%s, LINE:%i, MSG:%s", file, line, logStr);
    free(logStr);
}

celix_status_t pubsubAvrobinSerializer_create(celix_bundle_context_t *context, pubsub_avrobin_serializer_t **serializer) {
    celix_status_t status = CELIX_SUCCESS;

    *serializer = calloc(1, sizeof(**serializer));

    if (*serializer == NULL) {
        status = CELIX_ENOMEM;
    } else {

        (*serializer)->bundle_context = context;

        (*serializer)->loghelper = celix_logHelper_create(context, "celix_psa_serializer_avrobin");

        avrobinSerializer_logSetup(dfi_log, (*serializer), 1);
        dynFunction_logSetup(dfi_log, (*serializer), 1);
        dynType_logSetup(dfi_log, (*serializer), 1);
        dynCommon_logSetup(dfi_log, (*serializer), 1);
    }

    return status;
}

celix_status_t pubsubAvrobinSerializer_destroy(pubsub_avrobin_serializer_t *serializer) {
    celix_status_t status = CELIX_SUCCESS;

    celix_logHelper_destroy(serializer->loghelper);

    free(serializer);

    return status;
}

celix_status_t pubsubAvrobinSerializer_createSerializerMap(void *handle, const celix_bundle_t *bundle, hash_map_pt *serializerMap) {
    celix_status_t status = CELIX_SUCCESS;
    pubsub_avrobin_serializer_t *serializer = handle;

    hash_map_pt map = hashMap_create(NULL, NULL, NULL, NULL);

    if (map != NULL) {
        pubsubAvrobinSerializer_fillMsgSerializerMap(map, bundle);
    } else {
        celix_logHelper_log(serializer->loghelper, CELIX_LOG_LEVEL_ERROR, "Cannot allocate memory for msg map");
        status = CELIX_ENOMEM;
    }

    if (status == CELIX_SUCCESS) {
        *serializerMap = map;
    }

    return status;
}

celix_status_t pubsubAvrobinSerializer_destroySerializerMap(void *handle, hash_map_pt serializerMap) {
    celix_status_t status = CELIX_SUCCESS;

    if (serializerMap == NULL) {
        return CELIX_ILLEGAL_ARGUMENT;
    }

    hash_map_iterator_t iter = hashMapIterator_construct(serializerMap);
    while (hashMapIterator_hasNext(&iter)) {
        pubsub_msg_serializer_t* msgSerializer = hashMapIterator_nextValue(&iter);
        pubsub_avrobin_msg_serializer_impl_t *impl = msgSerializer->handle;
        dynMessage_destroy(impl->msgType);
        free(msgSerializer); //also contains the service struct.
        free(impl);
    }

    hashMap_destroy(serializerMap, false, false);

    return status;
}

static celix_status_t pubsubMsgAvrobinSerializer_serialize(void *handle, const void *msg, struct iovec** output, size_t* outputIovLen) {
    celix_status_t status = CELIX_SUCCESS;

    pubsub_avrobin_msg_serializer_impl_t *impl = handle;

    uint8_t *avroData = NULL;
    size_t avroLen = -1;
    dyn_type *dynType = NULL;
    dynMessage_getMessageType(impl->msgType, &dynType);

    if (*output == NULL) {
        *output = calloc(1, sizeof(struct iovec));
        if (outputIovLen) *outputIovLen = 1;
        if (output == NULL) status = CELIX_BUNDLE_EXCEPTION;
    }
    if (avrobinSerializer_serialize(dynType, msg, &avroData, &avroLen) != 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    }

    /*if (status == CELIX_SUCCESS) {
        char *schema = NULL;
        if (avrobinSerializer_generateSchema(dynType, &schema)==0) {
            printf("%s\n", schema);
            free(schema);
        }
    }*/

    if (status == CELIX_SUCCESS) {
        (**output).iov_base = (void*)avroData;
        (**output).iov_len  = avroLen;
        if (outputIovLen) *outputIovLen = 1;
    }

    return status;
}

void pubsubMsgAvrobinSerializer_freeSerializeMsg(void* handle, struct iovec* input, size_t inputIovLen) {
  pubsub_avrobin_msg_serializer_impl_t *impl = handle;
  if (input == NULL) {
      return;
  }
  if (impl->msgType != NULL) {
    for (int i = 0; i < inputIovLen; i++) {
      if (input[i].iov_base) free(input[i].iov_base);
      input[i].iov_base = NULL;
      input[i].iov_len  = 0;
    }
    free(input);
  }
}


static celix_status_t pubsubMsgAvrobinSerializer_deserialize(void *handle, const struct iovec* input, size_t inputIovLen __attribute__((unused)), void **out) {
    celix_status_t status = CELIX_SUCCESS;
    if (input == NULL) return CELIX_BUNDLE_EXCEPTION;
    pubsub_avrobin_msg_serializer_impl_t *impl = handle;

    void *msg = NULL;
    dyn_type *dynType = NULL;
    dynMessage_getMessageType(impl->msgType, &dynType);

    if (avrobinSerializer_deserialize(dynType, (const uint8_t*)input->iov_base, input->iov_len, &msg) != 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else {
        *out = msg;
    }

    return status;
}

static void pubsubMsgAvrobinSerializer_freeDeserializeMsg(void *handle, void *msg) {
    pubsub_avrobin_msg_serializer_impl_t *impl = handle;
    if (impl->msgType != NULL) {
        dyn_type *dynType = NULL;
        dynMessage_getMessageType(impl->msgType, &dynType);
        dynType_free(dynType, msg);
    }
}

static char *pubsubAvrobinSerializer_getMsgDescriptionDir(const celix_bundle_t *bundle) {
    char *root = NULL;

    bool isSystemBundle = false;
    bundle_isSystemBundle(bundle, &isSystemBundle);

    if (isSystemBundle == true) {
        celix_bundle_context_t *context;
        bundle_getContext(bundle, &context);

        const char *prop = NULL;

        bundleContext_getProperty(context, SYSTEM_BUNDLE_ARCHIVE_PATH, &prop);

        if (prop != NULL) {
            root = strdup(prop);
        } else {
            root = getcwd(NULL, 0);
        }
    } else {
        bundle_getEntry(bundle, ".", &root);
    }

    return root;
}

static void pubsubAvrobinSerializer_addMsgSerializerFromBundle(const char *root, const celix_bundle_t *bundle, hash_map_pt msgTypesMap) {
    char fqn[MAX_PATH_LEN];
    char path[MAX_PATH_LEN];
    const char* entry_name = NULL;
    FILE_INPUT_TYPE fileInputType;
    FILE* stream = NULL;

    const struct dirent *entry = NULL;
    DIR* dir = opendir(root);
    if (dir) {
        entry = readdir(dir);
    }

    for (; entry != NULL; entry = readdir(dir)) {
        entry_name = entry->d_name;
        printf("DMU: Parsing entry '%s'\n", entry_name);
        fileInputType = getFileInputType(entry_name);
        stream = openFileStream(fileInputType, entry_name, root, /*out*/fqn, /*out*/path);
        if (!stream) {
            printf("DMU: Cannot open descriptor file: '%s'.\n", path);
            continue; // Go to next entry in directory
        }

        pubsub_avrobin_msg_serializer_impl_t *impl = calloc(1, sizeof(*impl));
        pubsub_msg_serializer_t *msgSerializer = calloc(1,sizeof(*msgSerializer));
        msgSerializer->handle = impl;

        int translation_result = -1;
        if (fileInputType == FIT_DESCRIPTOR) {
            translation_result = pubsubMsgAvrobinSerializer_convertDescriptor(stream, msgSerializer);
        }
        else if (fileInputType == FIT_AVPR) {
            translation_result = pubsubMsgAvrobinSerializer_convertAvpr(stream, msgSerializer, fqn);
        }
        fclose(stream);

        if (translation_result != 0) {
            printf("DMU: could not create serializer for '%s'\n", entry_name);
            free(impl);
            free(msgSerializer);
            continue;
        }

        // serializer has been constructed, try to put in the map
        if (hashMap_containsKey(msgTypesMap, (void *) (uintptr_t) msgSerializer->msgId)) {
            printf("Cannot add msg %s. clash in msg id %d!!\n", msgSerializer->msgName, msgSerializer->msgId);
            dynMessage_destroy(impl->msgType);
            free(msgSerializer);
            free(impl);
        } else if (msgSerializer->msgId == 0) {
            printf("Cannot add msg %s. clash in msg id %d!!\n", msgSerializer->msgName, msgSerializer->msgId);
            dynMessage_destroy(impl->msgType);
            free(msgSerializer);
            free(impl);
        }
        else {
            hashMap_put(msgTypesMap, (void *) (uintptr_t) msgSerializer->msgId, msgSerializer);
        }
    }

    if (dir) {
        closedir(dir);
    }
}

static void pubsubAvrobinSerializer_fillMsgSerializerMap(hash_map_pt msgTypesMap, const celix_bundle_t *bundle) {
    char *root = NULL;
    char *metaInfPath = NULL;

    root = pubsubAvrobinSerializer_getMsgDescriptionDir(bundle);

    if (root != NULL) {
        asprintf(&metaInfPath, "%s/META-INF/descriptors", root);

        pubsubAvrobinSerializer_addMsgSerializerFromBundle(root, bundle, msgTypesMap);
        pubsubAvrobinSerializer_addMsgSerializerFromBundle(metaInfPath, bundle, msgTypesMap);

        free(metaInfPath);
        free(root);
    }
}

static FILE* openFileStream(FILE_INPUT_TYPE file_input_type, const char* filename, const char* root, /*output*/ char* avpr_fqn, /*output*/ char* path) {
    FILE* result = NULL;
    memset(path, 0, MAX_PATH_LEN);
    switch (file_input_type) {
        case FIT_INVALID:
            snprintf(path, MAX_PATH_LEN, "Because %s is not a valid file", filename);
            break;

        case FIT_DESCRIPTOR:
            snprintf(path, MAX_PATH_LEN, "%s/%s", root, filename);
            result = fopen(path, "r");
            break;

        case FIT_AVPR:
            if (readPropertiesFile(filename, root, avpr_fqn, path)) {
                result = fopen(path, "r");
            }
            break;

        default:
            printf("DMU: Unknown file input type, returning NULL!\n");
            break;
    }

    return result;
}

static FILE_INPUT_TYPE getFileInputType(const char* filename) {
    if (strstr(filename, ".descriptor")) {
        return FIT_DESCRIPTOR;
    }
    else if (strstr(filename, ".properties")) {
        return FIT_AVPR;
    }
    else {
        return FIT_INVALID;
    }
}

static bool readPropertiesFile(const char* properties_file_name, const char* root, /*output*/ char* avpr_fqn, /*output*/ char* path) {
    snprintf(path, MAX_PATH_LEN, "%s/%s", root, properties_file_name); // use path to create path to properties file
    FILE *properties = fopen(path, "r");
    if (!properties) {
        printf("DMU: Could not find or open %s as a properties file in %s\n", properties_file_name, root);
        return false;
    }

    *avpr_fqn = '\0';
    *path = '\0'; //re-use path to create path to avpr file
    char *p_line = malloc(MAX_PATH_LEN);
    size_t line_len = MAX_PATH_LEN;
    while (getline(&p_line, &line_len, properties) >= 0) {
        if (strncmp(p_line, "fqn=", strlen("fqn=")) == 0) {
            snprintf(avpr_fqn, MAX_PATH_LEN, "%s", (p_line + strlen("fqn=")));
            avpr_fqn[strcspn(avpr_fqn, "\n")] = 0;
        }
        else if (strncmp(p_line, "avpr=", strlen("avpr=")) == 0) {
            snprintf(path, MAX_PATH_LEN, "%s/%s", root, (p_line + strlen("avpr=")));
            path[strcspn(path, "\n")] = 0;
        }
    }
    free(p_line);
    fclose(properties);

    if (*avpr_fqn == '\0') {
        printf("CMU: File %s does not contain a fully qualified name for the parser\n", properties_file_name);
        return false;
    }

    if (*path == '\0') {
        printf("CMU: File %s does not contain a location for the avpr file\n", properties_file_name);
        return false;
    }

    return true;
}

static int pubsubMsgAvrobinSerializer_convertDescriptor(FILE* file_ptr, pubsub_msg_serializer_t* serializer) {
    dyn_message_type* msgType = NULL;
    int rc = dynMessage_parse(file_ptr, &msgType);
    if (rc != 0 || msgType == NULL) {
        printf("DMU: cannot parse message from descriptor.\n");
        return -1;
    }

    char* msgName = NULL;
    rc += dynMessage_getName(msgType, &msgName);

    version_pt msgVersion = NULL;
    rc += dynMessage_getVersion(msgType, &msgVersion);

    if (rc != 0 || msgName == NULL || msgVersion == NULL) {
        printf("DMU: cannot retrieve name and/or version from msg\n");
        return -1;
    }

    dyn_type * type = NULL;
    dynMessage_getMessageType(msgType, &type);

    unsigned int msgId = 0;

    char *msgIdStr = NULL;
    int rv = dynMessage_getAnnotationEntry(msgType, "msgId", &msgIdStr);
    if (rv == CELIX_SUCCESS && msgIdStr != NULL) {
        // custom msg id passed, use it
        long customMsgId = strtol(msgIdStr, NULL, 10);
        if (customMsgId > 0)
            msgId = (unsigned int) customMsgId;
    }

    if (msgId == 0) {
        msgId = utils_stringHash(msgName);
    }

    pubsub_avrobin_msg_serializer_impl_t * handle = (pubsub_avrobin_msg_serializer_impl_t*)serializer->handle;
    handle->msgType = msgType;
    handle->msgId = msgId;
    handle->msgName = msgName;
    handle->msgVersion = msgVersion;

    serializer->msgId = handle->msgId;
    serializer->msgName = handle->msgName;
    serializer->msgVersion = handle->msgVersion;

    serializer->serialize = (void*) pubsubMsgAvrobinSerializer_serialize;
    serializer->deserialize = (void*) pubsubMsgAvrobinSerializer_deserialize;
    serializer->freeSerializeMsg = (void*) pubsubMsgAvrobinSerializer_freeSerializeMsg;
    serializer->freeDeserializeMsg = (void*) pubsubMsgAvrobinSerializer_freeDeserializeMsg;
    return 0;
}

static int pubsubMsgAvrobinSerializer_convertAvpr(FILE* file_ptr, pubsub_msg_serializer_t* serializer, const char* fqn) {
    if (!file_ptr || !fqn || !serializer) return -2;

    //TODO FIXME, see #158
    //dyn_message_type* msgType = dynMessage_parseAvpr(file_ptr, fqn);
    dyn_message_type* msgType = NULL;

    if (!msgType) {
        printf("DMU: cannot parse avpr file for '%s'\n", fqn);
        return -1;
    }

    dyn_type* type = NULL;
    dynMessage_getMessageType(msgType, &type);

    const char* msgName = dynType_getName(type);

    version_pt msgVersion = NULL;
    celix_status_t s = version_createVersionFromString(dynType_getMetaInfo(type, "version"), &msgVersion);

    if (s != CELIX_SUCCESS || !msgName) {
        printf("DMU: cannot retrieve name and/or version from msg\n");
        if (s == CELIX_SUCCESS) {
            version_destroy(msgVersion);
        }
        return -1;
    }

    unsigned int msgId = 0;

    const char *msgIdStr = dynType_getMetaInfo(type, "msgId");
    if (msgIdStr != NULL) {
        // custom msg id passed, use it
        long customMsgId = strtol(msgIdStr, NULL, 10);
        if (customMsgId > 0)
            msgId = (unsigned int) customMsgId;
    }

    if (msgId == 0) {
        msgId = utils_stringHash(msgName);
    }

    pubsub_avrobin_msg_serializer_impl_t * handle = (pubsub_avrobin_msg_serializer_impl_t*) serializer->handle;
    handle->msgType = msgType;
    handle->msgId = msgId;
    handle->msgName = msgName;
    handle->msgVersion = msgVersion;

    serializer->msgId = handle->msgId;
    serializer->msgName = handle->msgName;
    serializer->msgVersion = handle->msgVersion;

    serializer->serialize = (void*) pubsubMsgAvrobinSerializer_serialize;
    serializer->deserialize = (void*) pubsubMsgAvrobinSerializer_deserialize;
    serializer->freeSerializeMsg = (void*) pubsubMsgAvrobinSerializer_freeSerializeMsg;
    serializer->freeDeserializeMsg = (void*) pubsubMsgAvrobinSerializer_freeDeserializeMsg;

    return 0;
}
