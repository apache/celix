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

#include "pubsub_serialization_provider.h"

#include <stdlib.h>
#include <stdarg.h>
#include <dirent.h>
#include <string.h>

#include "pubsub_message_serialization_marker.h"
#include "celix_constants.h"
#include "dyn_function.h"
#include "celix_version.h"
#include "celix_utils.h"
#include "dyn_message.h"
#include "dyn_interface.h"
#include "pubsub_utils.h"
#include "celix_log_helper.h"
#include "pubsub_message_serialization_service.h"
#include "celix_shell_command.h"

#define MAX_PATH_LEN    1024

typedef enum
{
    FIT_INVALID = 0,
    FIT_DESCRIPTOR = 1,
    FIT_AVPR = 2
} descriptor_type_e;

#define L_DEBUG(...) \
    celix_logHelper_debug(provider->logHelper, __VA_ARGS__)
#define L_INFO(...) \
    celix_logHelper_info(provider->logHelper, __VA_ARGS__)
#define L_WARN(...) \
    celix_logHelper_warning(provider->logHelper, __VA_ARGS__)
#define L_ERROR(...) \
    celix_logHelper_error(provider->logHelper, __VA_ARGS__)


struct pubsub_serialization_provider {
    celix_bundle_context_t *ctx;
    celix_log_helper_t *logHelper;
    char* serializationType;

    //serialization callbacks
    celix_status_t (*serialize)(pubsub_serialization_entry_t* entry, const void* msg, struct iovec** output, size_t* outputIovLen);
    void (*freeSerializeMsg)(pubsub_serialization_entry_t* entry, struct iovec* input, size_t inputIovLen);
    celix_status_t (*deserialize)(pubsub_serialization_entry_t* entry, const struct iovec* input, size_t inputIovLen __attribute__((unused)), void **out);
    void (*freeDeserializeMsg)(pubsub_serialization_entry_t* entry, void *msg);

    //updated serialization services
    long bundleTrackerId;

    celix_shell_command_t cmdSvc;
    long cmdSvcId;

    pubsub_message_serialization_marker_t markerSvc;
    long markerSvcId;

    celix_thread_mutex_t mutex; //protects below
    celix_array_list_t *serializationSvcEntries; //key = pubsub_serialization_entry;
};

static void dfi_log(void *handle, int level, const char *file, int line, const char *msg, ...) {
    (void)level;
    va_list ap;
    pubsub_serialization_provider_t *provider = handle;
    char *logStr = NULL;
    va_start(ap, msg);
    vasprintf(&logStr, msg, ap);
    va_end(ap);
    celix_logHelper_log(provider->logHelper, CELIX_LOG_LEVEL_WARNING, "FILE:%s, LINE:%i, MSG:%s", file, line, logStr);
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

/**
 * Reads an avpr properties file. A properties file which revert to an avpr file and the usaged definition in the avpr.
 */
static bool readAvprPropertiesFile(pubsub_serialization_provider_t* provider, const char* properties_file_name, const char* root, char* avpr_fqn, char* path) {
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

static FILE* openFileStream(pubsub_serialization_provider_t* provider, descriptor_type_e descriptorType, const char* filename, const char* root, char* avpr_fqn, char* pathOrError) {
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
            if (readAvprPropertiesFile(provider, filename, root, avpr_fqn, pathOrError)) {
                result = fopen(pathOrError, "r");
            }
            break;
        default:
            L_WARN("Unknown file input type, returning NULL!\n");
            break;
    }

    return result;
}

static unsigned int pubsub_serializationProvider_getMsgId(pubsub_serialization_provider_t* provider __attribute__((unused)), dyn_message_type *msg) {
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

static dyn_message_type* pubsub_serializationProvider_parseDfiDescriptor(pubsub_serialization_provider_t* provider, FILE* stream, const char* entryPath) {
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

static bool pubsub_serializationProvider_isDescriptorInterface(pubsub_serialization_provider_t* provider, FILE* stream) {
    dyn_interface_type *msg = NULL;
    bool isInterface = false;

    int rc = dynInterface_parse(stream, &msg);
    if (rc == 0 && msg != NULL) {
        isInterface = true;
    }
    dynInterface_destroy(msg);
    fseek(stream, 0, SEEK_SET);

    return isInterface;
}

//TODO FIXME, see #158
//
//    static dyn_message_type* pubsub_serializationProvider_parseAvprDescriptor(pubsub_serialization_provider_t* provider, FILE* stream, const char *entryName, const char* fqn) {
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
 * Check if a pubsub serialization entry is already present (exact path)
 *
 * @return true if the entry is a already present
 */
static bool pubsub_serializationProvider_alreadyAddedEntry(pubsub_serialization_provider_t* provider, pubsub_serialization_entry_t* entry) {
    for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
        pubsub_serialization_entry_t *visit = celix_arrayList_get(provider->serializationSvcEntries, i);
        if (celix_utils_stringEquals(visit->readFromEntryPath, entry->readFromEntryPath)) {
            return true;
        }
    }
    return false;
}

/**
 * Validates an pubsub serialization entry and check if this is a new unique entry.
 *
 * Checks whether the entry is valid. Specifically checks:
 *  - If there is not already an entry with the same msg fqn, but a different msg id.
 *  - If there is not already an entry with the same msg id, but a different msg fqn.
 *  - If there is not already an entry for the message with a different versions. (this is not supported).
 *  - If there is not already an entry for the message with a different descriptor content.
 *
 * Logs error if msg id clashes or versions are different.
 * Logs warning if descriptors are different.
 *
 * @return true if the entry is a new (unique) entry.
 */
static bool pubsub_serializationProvider_validateEntry(pubsub_serialization_provider_t* provider, pubsub_serialization_entry_t* entry) {
    bool unique = true;
    
    celixThreadMutex_lock(&provider->mutex);
    for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
        pubsub_serialization_entry_t* visit = celix_arrayList_get(provider->serializationSvcEntries, i);
        if (visit->msgId == entry->msgId || strncmp(visit->msgFqn, entry->msgFqn, 1024*1024) == 0) {
            unique = false; //already have a descriptor with the same id or fqn. check if valid
            visit->nrOfTimesRead += 1;
            if (visit->msgId == entry->msgId && strncmp(visit->msgFqn, entry->msgFqn, 1024*1024) != 0) {
                L_ERROR("Error adding descriptor %s. Found msg types with same msg id, but different msg fqn. Msg id is %d, but found fully qualified names are '%s' and '%s'. Not adding descriptor with msg fqn %s.",
                        entry->readFromEntryPath, entry->msgId, entry->msgFqn, visit->msgFqn, entry->msgFqn);
                entry->invalidReason = "msg id clash";
                entry->valid = false;
            } else if (strncmp(visit->msgFqn, entry->msgFqn, 1024*1024) == 0 && entry->msgId != visit->msgId) {
                L_ERROR("Error adding descriptor %s. Found msg types with same fqn, but different msg ids. Msg fqn is %s, but found msg ids are '%d' and '%d'. Not adding descriptor with msg id %d.",
                        entry->readFromEntryPath, entry->msgFqn, entry->msgId, visit->msgId, entry->msgId);
                entry->invalidReason = "msg fqn clash";
                entry->valid = false;
            } else if (celix_version_compareTo(visit->msgVersion, entry->msgVersion) != 0) {
                L_ERROR("Error adding descriptor %s. Found two different version for msg %s. This is not supported, please align used versions between bundles!. Versions found %s and %s. Not adding descriptor with version %s.",
                        entry->readFromEntryPath, entry->msgFqn, entry->msgVersionStr, visit->msgVersionStr, entry->msgVersionStr);
                entry->invalidReason = "different versions for the same msg type";
                entry->valid = false;
            } else if (strncmp(visit->descriptorContent, entry->descriptorContent, 1024*1014*10) != 0) {
                L_ERROR("Error adding descriptor %s. Added descriptor content is different than existing content. Existing: '%s', added: %s.", entry->readFromEntryPath, entry->descriptorContent, visit->descriptorContent);
                entry->invalidReason = "different versions for the same msg type";
                entry->valid = false;
            }
        }
    }
    celixThreadMutex_unlock(&provider->mutex);

    return unique;
}

static void pubsub_serializationProvider_registerSerializationEntry(pubsub_serialization_provider_t* provider, pubsub_serialization_entry_t* entry) {
    if (entry->svcId == -1L) {
        celix_properties_t* props = celix_properties_create();
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_FQN_PROPERTY, entry->msgFqn);
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_VERSION_PROPERTY, entry->msgVersionStr);
        celix_properties_setLong(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_MSG_ID_PROPERTY, (long)entry->msgId);
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_SERVICE_SERIALIZATION_TYPE_PROPERTY, provider->serializationType);

        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.svc = &entry->svc;
        opts.serviceName = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_NAME;
        opts.serviceVersion = PUBSUB_MESSAGE_SERIALIZATION_SERVICE_VERSION;
        opts.properties = props;
        entry->svcId = celix_bundleContext_registerServiceWithOptions(provider->ctx, &opts);
    }
}

static void pubsub_serializationProvider_parseDescriptors(pubsub_serialization_provider_t* provider, const char *root, long bndId) {
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
            if(!pubsub_serializationProvider_isDescriptorInterface(provider, stream)) {
                msgType = pubsub_serializationProvider_parseDfiDescriptor(provider, stream, entry_name);
            } else {
                L_DEBUG("Ignoring interface file");
            }
        } else if (descriptorType == FIT_AVPR) {
            L_DEBUG("Ignoring avpr files for now, needs fixing!");
            //msgType = pubsub_serializationProvider_parseAvprDescriptor(provider, stream, entry_name, /*TODO FQN*/fqn);
        } else {
            L_ERROR("Unexpected descriptor type for entry %s.", entryPath);
        }

        if (msgType == NULL) {
            free(entryPath);
            fclose(stream);
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
        unsigned int msgId = pubsub_serializationProvider_getMsgId(provider, msgType);

        pubsub_serialization_entry_t* serEntry = calloc(1, sizeof(*serEntry));
        serEntry->log = provider->logHelper;
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
        serEntry->svc.serialize = (void*)provider->serialize;
        serEntry->svc.freeSerializedMsg = (void*)provider->freeSerializeMsg;
        serEntry->svc.deserialize = (void*)provider->deserialize;
        serEntry->svc.freeDeserializedMsg = (void*)provider->freeDeserializeMsg;
        serEntry->svcId = -1L;

        if (pubsub_serializationProvider_alreadyAddedEntry(provider, serEntry)) {
            L_WARN("Skipping entry %s. Exact entry already present!. Double event triggered?", serEntry->readFromEntryPath);
            free(serEntry->descriptorContent);
            free(serEntry->readFromEntryPath);
            free(serEntry->msgVersionStr);
            dynMessage_destroy(serEntry->msgType);
            free(serEntry);
            continue;
        }

        bool unique = pubsub_serializationProvider_validateEntry(provider, serEntry);
        if (unique && serEntry->valid) { //note only register if unique and valid
            L_DEBUG("Adding message serialization entry for msg %s with id %d and version %s", serEntry->msgFqn, serEntry->msgId, serEntry->msgVersion);
            pubsub_serializationProvider_registerSerializationEntry(provider, serEntry);
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

static void pubsub_serializationProvider_printEntryDetails(pubsub_serialization_provider_t* provider, FILE *outStream, pubsub_serialization_entry_t *entry) {
    char *bndName = celix_bundleContext_getBundleSymbolicName(provider->ctx, entry->readFromBndId);
    fprintf(outStream, "|- %20s = %s\n", "msg fqn", entry->msgFqn);
    fprintf(outStream, "|- %20s = %d (0x8%x)\n", "msg id", entry->msgId, entry->msgId);
    fprintf(outStream, "|- %20s = %s\n", "msg vesion", entry->msgVersionStr);
    fprintf(outStream, "|- %20s = %li\n", "svc id", entry->svcId);
    fprintf(outStream, "|- %20s = %s (bundle id %li)\n", "read from bundle", bndName, entry->readFromBndId);
    fprintf(outStream, "|- %20s = %s\n", "bundle entry name", entry->readFromEntryPath);
    fprintf(outStream, "|- %20s = %lu\n", "nr of times found", (long unsigned int) entry->nrOfTimesRead);
    fprintf(outStream, "|- %20s = %s\n", "valid", entry->valid ? "true" : "false");
    if (!entry->valid) {
        fprintf(outStream, "|- %20s = %s\n", "invalid reason", entry->invalidReason);
    }
    fprintf(outStream, "|- %20s:\n", "descriptor");
    fprintf(outStream, "%s\n", entry->descriptorContent);
    free(bndName);
}

bool pubsub_serializationProvider_executeCommand(void *handle, const char *commandLine , FILE *outStream, FILE *errorStream  __attribute__((unused))) {
    pubsub_serialization_provider_t* provider = handle;

    bool verbose = false;
    bool invalids = false;
    unsigned int msgId = 0;
    const char *msgFqn = NULL;

    //parse command line
    char *line = celix_utils_strdup(commandLine);
    char *lasts = NULL;
    // skip first argument since this is the command
    strtok_r(line," ", &lasts);
    char* tok = strtok_r(NULL, " ", &lasts);

    if (tok != NULL && strncmp("invalids", tok, 32) == 0) {
        invalids = true;
    } else if (tok != NULL && strncmp("verbose", tok, 32) == 0) {
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
            pubsub_serialization_entry_t *entry = celix_arrayList_get(provider->serializationSvcEntries, i);
            if (msgId != 0 && msgId == entry->msgId) {
                match = true;
            } else if (msgFqn != NULL && strncmp(msgFqn, entry->msgFqn, 1024*1024) == 0) {
                match = true;
            }
            if (match) {
                fprintf(outStream, "%s message serialization service info:\n", provider->serializationType);
                pubsub_serializationProvider_printEntryDetails(provider, outStream, entry);
                break;
            }
        }
        celixThreadMutex_unlock(&provider->mutex);
    } else {
        celixThreadMutex_lock(&provider->mutex);
        if (celix_arrayList_size(provider->serializationSvcEntries) == 0) {
            fprintf(outStream, "No %s message serialization services available.\n", provider->serializationType);
        } else if (invalids) {
            fprintf(outStream, "%s invalid message serialization services: \n", provider->serializationType);
            size_t count = 0;
            for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
                pubsub_serialization_entry_t *entry = celix_arrayList_get(provider->serializationSvcEntries, i);
                if (!entry->valid) {
                    fprintf(outStream, "|- entry nr          : %d\n", i + 1);
                    pubsub_serializationProvider_printEntryDetails(provider, outStream, entry);
                    fprintf(outStream, "\n");
                    count++;
                }
            }
            if (count == 0) {
                fprintf(outStream, "No invalid message serialization entries found!\n");
            }
        } else {
            fprintf(outStream, "%s message serialization services: \n", provider->serializationType);
            for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
                pubsub_serialization_entry_t *entry = celix_arrayList_get(provider->serializationSvcEntries, i);
                if (verbose) {
                    fprintf(outStream, "|- entry nr          : %d\n", i + 1);
                    pubsub_serializationProvider_printEntryDetails(provider, outStream, entry);
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

void pubsub_serializationProvider_onInstalledBundle(void *handle, const celix_bundle_t *bnd) {
    pubsub_serialization_provider_t* provider = handle;
    char *descriptorsDir = pubsub_getMessageDescriptorsDir(provider->ctx, bnd);
    if (descriptorsDir != NULL) {
        pubsub_serializationProvider_parseDescriptors(provider, descriptorsDir, celix_bundle_getId(bnd));
        free(descriptorsDir);
    }
}

pubsub_serialization_provider_t *pubsub_serializationProvider_create(
        celix_bundle_context_t *ctx,
        const char* serializationType,
        bool backwardsCompatible,
        long serializationServiceRanking,
        celix_status_t (*serialize)(pubsub_serialization_entry_t* entry, const void* msg, struct iovec** output, size_t* outputIovLen),
        void (*freeSerializeMsg)(pubsub_serialization_entry_t* entry, struct iovec* input, size_t inputIovLen),
        celix_status_t (*deserialize)(pubsub_serialization_entry_t* entry, const struct iovec* input, size_t inputIovLen __attribute__((unused)), void **out),
        void (*freeDeserializeMsg)(pubsub_serialization_entry_t* entry, void *msg)) {
    pubsub_serialization_provider_t* provider = calloc(1, sizeof(*provider));
    provider->ctx = ctx;
    celixThreadMutex_create(&provider->mutex, NULL);
    provider->serializationSvcEntries = celix_arrayList_create();

    provider->serializationType = celix_utils_strdup(serializationType);
    provider->serialize = serialize;
    provider->freeSerializeMsg = freeSerializeMsg;
    provider->deserialize = deserialize;
    provider->freeDeserializeMsg = freeDeserializeMsg;
    provider->logHelper = celix_logHelper_create(ctx, "celix_pubsub_serialization_provider");

    dynFunction_logSetup(dfi_log, provider, 1);
    dynType_logSetup(dfi_log, provider, 1);
    dynCommon_logSetup(dfi_log, provider, 1);

    {
        //Start bundle tracker and register pubsub_message_serialization services
        celix_bundle_tracking_options_t opts = CELIX_EMPTY_BUNDLE_TRACKING_OPTIONS;
        opts.callbackHandle  = provider;
        opts.onInstalled = pubsub_serializationProvider_onInstalledBundle;
        opts.includeFrameworkBundle = true;
        provider->bundleTrackerId = celix_bundleContext_trackBundlesWithOptions(ctx, &opts);
    }

    {
        //Register shell command to query serializers
        provider->cmdSvc.handle = provider;
        provider->cmdSvc.executeCommand = pubsub_serializationProvider_executeCommand;

        char *name = NULL;
        asprintf(&name,"celix::%s_message_serialization", provider->serializationType);
        char *usage = NULL;
        asprintf(&usage,"celix::%s_message_serialization [verbose | invalids | <msg id> | <msg fqn>]", provider->serializationType);

        celix_properties_t* props = celix_properties_create();
        provider->cmdSvc.handle = provider;
        celix_properties_set(props, CELIX_SHELL_COMMAND_NAME, name);
        celix_properties_set(props, CELIX_SHELL_COMMAND_USAGE, usage);
        celix_properties_set(props, CELIX_SHELL_COMMAND_DESCRIPTION, "list available json message serialization services or provide detailed information about a serialization service.");
        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.svc = &provider->cmdSvc;
        opts.serviceName = CELIX_SHELL_COMMAND_SERVICE_NAME;
        opts.serviceVersion = CELIX_SHELL_COMMAND_SERVICE_VERSION;
        opts.properties = props;
        provider->cmdSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);

        free(name);
        free(usage);
    }

    {
        //Register pubsub_message_serialization_marker service to indicate the availability of this message serialization type.
        celix_properties_t* props = celix_properties_create();
        provider->markerSvc.handle = provider;
        celix_properties_set(props, PUBSUB_MESSAGE_SERIALIZATION_MARKER_SERIALIZATION_TYPE_PROPERTY, provider->serializationType);
        celix_properties_setBool(props, PUBSUB_MESSAGE_SERIALIZATION_MARKER_SERIALIZATION_BACKWARDS_COMPATIBLE, backwardsCompatible);
        celix_service_registration_options_t opts = CELIX_EMPTY_SERVICE_REGISTRATION_OPTIONS;
        opts.svc = &provider->markerSvc;
        opts.serviceName = PUBSUB_MESSAGE_SERIALIZATION_MARKER_NAME;
        opts.serviceVersion = PUBSUB_MESSAGE_SERIALIZATION_MARKER_VERSION;
        opts.properties = props;
        provider->markerSvcId = celix_bundleContext_registerServiceWithOptions(ctx, &opts);
    }

    return provider;
}

void pubsub_serializationProvider_destroy(pubsub_serialization_provider_t* provider) {
    if (provider != NULL) {
        celix_bundleContext_unregisterService(provider->ctx, provider->markerSvcId);

        celix_bundleContext_stopTracker(provider->ctx, provider->bundleTrackerId);
        celix_bundleContext_unregisterService(provider->ctx, provider->cmdSvcId);

        celixThreadMutex_lock(&provider->mutex);
        for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
            pubsub_serialization_entry_t *entry = celix_arrayList_get(provider->serializationSvcEntries, i);
            celix_bundleContext_unregisterService(provider->ctx, entry->svcId);
            if (entry->freeUserData) {
                entry->freeUserData(entry->userData);
            }
            free(entry->descriptorContent);
            free(entry->readFromEntryPath);
            free(entry->msgVersionStr);
            dynMessage_destroy(entry->msgType);
            free(entry);
        }
        celix_arrayList_destroy(provider->serializationSvcEntries);
        celixThreadMutex_unlock(&provider->mutex);

        celixThreadMutex_destroy(&provider->mutex);

        celix_logHelper_destroy(provider->logHelper);

        free(provider->serializationType);
        free(provider);
    }
}

size_t pubsub_serializationProvider_nrOfEntries(pubsub_serialization_provider_t* provider) {
    size_t count = 0;
    celixThreadMutex_lock(&provider->mutex);
    for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
        pubsub_serialization_entry_t *entry = celix_arrayList_get(provider->serializationSvcEntries, i);
        if (entry->valid) {
            ++count;
        }
    }
    celixThreadMutex_unlock(&provider->mutex);
    return count;
}

size_t pubsub_serializationProvider_nrOfInvalidEntries(pubsub_serialization_provider_t* provider) {
    size_t count = 0;
    celixThreadMutex_lock(&provider->mutex);
    for (int i = 0; i < celix_arrayList_size(provider->serializationSvcEntries); ++i) {
        pubsub_serialization_entry_t *entry = celix_arrayList_get(provider->serializationSvcEntries, i);
        if (!entry->valid) {
            ++count;
        }
    }
    celixThreadMutex_unlock(&provider->mutex);
    return count;
}

celix_log_helper_t* pubsub_serializationProvider_getLogHelper(pubsub_serialization_provider_t *provider) {
    return provider->logHelper;
}