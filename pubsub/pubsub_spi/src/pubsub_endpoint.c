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
/*
 * endpoint_description.c
 *
 *  \date       25 Jul 2014
 *  \author     <a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright  Apache License, Version 2.0
 */

#include <string.h>
#include <stdlib.h>
#include <uuid/uuid.h>

#include "celix_errno.h"
#include "celix_log.h"

#include "pubsub_common.h"
#include "pubsub_endpoint.h"
#include "constants.h"

#include "pubsub_utils.h"


static void pubsubEndpoint_setFields(pubsub_endpoint_pt psEp, const char* fwUUID, const char* scope, const char* topic, long bundleId, long serviceId,const char* endpoint, const char *pubsubType, properties_pt topic_props);
static properties_pt pubsubEndpoint_getTopicProperties(bundle_pt bundle, const char *topic, bool isPublisher);
static bool pubsubEndpoint_isEndpointValid(pubsub_endpoint_pt psEp);

static void pubsubEndpoint_setFields(pubsub_endpoint_pt psEp, const char* fwUUID, const char* scope, const char* topic, long bundleId, long serviceId, const char* endpoint, const char *pubsubType, properties_pt topic_props) {

	if (psEp->endpoint_props == NULL) {
		psEp->endpoint_props = properties_create();
	}

	char endpointUuid[37];

	uuid_t endpointUid;
	uuid_generate(endpointUid);
	uuid_unparse(endpointUid, endpointUuid);

	properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_UUID, endpointUuid);

	if (fwUUID != NULL) {
		properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID, fwUUID);
	}

	if (scope != NULL) {
		properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_TOPIC_SCOPE, scope);
	}

	if (topic != NULL) {
		properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_TOPIC_NAME, topic);
	}

    char idBuf[32];

    if (bundleId >= 0) {
        snprintf(idBuf, sizeof(idBuf), "%li", bundleId);
        properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_BUNDLE_ID, idBuf);
    }

    if (serviceId >= 0) {
        snprintf(idBuf, sizeof(idBuf), "%li", bundleId);
        properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_SERVICE_ID, idBuf);
    }

	if(endpoint != NULL) {
		properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_URL, endpoint);
	}

    if (pubsubType != NULL) {
        properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_TYPE, pubsubType);
    }

	if(topic_props != NULL) {
        properties_copy(topic_props, &(psEp->topic_props));
	}
}

static properties_pt pubsubEndpoint_getTopicProperties(bundle_pt bundle, const char *topic, bool isPublisher){

	properties_pt topic_props = NULL;

	bool isSystemBundle = false;
	bundle_isSystemBundle(bundle, &isSystemBundle);
	long bundleId = -1;
	bundle_isSystemBundle(bundle, &isSystemBundle);
	bundle_getBundleId(bundle,&bundleId);

	if(isSystemBundle == false) {

		char *bundleRoot = NULL;
		char* topicPropertiesPath = NULL;
		bundle_getEntry(bundle, ".", &bundleRoot);

		if(bundleRoot != NULL){

			asprintf(&topicPropertiesPath, "%s/META-INF/topics/%s/%s.properties", bundleRoot, isPublisher?"pub":"sub", topic);
			topic_props = properties_load(topicPropertiesPath);
			if(topic_props==NULL){
				printf("PSEP: Could not load properties for %s on topic %s, bundleId=%ld\n", isPublisher?"publication":"subscription", topic,bundleId);
			}

			free(topicPropertiesPath);
			free(bundleRoot);
		}
	}

	return topic_props;
}

celix_status_t pubsubEndpoint_setField(pubsub_endpoint_pt ep, const char* key, const char* value) {
	celix_status_t status = CELIX_SUCCESS;

	if (ep->endpoint_props == NULL) {
		printf("PUBSUB_EP: No endpoint_props for endpoint available!\n");
		return CELIX_ILLEGAL_STATE;
	}

	if (key != NULL && value != NULL) {
		properties_set(ep->endpoint_props, key, value);
	}

	return status;
}

celix_status_t pubsubEndpoint_create(const char* fwUUID, const char* scope, const char* topic, long bundleId,  long serviceId, const char* endpoint, const char* pubsubType, properties_pt topic_props,pubsub_endpoint_pt* out){
	celix_status_t status = CELIX_SUCCESS;

    pubsub_endpoint_pt psEp = calloc(1, sizeof(*psEp));

	pubsubEndpoint_setFields(psEp, fwUUID, scope, topic, bundleId, serviceId, endpoint, pubsubType, topic_props);

    if (!pubsubEndpoint_isEndpointValid(psEp)) {
        status = CELIX_ILLEGAL_STATE;
    }

    if (status == CELIX_SUCCESS) {
        *out = psEp;
    } else {
        pubsubEndpoint_destroy(psEp);
    }

	return status;

}

celix_status_t pubsubEndpoint_clone(pubsub_endpoint_pt in, pubsub_endpoint_pt *out){
	celix_status_t status = CELIX_SUCCESS;

    pubsub_endpoint_pt ep = calloc(1,sizeof(*ep));

	status = properties_copy(in->endpoint_props, &(ep->endpoint_props));

    if (in->topic_props != NULL) {
        status += properties_copy(in->topic_props, &(ep->topic_props));
    }

    if (status == CELIX_SUCCESS) {
        *out = ep;
    } else {
        pubsubEndpoint_destroy(ep);
    }

	return status;
}

celix_status_t pubsubEndpoint_createFromServiceReference(bundle_context_t *ctx, service_reference_pt reference, bool isPublisher, pubsub_endpoint_pt* out){
	celix_status_t status = CELIX_SUCCESS;

	pubsub_endpoint_pt ep = calloc(1,sizeof(*ep));

	const char* fwUUID = NULL;
	bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, &fwUUID);

	const char* scope = NULL;
	serviceReference_getPropertyWithDefault(reference, PUBSUB_SUBSCRIBER_SCOPE, PUBSUB_SUBSCRIBER_SCOPE_DEFAULT, &scope);

	const char* topic = NULL;
	serviceReference_getProperty(reference, PUBSUB_SUBSCRIBER_TOPIC,&topic);

	const char* serviceId = NULL;
	serviceReference_getProperty(reference,(char*)OSGI_FRAMEWORK_SERVICE_ID,&serviceId);


    long bundleId = -1;
    bundle_pt bundle = NULL;
    serviceReference_getBundle(reference, &bundle);
    if (bundle != NULL) {
        bundle_getBundleId(bundle, &bundleId);
    }

	/* TODO: is topic_props==NULL a fatal error such that EP cannot be created? */
	properties_pt topic_props = pubsubEndpoint_getTopicProperties(bundle, topic, isPublisher);

    const char *pubsubType = isPublisher ? PUBSUB_PUBLISHER_ENDPOINT_TYPE : PUBSUB_SUBSCRIBER_ENDPOINT_TYPE;

	pubsubEndpoint_setFields(ep, fwUUID, scope, topic, bundleId, strtol(serviceId,NULL,10), NULL, pubsubType, topic_props);

    if (!pubsubEndpoint_isEndpointValid(ep)) {
        status = CELIX_ILLEGAL_STATE;
    }

    if (status == CELIX_SUCCESS) {
        *out = ep;
    } else {
        pubsubEndpoint_destroy(ep);
    }

	return status;

}

celix_status_t pubsubEndpoint_createFromDiscoveredProperties(properties_t *discoveredProperties, pubsub_endpoint_pt* out) {
    celix_status_t status = CELIX_SUCCESS;
    
    pubsub_endpoint_pt psEp = calloc(1, sizeof(*psEp));
    
    if (psEp == NULL) {
        return CELIX_ENOMEM;
    }

    psEp->endpoint_props = discoveredProperties;

    if (!pubsubEndpoint_isEndpointValid(psEp)) {
        status = CELIX_ILLEGAL_STATE;
    }

    if (status == CELIX_SUCCESS) {
        *out = psEp;
    } else {
        pubsubEndpoint_destroy(psEp);
    }

    return status;
}

celix_status_t pubsubEndpoint_createFromListenerHookInfo(bundle_context_t *ctx, listener_hook_info_pt info, bool isPublisher, pubsub_endpoint_pt* out){
	celix_status_t status = CELIX_SUCCESS;

	const char* fwUUID=NULL;
	bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, &fwUUID);

	if( fwUUID==NULL) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	char* topic = NULL;
	char* scope = NULL;
	pubsub_getPubSubInfoFromFilter(info->filter, &topic, &scope);

	if (topic==NULL) {
		free(scope);
		return CELIX_BUNDLE_EXCEPTION;
	}
	if (scope == NULL) {
		scope = strdup(PUBSUB_PUBLISHER_SCOPE_DEFAULT);
	}

        pubsub_endpoint_pt psEp = calloc(1, sizeof(**out));

	bundle_pt bundle = NULL;
	long bundleId = -1;
	bundleContext_getBundle(info->context,&bundle);
	bundle_getBundleId(bundle,&bundleId);

	properties_pt topic_props = pubsubEndpoint_getTopicProperties(bundle, topic, isPublisher);

	/* TODO: is topic_props==NULL a fatal error such that EP cannot be created? */
	pubsubEndpoint_setFields(psEp, fwUUID, scope, topic, bundleId, -1, NULL, PUBSUB_PUBLISHER_ENDPOINT_TYPE, topic_props);
	free(scope);
	free(topic);

    if (!pubsubEndpoint_isEndpointValid(psEp)) {
        status = CELIX_ILLEGAL_STATE;
    }

    if (status == CELIX_SUCCESS) {
        *out = psEp;
    } else {
        pubsubEndpoint_destroy(psEp);
    }

	return status;
}

void pubsubEndpoint_destroy(pubsub_endpoint_pt psEp){
    if (psEp == NULL) return;

	if(psEp->topic_props != NULL){
		properties_destroy(psEp->topic_props);
	}

	if (psEp->endpoint_props != NULL) {
		properties_destroy(psEp->endpoint_props);
    }

	free(psEp);

	return;

}

bool pubsubEndpoint_equals(pubsub_endpoint_pt psEp1,pubsub_endpoint_pt psEp2){

	if (psEp1->endpoint_props && psEp2->endpoint_props) {
		return !strcmp(properties_get(psEp1->endpoint_props, PUBSUB_ENDPOINT_UUID),
					  properties_get(psEp2->endpoint_props, PUBSUB_ENDPOINT_UUID));
	}else {
		return false;
	}
}

char * pubsubEndpoint_createScopeTopicKey(const char* scope, const char* topic) {
	char *result = NULL;
	asprintf(&result, "%s:%s", scope, topic);

	return result;
}


static bool pubsubEndpoint_isEndpointValid(pubsub_endpoint_pt psEp) {
    //required properties
    bool valid = true;
    static const char* keys[] = {
        PUBSUB_ENDPOINT_UUID,
        PUBSUB_ENDPOINT_FRAMEWORK_UUID,
        PUBSUB_ENDPOINT_TYPE,
        PUBSUB_ENDPOINT_TOPIC_NAME,
        PUBSUB_ENDPOINT_TOPIC_SCOPE,
        NULL };
    int i;
    for (i = 0; keys[i] != NULL; ++i) {
        const char *val = properties_get(psEp->endpoint_props, keys[i]);
        if (val == NULL) { //missing required key
            fprintf(stderr, "[ERROR] PubSubEndpoint: Invalid endpoint missing key: '%s'\n", keys[i]);
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
