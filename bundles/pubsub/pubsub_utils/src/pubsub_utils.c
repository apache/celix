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

#include <string.h>
#include <stdlib.h>

#include "celix_constants.h"
#include "celix_filter.h"
#include "filter.h"

#include "pubsub/publisher.h"
#include "pubsub_utils.h"

#include "array_list.h"
#include "celix_bundle.h"

#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <stdint.h>
#include "celix_utils.h"

#define MAX_KEYBUNDLE_LENGTH 256

/**
 * The config/env options to set the extender path which is used to find
 * descriptors if the requesting bundle is the framework bundle.
 */
#define SYSTEM_BUNDLE_ARCHIVE_PATH  "CELIX_FRAMEWORK_EXTENDER_PATH"


celix_status_t pubsub_getPubSubInfoFromFilter(const char* filterstr, char **scopeOut, char **topicOut) {
    celix_status_t status = CELIX_SUCCESS;
    const char *scope = NULL;
    const char *topic = NULL;
    const char *objectClass = NULL;
    celix_filter_t *filter = celix_filter_create(filterstr);
    scope = celix_filter_findAttribute(filter, PUBSUB_PUBLISHER_SCOPE);
    topic = celix_filter_findAttribute(filter, PUBSUB_PUBLISHER_TOPIC);
    objectClass = celix_filter_findAttribute(filter, OSGI_FRAMEWORK_OBJECTCLASS);

    if (topic != NULL && objectClass != NULL && strncmp(objectClass, PUBSUB_PUBLISHER_SERVICE_NAME, 128) == 0) {
        //NOTE topic must be present, scope can be present in the filter
        *topicOut = celix_utils_strdup(topic);
        if (scope != NULL) {
            if (strncmp("*", scope, 2) == 0) {
                //if scope attribute is present with a *, assume this is a negative test e.g. (&(topic=foo)(!(scope=*)))
                *scopeOut = NULL;
            } else {
                *scopeOut = celix_utils_strdup(scope);
            }
        } else {
            *scopeOut = NULL;
        }
    } else {
        *topicOut = NULL;
        *scopeOut = NULL;
    }

    if (filter != NULL) {
         filter_destroy(filter);
    }
    return status;
}


/**
 * Loop through all bundles and look for the bundle with the keys inside.
 * If no key bundle found, return NULL
 *
 * Caller is responsible for freeing the object
 */
char* pubsub_getKeysBundleDir(celix_bundle_context_t *ctx) {
    array_list_pt bundles = NULL;
    bundleContext_getBundles(ctx, &bundles);
    uint32_t nrOfBundles = arrayList_size(bundles);
    long bundle_id = -1;
    char* result = NULL;

    for (int i = 0; i < nrOfBundles; i++) {
        celix_bundle_t *b = arrayList_get(bundles, i);

        /* Skip bundle 0 (framework bundle) since it has no path nor revisions */
        bundle_getBundleId(b, &bundle_id);
        if (bundle_id == 0) {
            continue;
        }

        char* dir = NULL;
        bundle_getEntry(b, ".", &dir);

        char cert_dir[MAX_KEYBUNDLE_LENGTH];
        snprintf(cert_dir, MAX_KEYBUNDLE_LENGTH, "%s/META-INF/keys", dir);

        struct stat s;
        int err = stat(cert_dir, &s);
        if (err != -1) {
            if (S_ISDIR(s.st_mode)) {
                result = dir;
                break;
            }
        }

        free(dir);
    }

    arrayList_destroy(bundles);

    return result;
}

celix_properties_t *pubsub_utils_getTopicProperties(const celix_bundle_t *bundle, const char *scope, const char *topic, bool isPublisher) {
    celix_properties_t *topic_props = NULL;

    bool isSystemBundle = false;
    bundle_isSystemBundle((celix_bundle_t *)bundle, &isSystemBundle);
    long bundleId = -1;
    bundle_isSystemBundle((celix_bundle_t *)bundle, &isSystemBundle);
    bundle_getBundleId((celix_bundle_t *)bundle,&bundleId);

    if (isSystemBundle == false) {
        char *bundleRoot = NULL;
        char *topicPropertiesPath = NULL;
        bundle_getEntry((celix_bundle_t *)bundle, ".", &bundleRoot);

        if (bundleRoot != NULL) {
            if (scope) {
                asprintf(&topicPropertiesPath, "%s/META-INF/topics/%s/%s.%s.properties", bundleRoot, isPublisher? "pub":"sub", scope, topic);
                topic_props = celix_properties_load(topicPropertiesPath);
            }

            if (topic_props == NULL) {
              free(topicPropertiesPath);
              asprintf(&topicPropertiesPath, "%s/META-INF/topics/%s/%s.properties", bundleRoot, isPublisher ? "pub" : "sub", topic);
              topic_props = celix_properties_load(topicPropertiesPath);
              if (topic_props == NULL) {
                  printf("[Debug] PubSub: Could not load properties for %s on scope %s / topic %s. Searched location %s, bundleId=%ld\n", isPublisher ? "publication" : "subscription", scope == NULL ? "(null)" : scope, topic, topicPropertiesPath, bundleId);
              }
            }
            free(topicPropertiesPath);
            free(bundleRoot);
        }
    }

    return topic_props;
}

unsigned int pubsub_msgIdHashFromFqn(const char* fqn) {
    return celix_utils_stringHash(fqn);
}

char* pubsub_getMessageDescriptorsDir(celix_bundle_context_t* ctx, const celix_bundle_t *bnd) {
    char *root = NULL;

    bool isSystemBundle = celix_bundle_isSystemBundle(bnd);
    bundle_isSystemBundle(bnd, &isSystemBundle);

    if (isSystemBundle == true) {

        const char *dir = celix_bundleContext_getProperty(ctx, SYSTEM_BUNDLE_ARCHIVE_PATH, NULL);
        //asprintf(&root, "%s/META-INF/descriptors", dir);
        struct stat s;
        if (dir && stat(dir, &s) == 0) {
            //file does exist
            root = celix_utils_strdup(dir);
        }
    } else {
        root = celix_bundle_getEntry(bnd, "META-INF/descriptors");
    }
    return root;
}

const char* pubsub_getEnvironmentVariableWithScopeTopic(celix_bundle_context_t* ctx, const char *key, const char *topic, const char *scope) {
    char *combinedKey = malloc(512 * 1024);
    memset(combinedKey, 0, 512 * 1024);
    strncpy(combinedKey, key, 512 * 1024);
    strncat(combinedKey, topic, 256 * 1024 - 25);
    if(scope != NULL) {
        strncat(combinedKey, "_", 1);
        strncat(combinedKey, scope, 256 * 1024 - 25);
    }

    const char *ret = celix_bundleContext_getProperty(ctx, combinedKey, NULL);
    free(combinedKey);
    return ret;
}