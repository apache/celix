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

#ifndef CELIX_PUBSUB_SERIALIZATION_PROVIDER_H
#define CELIX_PUBSUB_SERIALIZATION_PROVIDER_H

#include "celix_log_helper.h"
#include "celix_bundle_context.h"
#include "pubsub_message_serialization_service.h"
#include "dyn_message.h"
#include "celix_version.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct pubsub_serialization_provider pubsub_serialization_provider_t; //opaque

typedef struct {
    celix_log_helper_t *log;
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

    //custom user data, will initialized to NULL. If freeUserData is set during destruction of the entry, this will be called.
    void* userData;
    void (*freeUserData)(void* userData);
} pubsub_serialization_entry_t;

/**
 * @brief Creates A (descriptor based) Serialization Provider.
 *
 * The provider monitors bundles and creates pubsub message serialization services for every unique descriptor found.
 *
 * Descriptors can be DFI descriptors or AVPR descriptors (FIXME #158).
 *
 * The provider will look for descriptors in META-INF/descriptors directory of every installed bundle.
 * If a framework config CELIX_FRAMEWORK_EXTENDER_PATH is set, this path will also be used to search for descriptor files.
 *
 * For every unique and valid descriptor found a pubsub_message_serialization_service will be registered for the serialization type (e.g. 'json')
 * The provider will also register a single pubsub_serialization_marker service which 'marks' the existing of the a serialization type serialization support.
 *
 * Lastly a celix command shell will be register with the command name 'celix::<serialization_type>_message_serialization' which can be used to interactively
 * query the provided serialization services.
 *
 * If no specific serialization is configured or builtin by the PubSubAdmin, serialization is chosen by the
 * highest ranking serialization marker service.
 *
 *
 * @param ctx                           The bundle context
 * @param serializationType             The serialization type (e.g. 'json')
 * @param backwardsCompatible           Whether the serializer can deserialize data if the minor version is higher. (note true for JSON)
 *                                      Will be used to set the 'serialization.backwards.compatible' service property for the pusbub_message_serialization_marker
 * @param serializationServiceRanking   The service raking used for the serialization marker service.
 * @param serialize                     The serialize function to use
 * @param freeSerializeMsg              The freeSerializeMsg function to use
 * @param deserialize                   The deserialize function to use
 * @param freeDeserializeMsg            The freeDesrializeMsg function to use
 * @return                              A pubsub serialization provided for the requested serialization type using the
 *                                      provided serialization functions.
 */
pubsub_serialization_provider_t *pubsub_serializationProvider_create(
        celix_bundle_context_t *ctx,
        const char* serializationType,
        bool backwardsCompatible,
        long serializationServiceRanking,
        celix_status_t (*serialize)(pubsub_serialization_entry_t* entry, const void* msg, struct iovec** output, size_t* outputIovLen),
        void (*freeSerializeMsg)(pubsub_serialization_entry_t* entry, struct iovec* input, size_t inputIovLen),
        celix_status_t (*deserialize)(pubsub_serialization_entry_t* entry, const struct iovec* input, size_t inputIovLen __attribute__((unused)), void **out),
        void (*freeDeserializeMsg)(pubsub_serialization_entry_t* entry, void *msg));

/**
 * Destroys the provided JSON Serialization Provider.
 */
void pubsub_serializationProvider_destroy(pubsub_serialization_provider_t *provider);


/**
 * @brief Returns the number of valid entries.
 */
size_t pubsub_serializationProvider_nrOfEntries(pubsub_serialization_provider_t *provider);

/**
 * @brief Returns the number of invalid entries.
 */
size_t pubsub_serializationProvider_nrOfInvalidEntries(pubsub_serialization_provider_t *provider);

/**
 * @brief Returns the log helper of the serialization provider.
 */
celix_log_helper_t* pubsub_serializationProvider_getLogHelper(pubsub_serialization_provider_t *provider);


#ifdef __cplusplus
};
#endif

#endif //CELIX_PUBSUB_SERIALIZATION_PROVIDER_H
