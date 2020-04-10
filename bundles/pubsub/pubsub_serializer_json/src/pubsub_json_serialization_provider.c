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
#include <dirent.h>
#include <string.h>

#include "json_serializer.h"
#include "celix_version.h"
#include "celix_utils.h"
#include "dyn_message.h"
#include "pubsub_utils.h"
#include "log_helper.h"
#include "pubsub_message_serialization_service.h"
#include "../../../shell/shell/api/celix_shell_command.h"

#define MAX_PATH_LEN    1024
#define PUBSUB_JSON_SERIALIZATION_TYPE "json"

typedef enum
{
    FIT_INVALID = 0,
    FIT_DESCRIPTOR = 1,
    FIT_AVPR = 2
} descriptor_type_e;

#define L_DEBUG(...) \
    logHelper_log(provider->log, OSGI_LOGSERVICE_DEBUG, __VA_ARGS__)
#define L_INFO(...) \
    logHelper_log(provider->log, OSGI_LOGSERVICE_INFO, __VA_ARGS__)
#define L_WARN(...) \
    logHelper_log(provider->log, OSGI_LOGSERVICE_WARNING, __VA_ARGS__)
#define L_ERROR(...) \
    logHelper_log(provider->log, OSGI_LOGSERVICE_ERROR, __VA_ARGS__)

typedef struct {
    long svcId;
    pubsub_message_serialization_service_t  svc;
    char* descriptorContent;
    dyn_message_type *msgType;
    unsigned int msgId;
    const char* msgFqn;
    celix_version_t *msgVersion;
    char *msgVersionStr;

    long readFromBndId;
    char* readFromEntryPath;
    size_t nrOfTimesRead; //nr of times read from different bundles.

    bool valid;
    const char* invalidReason;
} pubsub_json_serialization_entry_t ;

struct pubsub_json_serialization_provider {
    celix_bundle_context_t *ctx;
    log_helper_t *log;

    //updated serialization services
    long bundleTrackerId;

    pubsub_message_serialization_marker_t markerSvc;
    long serializationMarkerSvcId;

    celix_shell_command_t cmdSvc;
    long cmdSvcId;

    celix_thread_mutex_t mutex; //protects below
    celix_array_list_t *serializationSvcEntries; //key = pubsub_json_serialization_entry;
};

static void dfi_log(void *handle, int level, const char *file, int line, const char *msg, ...) {
    va_list ap;
    pubsub_json_serialization_provider_t *provider = handle;
    char *logStr = NULL;
    va_start(ap, msg);
    vasprintf(&logStr, msg, ap);
    va_end(ap);
    logHelper_log(provider->log, level, "FILE:%s, LINE:%i, MSG:%s", file, line, logStr);
    free(logStr);
}

static descriptor_type_e getDescriptorType(const char* filename) {
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

static bool readPropertiesFile(pubsub_json_serialization_provider_t* provider, const char* properties_file_name, const char* root, char* avpr_fqn, char* path) {
    snprintf(path, MAX_PATH_LEN, "%s/%s", root, properties_file_name); // use path to create path to properties file
    FILE *properties = fopen(path, "r");
    if (!properties) {
        L_WARN("Could not find or open %s as a properties file in %s\n", properties_file_name, root);
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
        L_WARN("File %s does not contain a fully qualified name for the parser\n", properties_file_name);
        return false;
    }

    if (*path == '\0') {
        L_WARN("File %s does not contain a location for the avpr file\n", properties_file_name);
        return false;
    }

    return true;
}

static FILE* openFileStream(pubsub_json_serialization_provider_t* provider, descriptor_type_e descriptorType, const char* filename, const char* root, char* avpr_fqn, char* pathOrError) {
    FILE* result = NULL;
    memset(pathOrError, 0, MAX_PATH_LEN);
    switch (descriptorType) {
        case FIT_INVALID:
            snprintf(pathOrError, MAX_PATH_LEN, "Because %s is not a valid file", filename);
            break;
        case FIT_DESCRIPTOR:
            snprintf(pathOrError, MAX_PATH_LEN, "%s/%s", root, filename);
            result = fopen(pathOrError, "r");
            break;
        case FIT_AVPR:
            if (readPropertiesFile(provider, filename, root, avpr_fqn, pathOrError)) {
                result = fopen(pathOrError, "r");
            }
            break;
        default:
            L_WARN("[json serializer] Unknown file input type, returning NULL!\n");
            break;
    }

    return result;
}

static unsigned int pubsub_jsonSerializationProvider_getMsgId(pubsub_json_serialization_provider_t* provider __attribute__((unused)), dyn_message_type *msg) {
    unsigned int msgId = 0;

    char *msgName = NULL;
    dynMessage_getName(msg, &msgName);

    char *msgIdStr = NULL;
    int rv = dynMessage_getAnnotationEntry(msg, "msgId", &msgIdStr);
    if (rv == CELIX_SUCCESS && msgIdStr != NULL) {
        // custom msg id passed, use it
        long customMsgId = strtol(msgIdStr, NULL, 10);
        if (customMsgId > 0) {
            msgId = (unsigned int) customMsgId;
        }
    }
    if (msgId == 0) {
        msgId = celix_utils_stringHash(msgName);
    }

    return msgId;
}

static dyn_message_type* pubsub_jsonSerializationProvider_parseDfiDescriptor(pubsub_json_serialization_provider_t* provider, FILE* stream, const char* entryPath) {
    dyn_message_type *msg = NULL;
    int rc = dynMessage_parse(stream, &msg);
    if (rc != 0 || msg == NULL) {
        L_WARN("Cannot parse message from descriptor from entry %s.\n", entryPath);
        return NULL;
    }

    char *msgName = NULL;
    rc += dynMessage_getName(msg, &msgName);

    version_pt msgVersion = NULL;
    rc += dynMessage_getVersion(msg, &msgVersion);

    if (rc != 0 || msgName == NULL || msgVersion == NULL) {
        L_WARN("Cannot retrieve name and/or version from msg, using entry %s.\n", entryPath);
        dynMessage_destroy(msg);
        return NULL;
    }

    return msg;
}

//TODO FIXME, see #158
//
//    static dyn_message_type* pubsub_jsonSerializationProvider_parseAvprDescriptor(pubsub_json_serialization_provider_t* provider, FILE* stream, const char *entryName, const char* fqn) {
//
//    //dyn_message_type* msgType = dynMessage_parseAvpr(file_ptr, fqn);
//    dyn_message_type* msgType = NULL;
//
//    if (!msgType) {
//        L_WARN("[json serializer] Cannot parse avpr file '%s'\n", fqn);
//        return -1;
//    }
//
//    dyn_type* type;
//    dynMessage_getMessageType(msgType, &type);
//
//    const char *msgName = dynType_getName(type);
//
//    version_pt msgVersion = NULL;
//    celix_status_t s = version_createVersionFromString(dynType_getMetaInfo(type, "version"), &msgVersion);
//
//    if (s != CELIX_SUCCESS || !msgName) {
//        L_WARN("[json serializer] Cannot retrieve name and/or version from msg\n");
//        if (s == CELIX_SUCCESS) {
//            version_destroy(msgVersion);
//        }
//        return -1;
//    }
//
//    unsigned int msgId = 0;
//    const char *msgIdStr = dynType_getMetaInfo(type, "msgId");
//    if (msgIdStr != NULL) {
//        // custom msg id passed, use it
//        long customMsgId = strtol(msgIdStr, NULL, 10);
//        if (customMsgId > 0)
//            msgId = (unsigned int) customMsgId;
//    }
//
//    if (msgId == 0) {
//        msgId = utils_stringHash(msgName);
//    }
//
//
//
//    return 0;
//}
//}

/**
 * Returns true if the msgType is valid and uqinue (new msg fqn & msg id).
 * Logs error if msg id clashes or versions are different.
 * Logs warning if descriptors are different.
 */
static bool pubsub_jsonSerializationProvider_isUniqueAndCheckValid(pubsub_json_serialization_provider_t* provider, pubsub_json_serialization_entry_t* entry) {
    bool unique = true;
    
    celixThreadMutex_lock(&provider->mutex);
    for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
        pubsub_json_serialization_entry_t* visit = celix_arrayList_get(provider->serializationSvcEntries, i);
        if (visit->msgId == entry->msgId || strncmp(visit->msgFqn, entry->msgFqn, 1024*1024) == 0) {
            unique = false; //already have a descriptor with the same id or fqn. check if valid
            visit->nrOfTimesRead += 1;
            if (visit->msgId == entry->msgId && strncmp(visit->msgFqn, entry->msgFqn, 1024*1024) != 0) {
                L_ERROR("Error descriptor adding %s. Found msg types with same msg id, but different msg fqn. Msg id is %d, but found fully qualified names are '%s' and '%s'",
                        entry->readFromEntryPath, entry->msgId, entry->msgFqn, visit->msgFqn);
                entry->invalidReason = "msg id clash";
                entry->valid = false;
            } else if (strncmp(visit->msgFqn, entry->msgFqn, 1024*1024) == 0 && entry->msgId != visit->msgId) {
                L_ERROR("Error descriptor adding %s. Found msg types with same fqn, but different msg ids. Msg fqn is %d, but found msg ids are '%d' and '%d'",
                        entry->readFromEntryPath, entry->msgFqn, entry->msgId, visit->msgId);
                entry->invalidReason = "msg fqn clash";
                entry->valid = false;
            } else if (celix_version_compareTo(visit->msgVersion, entry->msgVersion) != 0) {
                L_ERROR("Error descriptor adding %s. Found two different version for msg %s. This is not supported, please align used versions between bundles!. Versions found %s and %s",
                        entry->readFromEntryPath, entry->msgFqn, entry->msgVersionStr, visit->msgVersionStr);
                entry->invalidReason = "different versions for the same msg type";
                entry->valid = false;
            } else if (strncmp(visit->descriptorContent, entry->descriptorContent, 1024*1014*10) != 0) {
                L_ERROR("Error adding descriptor %s. Found different descriptor content between '%s' and %s", entry->readFromEntryPath, entry->descriptorContent, visit->descriptorContent);
                entry->invalidReason = "different versions for the same msg type";
                entry->valid = false;
            }
        }
    }
    celixThreadMutex_unlock(&provider->mutex);

    return unique;
}

static void pubsub_jsonSerializationProvider_registerSerializationEntry(pubsub_json_serialization_provider_t* provider, pubsub_json_serialization_entry_t* entry) {
    if (entry->svcId == -1L) {
        celix_properties_t* props = celix_properties_create();
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY, entry->msgFqn);
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY, entry->msgVersionStr);
        celix_properties_setLong(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY, (long)entry->msgId);
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_SERIALIZATION_TYPE_PROPERTY, PUBSUB_JSON_SERIALIZATION_TYPE);

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.svc = &entry->svc;
        opts.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_VERSION;
        opts.properties = props;
        entry->svcId = celix_bundleContext_registerServiceWithOptions(provider->ctx, &opts);
    }
}

static celix_status_t pubsub_jsonSerializationProvider_serialize(void *handle, const void* msg, struct iovec** output, size_t* outputIovLen) {
    celix_status_t status = CELIX_SUCCESS;
    pubsub_json_serialization_entry_t *entry = handle;

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
        (**output).iov_len  = strlen(jsonOutput) + 1;
    }

    return status;
}

void pubsub_jsonSerializationProvider_freeSerializeMsg(void* handle, struct iovec* input, size_t inputIovLen) {
    pubsub_json_serialization_entry_t *entry = handle;
    if (input == NULL) return;
    if (entry->msgType != NULL) {
        for (int i = 0; i < inputIovLen; i++) {
            if (input[i].iov_base) {
                free(input[i].iov_base);
            }
            input[i].iov_base = NULL;
            input[i].iov_len  = 0;
        }
    }
}

celix_status_t pubsub_jsonSerializationProvider_deserialize(void* handle, const struct iovec* input, size_t inputIovLen __attribute__((unused)), void **out) {
    celix_status_t status = CELIX_SUCCESS;
    if (input == NULL) return CELIX_BUNDLE_EXCEPTION;
    pubsub_json_serialization_entry_t *entry = handle;
    void *msg = NULL;
    dyn_type* dynType;
    dynMessage_getMessageType(entry->msgType, &dynType);

    if (jsonSerializer_deserialize(dynType, (const char*)input->iov_base, &msg) != 0) {
        status = CELIX_BUNDLE_EXCEPTION;
    } else{
        *out = msg;
    }

    return status;
}

void pubsub_jsonSerializationProvider_freeDeserializeMsg(void* handle, void *msg) {
    pubsub_json_serialization_entry_t *entry = handle;
    if (entry->msgType != NULL) {
        dyn_type* dynType;
        dynMessage_getMessageType(entry->msgType, &dynType);
        dynType_free(dynType, msg);
    }
}

static void pubsub_jsonSerializationProvider_parseDescriptors(pubsub_json_serialization_provider_t* provider, const char *root, long bndId) {
    char fqn[MAX_PATH_LEN];
    char pathOrError[MAX_PATH_LEN];
    const char* entry_name = NULL;
    descriptor_type_e descriptorType;

    const struct dirent *entry = NULL;
    DIR* dir = opendir(root);
    if (dir) {
        entry = readdir(dir);
    }

    for (; entry != NULL; entry = readdir(dir)) {
        FILE* stream = NULL;
        entry_name = entry->d_name;
        descriptorType = getDescriptorType(entry_name);
        if (descriptorType != FIT_INVALID) {
            L_DEBUG("Parsing entry '%s'\n", entry_name);
            stream = openFileStream(provider, descriptorType, entry_name, root, /*out*/fqn, /*out*/pathOrError);
            if (!stream) {
                L_WARN("Cannot open descriptor file: '%s'\n", pathOrError);
            }
        }

        if (!stream) {
            continue; // Go to next entry in directory
        }

        char *entryPath = NULL;
        asprintf(&entryPath, "%s/%s", root, entry_name);

        dyn_message_type *msgType = NULL;
        if (descriptorType == FIT_DESCRIPTOR) {
            msgType = pubsub_jsonSerializationProvider_parseDfiDescriptor(provider, stream, entryPath);
        } else if (descriptorType == FIT_AVPR) {
            L_DEBUG("Ignoring avpr files for now, needs fixing!");
            //msgType = pubsub_jsonSerializationProvider_parseAvprDescriptor(provider, stream, entry_name, /*TODO FQN*/fqn);
        } else {
            L_ERROR("Unexpected descriptor type for entry %s.", entryPath);
        }

        if (msgType == NULL) {
            free(entryPath);
            continue;
        }

        fseek(stream, 0L, SEEK_END);
        long streamSize = ftell(stream);
        char *membuf = malloc(streamSize + 1);
        rewind(stream);
        fread(membuf, streamSize, 1, stream);
        fclose(stream);
        membuf[streamSize] = '\0';


        celix_version_t *msgVersion = NULL;
        char *msgFqn = NULL;
        dynMessage_getVersion(msgType, &msgVersion);
        dynMessage_getName(msgType, &msgFqn);
        unsigned int msgId = pubsub_jsonSerializationProvider_getMsgId(provider, msgType);

        pubsub_json_serialization_entry_t* serEntry = calloc(1, sizeof(*serEntry));
        serEntry->descriptorContent = membuf;
        serEntry->msgFqn = msgFqn;
        serEntry->msgVersion = msgVersion;
        serEntry->msgVersionStr = celix_version_toString(msgVersion);
        serEntry->msgId = msgId;
        serEntry->msgType = msgType;
        serEntry->readFromBndId = bndId;
        serEntry->readFromEntryPath = entryPath;
        serEntry->nrOfTimesRead = 1;
        serEntry->valid = true;
        serEntry->invalidReason = "";
        serEntry->svc.handle = serEntry;
        serEntry->svc.serialize = pubsub_jsonSerializationProvider_serialize;
        serEntry->svc.freeSerializedMsg = pubsub_jsonSerializationProvider_freeSerializeMsg;
        serEntry->svc.deserialize = pubsub_jsonSerializationProvider_deserialize;
        serEntry->svc.freeDeserializedMsg = pubsub_jsonSerializationProvider_freeDeserializeMsg;
        serEntry->svcId = -1L;


        bool unique = pubsub_jsonSerializationProvider_isUniqueAndCheckValid(provider, serEntry);
        if (unique && serEntry->valid) { //note only register if unique and valid
            L_DEBUG("Adding message serialization entry for msg %s with id %d and version %s", serEntry->msgFqn, serEntry->msgId, serEntry->msgVersion);
            pubsub_jsonSerializationProvider_registerSerializationEntry(provider, serEntry);
        }

        celixThreadMutex_lock(&provider->mutex);
        if (unique || !serEntry->valid) { //add all unique entries and ! invalid entries. The invalid entries are added to support debugging.
            celix_arrayList_add(provider->serializationSvcEntries, serEntry);
        } else {
            free(serEntry->descriptorContent);
            free(serEntry->readFromEntryPath);
            free(serEntry->msgVersionStr);
            dynMessage_destroy(serEntry->msgType);
            free(serEntry);
        }
        celixThreadMutex_unlock(&provider->mutex);
    }

    if (dir) {
        closedir(dir);
    }
}

static void pubsub_jsonSerializationProvider_printEntryDetails(pubsub_json_serialization_provider_t* provider, FILE *outStream, pubsub_json_serialization_entry_t *entry) {
    char *bndName = celix_bundleContext_getBundleSymbolicName(provider->ctx, entry->readFromBndId);
    fprintf(outStream, "|- msg fqn           : %s\n", entry->msgFqn);
    fprintf(outStream, "|- msg id            : %d (0x8%x)\n", entry->msgId, entry->msgId);
    fprintf(outStream, "|- msg version       : %s\n", entry->msgVersionStr);
    fprintf(outStream, "|- svc id            : %li\n", entry->svcId);
    fprintf(outStream, "|- read from bundle  : %s (bundle id %li)\n", bndName, entry->readFromBndId);
    fprintf(outStream, "|- bundle entry name : %s\n", entry->readFromEntryPath);
    fprintf(outStream, "|- nr of times found : %lu\n", entry->nrOfTimesRead);
    fprintf(outStream, "|- valid             : %s\n", entry->valid ? "true" : "false");
    if (!entry->valid) {
        fprintf(outStream, "|- invalid reason    : %s\n", entry->invalidReason);
    }
    fprintf(outStream, "|- descriptor        :\n");
    fprintf(outStream, "%s\n", entry->descriptorContent);
    free(bndName);
}

bool pubsub_jsonSerializationProvider_executeCommand(void *handle, const char *commandLine , FILE *outStream, FILE *errorStream  __attribute__((unused))) {
    pubsub_json_serialization_provider_t* provider = handle;

    bool verbose = false;
    unsigned int msgId = 0;
    const char *msgFqn = NULL;

    //parse command line
    char *line = celix_utils_strdup(commandLine);
    char *lasts = NULL;
    // skip first argument since this is the command
    strtok_r(line," ", &lasts);
    char* tok = strtok_r(NULL, " ", &lasts);

    if (tok != NULL && strncmp("-v", tok, 32) == 0) {
        verbose = true;
    } else if (tok != NULL) {
        errno = 0;
        msgId = strtol(tok, NULL, 10);
        if (errno == EINVAL) {
            msgId = 0;
            msgFqn = tok;
        }
    }

    if (msgId != 0 || msgFqn != NULL) {
        celixThreadMutex_lock(&provider->mutex);
        bool match = false;
        for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
            pubsub_json_serialization_entry_t *entry = celix_arrayList_get(provider->serializationSvcEntries, i);
            if (msgId != 0 && msgId == entry->msgId) {
                match = true;
            } else if (msgFqn != NULL && strncmp(msgFqn, entry->msgFqn, 1024*1024) == 0) {
                match = true;
            }
            if (match) {
                fprintf(outStream, "JSON message serialization service info:\n");
                pubsub_jsonSerializationProvider_printEntryDetails(provider, outStream, entry);
                break;
            }
        }
        celixThreadMutex_unlock(&provider->mutex);
    } else {
        celixThreadMutex_lock(&provider->mutex);
        if (celix_arrayList_size(provider->serializationSvcEntries) == 0) {
            fprintf(outStream, "No JSON message serialization services available.\n");
        } else {
            fprintf(outStream, "JSON message serialization services: \n");
            for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
                pubsub_json_serialization_entry_t *entry = celix_arrayList_get(provider->serializationSvcEntries, i);
                if (verbose) {
                    fprintf(outStream, "|- entry nr          : %d\n", i + 1);
                    pubsub_jsonSerializationProvider_printEntryDetails(provider, outStream, entry);
                    fprintf(outStream, "\n");
                } else {
                    fprintf(outStream, "|- entry nr %02d: msg id=%d, msg fqn=%s, msg version = %s, valid = %s\n", i + 1, entry->msgId, entry->msgFqn, entry->msgVersionStr, entry->valid ? "true" : "false");
                }
            }
        }
        celixThreadMutex_unlock(&provider->mutex);
    }

    free(line);
    return true;
}

void pubsub_jsonSerializationProvider_onInstalledBundle(void *handle, const celix_bundle_t *bnd) {
    pubsub_json_serialization_provider_t* provider = handle;
    char *descriptorsDir = pubsub_getMessageDescriptorsDir(provider->ctx, bnd);
    if (descriptorsDir != NULL) {
        pubsub_jsonSerializationProvider_parseDescriptors(provider, descriptorsDir, celix_bundle_getId(bnd));
    }
}

pubsub_json_serialization_provider_t* pubsub_jsonSerializationProvider_create(celix_bundle_context_t* ctx)  {
    pubsub_json_serialization_provider_t* provider = calloc(1, sizeof(*provider));
    provider->ctx = ctx;
    provider->log = logHelper_createWithName(ctx, "pubsub_JsonSerializationProvider");
    celixThreadMutex_create(&provider->mutex, NULL);
    provider->serializationSvcEntries = celix_arrayList_create();

    jsonSerializer_logSetup(dfi_log, provider, 1);
    dynFunction_logSetup(dfi_log, provider, 1);
    dynType_logSetup(dfi_log, provider, 1);
    dynCommon_logSetup(dfi_log, provider, 1);

    {
        celix_bundle_tracking_options_t opts = CELIX_EMPTY_BUNDLE_TRACKING_OPTIONS;
        opts.callbackHandle  = provider;
        opts.onInstalled = pubsub_jsonSerializationProvider_onInstalledBundle;
        opts.includeFrameworkBundle = true;
        provider->bundleTrackerId = celix_bundleContext_trackBundlesWithOptions(ctx, &opts);
    }

    {
        celix_properties_t* props = celix_properties_create();
        provider->markerSvc.handle = provider;
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_MARKER_SERIALIZATION_TYPE_PROPERTY, PUBSUB_JSON_SERIALIZATION_TYPE);
        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.svc = &provider->markerSvc;
        opts.serviceName = PUBSUB_MESSAGE_SERIALIZATION_MARKER_NAME;
        opts.serviceVersion = PUBSUB_MESSAGE_SERIALIZATION_MARKER_VERSION;
        opts.properties = props;
        provider->serializationMarkerSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);
    }

    {
        provider->cmdSvc.handle = provider;
        provider->cmdSvc.executeCommand = pubsub_jsonSerializationProvider_executeCommand;

        celix_properties_t* props = celix_properties_create();
        provider->cmdSvc.handle = provider;
        celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, "celix::json_message_serialization");
        celix_properties_set(props, CELIX_SHELL_COMMAND_USAGE, "celix::json_message_serialization [-v | <msg id> | <msg fqn>]");
        celix_properties_set(props, CELIX_SHELL_COMMAND_DESCRIPTION, "list available json message serialization services or provide detailed information about a serialization service.");
        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.svc = &provider->cmdSvc;
        opts.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
        opts.serviceVersion = CELIX_SHELL_COMMAND_SERVICE_VERSION;
        opts.properties = props;
        provider->cmdSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);
    }
    return provider;
}

void pubsub_jsonSerializationProvider_destroy(pubsub_json_serialization_provider_t* provider) {
    if (provider != NULL) {
        celix_bundleContext_stopTracker(provider->ctx, provider->bundleTrackerId);
        celix_bundleContext_unregisterService(provider->ctx, provider->serializationMarkerSvcId);
        celix_bundleContext_unregisterService(provider->ctx, provider->cmdSvcId);

        celixThreadMutex_lock(&provider->mutex);
        for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
            pubsub_json_serialization_entry_t *entry = celix_arrayList_get(provider->serializationSvcEntries, i);
            celix_bundleContext_unregisterService(provider->ctx, entry->svcId);
            free(entry->descriptorContent);
            free(entry->readFromEntryPath);
            free(entry->msgVersionStr);
            dynMessage_destroy(entry->msgType);
            free(entry);
        }
        celixThreadMutex_unlock(&provider->mutex);

        celixThreadMutex_destroy(&provider->mutex);

        free(provider);
    }
}

size_t pubsub_jsonSerializationProvider_nrOfEntries(pubsub_json_serialization_provider_t* provider) {
    size_t count = 0;
    celixThreadMutex_lock(&provider->mutex);
    for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
        pubsub_json_serialization_entry_t *entry = celix_arrayList_get(provider->serializationSvcEntries, i);
        if (entry->valid) {
            ++count;
        }
    }
    celixThreadMutex_unlock(&provider->mutex);
    return count;
}

size_t pubsub_jsonSerializationProvider_nrOfInvalidEntries(pubsub_json_serialization_provider_t* provider) {
    size_t count = 0;
    celixThreadMutex_lock(&provider->mutex);
    for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
        pubsub_json_serialization_entry_t *entry = celix_arrayList_get(provider->serializationSvcEntries, i);
        if (!entry->valid) {
            ++count;
        }
    }
    celixThreadMutex_unlock(&provider->mutex);
    return count;
}