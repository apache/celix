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
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdbool.h>
#include <netdb.h>
#include <netinet/in.h>

#include "constants.h"
#include "celix_threads.h"
#include "bundle_context.h"
#include "array_list.h"
#include "utils.h"
#include "celix_errno.h"
#include "filter.h"
#include "service_reference.h"
#include "service_registration.h"

#include "publisher_endpoint_announce.h"
#include "etcd_common.h"
#include "etcd_watcher.h"
#include "etcd_writer.h"
#include "pubsub_endpoint.h"
#include "pubsub_discovery_impl.h"

static bool pubsub_discovery_isEndpointValid(pubsub_endpoint_pt psEp);

/* Discovery activator functions */
celix_status_t pubsub_discovery_create(bundle_context_pt context, pubsub_discovery_pt *ps_discovery) {
    celix_status_t status = CELIX_SUCCESS;

    *ps_discovery = calloc(1, sizeof(**ps_discovery));

    if (*ps_discovery == NULL) {
        return CELIX_ENOMEM;
    }

    (*ps_discovery)->context = context;
    (*ps_discovery)->discoveredPubs = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
    (*ps_discovery)->listenerReferences = hashMap_create(serviceReference_hashCode, NULL, serviceReference_equals2, NULL);
    (*ps_discovery)->watchers = hashMap_create(utils_stringHash,NULL,utils_stringEquals, NULL);
    (*ps_discovery)->verbose = PUBSUB_ETCD_DISCOVERY_DEFAULT_VERBOSE;
    celixThreadMutex_create(&(*ps_discovery)->listenerReferencesMutex, NULL);
    celixThreadMutex_create(&(*ps_discovery)->discoveredPubsMutex, NULL);
    celixThreadMutex_create(&(*ps_discovery)->watchersMutex, NULL);

    const char *verboseStr = NULL;
    bundleContext_getProperty(context, PUBSUB_ETCD_DISCOVERY_VERBOSE_KEY, &verboseStr);
    if (verboseStr != NULL) {
        (*ps_discovery)->verbose = strncasecmp("true", verboseStr, strlen("true")) == 0;
    }

    return status;
}

celix_status_t pubsub_discovery_destroy(pubsub_discovery_pt ps_discovery) {
    celix_status_t status = CELIX_SUCCESS;

    celixThreadMutex_lock(&ps_discovery->discoveredPubsMutex);

    hash_map_iterator_pt iter = hashMapIterator_create(ps_discovery->discoveredPubs);

    while (hashMapIterator_hasNext(iter)) {
        array_list_pt pubEP_list = (array_list_pt) hashMapIterator_nextValue(iter);

        for(int i=0; i < arrayList_size(pubEP_list); i++) {
            pubsubEndpoint_destroy(((pubsub_endpoint_pt)arrayList_get(pubEP_list,i)));
        }
        arrayList_destroy(pubEP_list);
    }

    hashMapIterator_destroy(iter);

    hashMap_destroy(ps_discovery->discoveredPubs, true, false);
    ps_discovery->discoveredPubs = NULL;

    celixThreadMutex_unlock(&ps_discovery->discoveredPubsMutex);

    celixThreadMutex_destroy(&ps_discovery->discoveredPubsMutex);


    celixThreadMutex_lock(&ps_discovery->listenerReferencesMutex);

    hashMap_destroy(ps_discovery->listenerReferences, false, false);
    ps_discovery->listenerReferences = NULL;

    celixThreadMutex_unlock(&ps_discovery->listenerReferencesMutex);

    celixThreadMutex_destroy(&ps_discovery->listenerReferencesMutex);

    free(ps_discovery);

    return status;
}

celix_status_t pubsub_discovery_start(pubsub_discovery_pt ps_discovery) {
    celix_status_t status = CELIX_SUCCESS;
    status = etcdCommon_init(ps_discovery->context);
    ps_discovery->writer = etcdWriter_create(ps_discovery);

    return status;
}

celix_status_t pubsub_discovery_stop(pubsub_discovery_pt ps_discovery) {
    celix_status_t status = CELIX_SUCCESS;

    const char* fwUUID = NULL;

    bundleContext_getProperty(ps_discovery->context, OSGI_FRAMEWORK_FRAMEWORK_UUID, &fwUUID);
    if (fwUUID == NULL) {
        fprintf(stderr, "ERROR PSD: Cannot retrieve fwUUID.\n");
        return CELIX_INVALID_BUNDLE_CONTEXT;
    }

    celixThreadMutex_lock(&ps_discovery->watchersMutex);

    hash_map_iterator_pt iter = hashMapIterator_create(ps_discovery->watchers);
    while (hashMapIterator_hasNext(iter)) {
        struct watcher_info * wi = hashMapIterator_nextValue(iter);
        etcdWatcher_stop(wi->watcher);
    }
    hashMapIterator_destroy(iter);

    celixThreadMutex_lock(&ps_discovery->discoveredPubsMutex);

    /* Unexport all publishers for the local framework, and also delete from ETCD publisher belonging to the local framework */

    iter = hashMapIterator_create(ps_discovery->discoveredPubs);
    while (hashMapIterator_hasNext(iter)) {
        array_list_pt pubEP_list = (array_list_pt) hashMapIterator_nextValue(iter);

        int i;
        for (i = 0; i < arrayList_size(pubEP_list); i++) {
            pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt) arrayList_get(pubEP_list, i);
            if (strcmp(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID), fwUUID) == 0) {
                etcdWriter_deletePublisherEndpoint(ps_discovery->writer, pubEP);
            } else {
                pubsub_discovery_informPublishersListeners(ps_discovery, pubEP, false);
                arrayList_remove(pubEP_list, i);
                pubsubEndpoint_destroy(pubEP);
                i--;
            }
        }
    }

    hashMapIterator_destroy(iter);

    celixThreadMutex_unlock(&ps_discovery->discoveredPubsMutex);
    etcdWriter_destroy(ps_discovery->writer);

    iter = hashMapIterator_create(ps_discovery->watchers);
    while (hashMapIterator_hasNext(iter)) {
        struct watcher_info * wi = hashMapIterator_nextValue(iter);
        etcdWatcher_destroy(wi->watcher);
    }
    hashMapIterator_destroy(iter);
    hashMap_destroy(ps_discovery->watchers, true, true);
    celixThreadMutex_unlock(&ps_discovery->watchersMutex);
    return status;
}

/* Functions called by the etcd_watcher */

celix_status_t pubsub_discovery_addNode(pubsub_discovery_pt pubsub_discovery, pubsub_endpoint_pt pubEP) {
    celix_status_t status = CELIX_SUCCESS;

    bool valid = pubsub_discovery_isEndpointValid(pubEP);
    if (!valid) {
        status = CELIX_ILLEGAL_STATE;
        return status;
    }

    bool inform = false;
    celixThreadMutex_lock(&pubsub_discovery->discoveredPubsMutex);

    char *pubs_key = pubsubEndpoint_createScopeTopicKey(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE), properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
    array_list_pt pubEP_list = (array_list_pt)hashMap_get(pubsub_discovery->discoveredPubs,pubs_key);
    if(pubEP_list==NULL){
        arrayList_create(&pubEP_list);
        arrayList_add(pubEP_list,pubEP);
        hashMap_put(pubsub_discovery->discoveredPubs,strdup(pubs_key),pubEP_list);
        inform=true;
    }
    else{
        int i;
        bool found = false;
        for(i=0;i<arrayList_size(pubEP_list) && !found;i++){
            found = pubsubEndpoint_equals(pubEP,(pubsub_endpoint_pt)arrayList_get(pubEP_list,i));
        }
        if(found){
            pubsubEndpoint_destroy(pubEP);
        }
        else{
            arrayList_add(pubEP_list,pubEP);
            inform=true;
        }
    }
    free(pubs_key);

    celixThreadMutex_unlock(&pubsub_discovery->discoveredPubsMutex);

    if(inform){
        status = pubsub_discovery_informPublishersListeners(pubsub_discovery,pubEP,true);
    }

    return status;
}

celix_status_t pubsub_discovery_removeNode(pubsub_discovery_pt pubsub_discovery, pubsub_endpoint_pt pubEP) {
    celix_status_t status = CELIX_SUCCESS;
    pubsub_endpoint_pt p = NULL;
    bool found = false;

    celixThreadMutex_lock(&pubsub_discovery->discoveredPubsMutex);
    char *pubs_key = pubsubEndpoint_createScopeTopicKey(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE), properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
    array_list_pt pubEP_list = (array_list_pt) hashMap_get(pubsub_discovery->discoveredPubs, pubs_key);
    free(pubs_key);
    if (pubEP_list == NULL) {
        printf("WARNING PSD: Cannot find any registered publisher for topic %s. Something is not consistent.\n",
                properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
        status = CELIX_ILLEGAL_STATE;
    } else {
        int i;

        for (i = 0; !found && i < arrayList_size(pubEP_list); i++) {
            p = arrayList_get(pubEP_list, i);
            found = pubsubEndpoint_equals(pubEP, p);
            if (found) {
                arrayList_remove(pubEP_list, i);
                pubsubEndpoint_destroy(p);
            }
        }
    }

    celixThreadMutex_unlock(&pubsub_discovery->discoveredPubsMutex);
    if (found) {
        status = pubsub_discovery_informPublishersListeners(pubsub_discovery, pubEP, false);
    }
    pubsubEndpoint_destroy(pubEP);

    return status;
}

/* Callback to the pubsub_topology_manager */
celix_status_t pubsub_discovery_informPublishersListeners(pubsub_discovery_pt pubsub_discovery, pubsub_endpoint_pt pubEP, bool epAdded) {
    celix_status_t status = CELIX_SUCCESS;

    // Inform listeners of new publisher endpoint
    celixThreadMutex_lock(&pubsub_discovery->listenerReferencesMutex);

    if (pubsub_discovery->listenerReferences != NULL) {
        hash_map_iterator_pt iter = hashMapIterator_create(pubsub_discovery->listenerReferences);
        while (hashMapIterator_hasNext(iter)) {
            service_reference_pt reference = hashMapIterator_nextKey(iter);

            publisher_endpoint_announce_pt listener = NULL;

            bundleContext_getService(pubsub_discovery->context, reference, (void**) &listener);
            if (epAdded) {
                listener->announcePublisher(listener->handle, pubEP);
            } else {
                listener->removePublisher(listener->handle, pubEP);
            }
            bundleContext_ungetService(pubsub_discovery->context, reference, NULL);
        }
        hashMapIterator_destroy(iter);
    }

    celixThreadMutex_unlock(&pubsub_discovery->listenerReferencesMutex);

    return status;
}


/* Service's functions implementation */
celix_status_t pubsub_discovery_announcePublisher(void *handle, pubsub_endpoint_pt pubEP) {
    celix_status_t status = CELIX_SUCCESS;
    pubsub_discovery_pt pubsub_discovery = (pubsub_discovery_pt) handle;

    bool valid = pubsub_discovery_isEndpointValid(pubEP);
    if (!valid) {
        status = CELIX_ILLEGAL_ARGUMENT;
        return status;
    }

    if (pubsub_discovery->verbose) {
        printf("pubsub_discovery_announcePublisher : %s / %s\n",
                properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME),
                properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_URL));
    }



    celixThreadMutex_lock(&pubsub_discovery->discoveredPubsMutex);

    char *pub_key = pubsubEndpoint_createScopeTopicKey(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
    array_list_pt pubEP_list = (array_list_pt)hashMap_get(pubsub_discovery->discoveredPubs,pub_key);

    if(pubEP_list==NULL){
        arrayList_create(&pubEP_list);
        hashMap_put(pubsub_discovery->discoveredPubs,strdup(pub_key),pubEP_list);
    }
    free(pub_key);
    pubsub_endpoint_pt p = NULL;
    pubsubEndpoint_clone(pubEP, &p);

    arrayList_add(pubEP_list,p);

    status = etcdWriter_addPublisherEndpoint(pubsub_discovery->writer,p,true);

    celixThreadMutex_unlock(&pubsub_discovery->discoveredPubsMutex);

    return status;
}

celix_status_t pubsub_discovery_removePublisher(void *handle, pubsub_endpoint_pt pubEP) {
    celix_status_t status = CELIX_SUCCESS;
    pubsub_discovery_pt pubsub_discovery = (pubsub_discovery_pt) handle;

    bool valid = pubsub_discovery_isEndpointValid(pubEP);
    if (!valid) {
        status = CELIX_ILLEGAL_ARGUMENT;
        return status;
    }

    celixThreadMutex_lock(&pubsub_discovery->discoveredPubsMutex);

    char *pub_key = pubsubEndpoint_createScopeTopicKey(properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE),properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
    array_list_pt pubEP_list = (array_list_pt)hashMap_get(pubsub_discovery->discoveredPubs,pub_key);
    free(pub_key);
    if(pubEP_list==NULL){
        printf("WARNING PSD: Cannot find any registered publisher for topic %s. Something is not consistent.\n",properties_get(pubEP->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME));
        status = CELIX_ILLEGAL_STATE;
    }
    else{

        int i;
        bool found = false;
        pubsub_endpoint_pt p = NULL;

        for(i=0;!found && i<arrayList_size(pubEP_list);i++){
            p = (pubsub_endpoint_pt)arrayList_get(pubEP_list,i);
            found = pubsubEndpoint_equals(pubEP,p);
        }

        if(!found){
            printf("WARNING PSD: Trying to remove a not existing endpoint. Something is not consistent.\n");
            status = CELIX_ILLEGAL_STATE;
        }
        else{

            arrayList_removeElement(pubEP_list,p);

            status = etcdWriter_deletePublisherEndpoint(pubsub_discovery->writer,p);

            pubsubEndpoint_destroy(p);
        }
    }

    celixThreadMutex_unlock(&pubsub_discovery->discoveredPubsMutex);

    return status;
}

celix_status_t pubsub_discovery_interestedInTopic(void *handle, const char* scope, const char* topic) {
    pubsub_discovery_pt pubsub_discovery = (pubsub_discovery_pt) handle;

    char *scope_topic_key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&pubsub_discovery->watchersMutex);
    struct watcher_info * wi = hashMap_get(pubsub_discovery->watchers, scope_topic_key);
    if(wi) {
        wi->nr_references++;
        free(scope_topic_key);
    } else {
        wi = calloc(1, sizeof(*wi));
        etcdWatcher_create(pubsub_discovery, pubsub_discovery->context, scope, topic, &wi->watcher);
        wi->nr_references = 1;
        hashMap_put(pubsub_discovery->watchers, scope_topic_key, wi);
    }

    celixThreadMutex_unlock(&pubsub_discovery->watchersMutex);

    return CELIX_SUCCESS;
}

celix_status_t pubsub_discovery_uninterestedInTopic(void *handle, const char* scope, const char* topic) {
    pubsub_discovery_pt pubsub_discovery = (pubsub_discovery_pt) handle;

    char *scope_topic_key = pubsubEndpoint_createScopeTopicKey(scope, topic);
    celixThreadMutex_lock(&pubsub_discovery->watchersMutex);

    hash_map_entry_pt entry =  hashMap_getEntry(pubsub_discovery->watchers, scope_topic_key);
    if(entry) {
        struct watcher_info * wi = hashMapEntry_getValue(entry);
        wi->nr_references--;
        if(wi->nr_references == 0) {
            char *key = hashMapEntry_getKey(entry);
            hashMap_remove(pubsub_discovery->watchers, scope_topic_key);
            free(key);
            free(scope_topic_key);
            etcdWatcher_stop(wi->watcher);
            etcdWatcher_destroy(wi->watcher);
            free(wi);
        }
    } else {
        fprintf(stderr, "[DISC] Inconsistency error: Removing unknown topic %s\n", topic);
    }
    celixThreadMutex_unlock(&pubsub_discovery->watchersMutex);
    return CELIX_SUCCESS;
}

/* pubsub_topology_manager tracker callbacks */

celix_status_t pubsub_discovery_tmPublisherAnnounceAdded(void * handle, service_reference_pt reference, void * service) {
    celix_status_t status = CELIX_SUCCESS;

    pubsub_discovery_pt pubsub_discovery = (pubsub_discovery_pt)handle;
    publisher_endpoint_announce_pt listener = (publisher_endpoint_announce_pt)service;

    celixThreadMutex_lock(&pubsub_discovery->discoveredPubsMutex);
    celixThreadMutex_lock(&pubsub_discovery->listenerReferencesMutex);

    /* Notify the PSTM about discovered publisher endpoints */
    hash_map_iterator_pt iter = hashMapIterator_create(pubsub_discovery->discoveredPubs);
    while(hashMapIterator_hasNext(iter)){
        array_list_pt pubEP_list = (array_list_pt)hashMapIterator_nextValue(iter);
        int i;
        for(i=0;i<arrayList_size(pubEP_list);i++){
            pubsub_endpoint_pt pubEP = (pubsub_endpoint_pt)arrayList_get(pubEP_list,i);
            status += listener->announcePublisher(listener->handle, pubEP);
        }
    }

    hashMapIterator_destroy(iter);

    hashMap_put(pubsub_discovery->listenerReferences, reference, NULL);

    celixThreadMutex_unlock(&pubsub_discovery->listenerReferencesMutex);
    celixThreadMutex_unlock(&pubsub_discovery->discoveredPubsMutex);

    if (pubsub_discovery->verbose) {
        printf("PSD: pubsub_tm_announce_publisher added.\n");
    }

    return status;
}

celix_status_t pubsub_discovery_tmPublisherAnnounceModified(void * handle, service_reference_pt reference, void * service) {
    celix_status_t status = CELIX_SUCCESS;

    status = pubsub_discovery_tmPublisherAnnounceRemoved(handle, reference, service);
    if (status == CELIX_SUCCESS) {
        status = pubsub_discovery_tmPublisherAnnounceAdded(handle, reference, service);
    }

    return status;
}

celix_status_t pubsub_discovery_tmPublisherAnnounceRemoved(void * handle, service_reference_pt reference, void * service) {
    celix_status_t status = CELIX_SUCCESS;
    pubsub_discovery_pt pubsub_discovery = handle;

    celixThreadMutex_lock(&pubsub_discovery->listenerReferencesMutex);

    if (pubsub_discovery->listenerReferences != NULL) {
        if (hashMap_remove(pubsub_discovery->listenerReferences, reference)) {
            if (pubsub_discovery->verbose) {
                printf("PSD: pubsub_tm_announce_publisher removed.\n");
            }
        }
    }
    celixThreadMutex_unlock(&pubsub_discovery->listenerReferencesMutex);

    return status;
}

static bool pubsub_discovery_isEndpointValid(pubsub_endpoint_pt psEp) {
    //required properties
    bool valid = true;
    static const char* keys[] = {
        PUBSUB_ENDPOINT_UUID,
        PUBSUB_ENDPOINT_FRAMEWORK_UUID,
        PUBSUB_ENDPOINT_TYPE,
        PUBSUB_ENDPOINT_ADMIN_TYPE,
        PUBSUB_ENDPOINT_SERIALIZER,
        PUBSUB_ENDPOINT_TOPIC_NAME,
        PUBSUB_ENDPOINT_TOPIC_SCOPE,
        NULL };
    int i;
    for (i = 0; keys[i] != NULL; ++i) {
        const char *val = properties_get(psEp->endpoint_props, keys[i]);
        if (val == NULL) { //missing required key
            fprintf(stderr, "[ERROR] PSD: Invalid endpoint missing key: '%s'\n", keys[i]);
            valid = false;
        }
    }
    if (!valid) {
        const char *key = NULL;
        fprintf(stderr, "PubSubEndpoint entries:\n");
        PROPERTIES_FOR_EACH(psEp->endpoint_props, key) {
            fprintf(stderr, "\t'%s' : '%s'\n", key, properties_get(psEp->endpoint_props, key));
        }
        if (psEp->topic_props != NULL) {
            fprintf(stderr, "PubSubEndpoint topic properties entries:\n");
            PROPERTIES_FOR_EACH(psEp->topic_props, key) {
                fprintf(stderr, "\t'%s' : '%s'\n", key, properties_get(psEp->topic_props, key));
            }
        }
    }
    return valid;
}
