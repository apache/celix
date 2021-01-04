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

#ifndef PUBSUB_UTILS_H_
#define PUBSUB_UTILS_H_

#include "bundle_context.h"
#include "celix_array_list.h"
#include "celix_bundle_context.h"
#ifdef __cplusplus
extern "C" {
#endif
#define PUBSUB_UTILS_QOS_ATTRIBUTE_KEY      "qos"
#define PUBSUB_UTILS_QOS_TYPE_SAMPLE        "sample"    /* A.k.a. unreliable connection */
#define PUBSUB_UTILS_QOS_TYPE_CONTROL       "control"   /* A.k.a. reliable connection */
#define PUBSUB_UTILS_PSA_SEND_DELAY         "PSA_SEND_DELAY"
#define PUBSUB_UTILS_PSA_DEFAULT_SEND_DELAY 250 //  250 ms

/**
 * Returns the pubsub info from the provided filter. A pubsub filter should have a topic and can 
 * have a scope. If no topic is present the topic and scope output will be NULL.
 * If a topic is present the topic output will contain an allocated topic string and if a scope was
 * present an allocated scope string.
 * The caller is owner of the topic and scope output string.
 */
celix_status_t pubsub_getPubSubInfoFromFilter(const char* filterstr, char **scope, char **topic);

/**
 * Loop through all bundles and look for the bundle with the keys inside.
 * If no key bundle found, return NULL
 *
 * Caller is responsible for freeing the object
 */
char* pubsub_getKeysBundleDir(celix_bundle_context_t *ctx);

/**
 * Tries to find and read the topic properties for the provided bundle.
 *
 * Will look at  META-INF/topics/pub/<topic>.properties for publisher and
 * META-INF/topics/sub/<topic>.properties for subscribers.
 *
 * The caller is owner of the returned topic properties.
 *
 * @param bundle         The bundle where the properties reside.
 * @param scope          The scope name.
 * @param topic          The topic name.
 * @param isPublisher    true if the topic properties for a publisher should be found.
 * @return               The topic properties if found or NULL.
 */
celix_properties_t* pubsub_utils_getTopicProperties(const celix_bundle_t *bundle, const char *scope, const char *topic, bool isPublisher);

/**
 * Returns a msg id from the provided fully qualified name.
 *
 *
 * @param fqn fully qualified name
 * @return    msg id
 */
unsigned int pubsub_msgIdHashFromFqn(const char* fqn);

/**
 * Returns the directory where the descriptor for messages can be found.
 *
 * @param ctx The bundle context.
 * @param bnd The bundle to get descriptor root dir from.
 * @return NULL if the directory does not exists. Caller is owner of the data.
 */
char* pubsub_getMessageDescriptorsDir(celix_bundle_context_t*ctx, const celix_bundle_t *bnd);

/**
 * Combines the name of key with topic and optionally scope and retrieves the value of that environment variable or framework property
 * @param ctx The bundle context
 * @param key start of the key name, e.g. PUBSUB_TCP_STATIC_BIND_URL_FOR
 * @param topic Name of topic. NULL NOT allowed
 * @param scope Name of scope. NULL allowed
 * @return Value of environment variable name, NULL if variable not defined/found.
 */
const char* pubsub_getEnvironmentVariableWithScopeTopic(celix_bundle_context_t* ctx, const char *key, const char *topic, const char *scope);

#ifdef __cplusplus
}
#endif

#endif /* PUBSUB_UTILS_H_ */
