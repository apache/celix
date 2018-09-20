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
#include <celix_api.h>

#include "celix_errno.h"
#include "celix_log.h"

#include "pubsub_common.h"
#include "pubsub_endpoint.h"
#include "constants.h"

#include "pubsub_utils.h"


static void pubsubEndpoint_setFields(pubsub_endpoint_pt psEp, const char* fwUUID, const char* scope, const char* topic, const char *pubsubType, const char *admin, const char *ser, const celix_properties_t* topic_props);
static properties_pt pubsubEndpoint_getTopicProperties(bundle_pt bundle, const char *topic, bool isPublisher);

static void pubsubEndpoint_setFields(pubsub_endpoint_pt psEp, const char* fwUUID, const char* scope, const char* topic, const char *pubsubType, const char *adminType, const char *serType, const celix_properties_t *topic_props) {

	if (psEp->properties == NULL) {
		if (topic_props != NULL) {
			psEp->properties = celix_properties_copy(topic_props);
		} else {
			psEp->properties = properties_create();
		}
	}

	char endpointUuid[37];
	uuid_t endpointUid;
	uuid_generate(endpointUid);
	uuid_unparse(endpointUid, endpointUuid);
	celix_properties_set(psEp->properties, PUBSUB_ENDPOINT_UUID, endpointUuid);

	if (fwUUID != NULL) {
		celix_properties_set(psEp->properties, PUBSUB_ENDPOINT_FRAMEWORK_UUID, fwUUID);
	}

	if (scope != NULL) {
		celix_properties_set(psEp->properties, PUBSUB_ENDPOINT_TOPIC_SCOPE, scope);
	}

	if (topic != NULL) {
		celix_properties_set(psEp->properties, PUBSUB_ENDPOINT_TOPIC_NAME, topic);
	}

	if (pubsubType != NULL) {
		celix_properties_set(psEp->properties, PUBSUB_ENDPOINT_TYPE, pubsubType);
	}

	if (adminType != NULL) {
		celix_properties_set(psEp->properties, PUBSUB_ENDPOINT_ADMIN_TYPE, adminType);
	}

	if (serType != NULL) {
		celix_properties_set(psEp->properties, PUBSUB_ENDPOINT_SERIALIZER, serType);
	}

	psEp->topicName = celix_properties_get(psEp->properties, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
	psEp->topicScope = celix_properties_get(psEp->properties, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL);
	psEp->uuid = celix_properties_get(psEp->properties, PUBSUB_ENDPOINT_UUID, NULL);
	psEp->frameworkUUid = celix_properties_get(psEp->properties, PUBSUB_ENDPOINT_FRAMEWORK_UUID, NULL);
	psEp->type = celix_properties_get(psEp->properties, PUBSUB_ENDPOINT_TYPE, NULL);
	psEp->adminType = celix_properties_get(psEp->properties, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
	psEp->serializerType = celix_properties_get(psEp->properties, PUBSUB_ENDPOINT_SERIALIZER, NULL);
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

celix_status_t pubsubEndpoint_create(const char* fwUUID, const char* scope, const char* topic, const char* pubsubType, const char* adminType, const char *serType, celix_properties_t *topic_props, pubsub_endpoint_t **out) {
	celix_status_t status = CELIX_SUCCESS;

	pubsub_endpoint_t *psEp = calloc(1, sizeof(*psEp));

	pubsubEndpoint_setFields(psEp, fwUUID, scope, topic, pubsubType, adminType, serType, topic_props);

	if (!pubsubEndpoint_isValid(psEp->properties, true, true)) {
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
	return pubsubEndpoint_createFromProperties(in->properties, out);
}

celix_status_t pubsubEndpoint_createFromSvc(bundle_context_t* ctx, const celix_bundle_t *bnd, const celix_properties_t *svcProps, bool isPublisher, pubsub_endpoint_pt* out) {
	celix_status_t status = CELIX_SUCCESS;

	pubsub_endpoint_pt ep = calloc(1,sizeof(*ep));

	const char* fwUUID = celix_bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, NULL);
	const char* scope = celix_properties_get(svcProps,  PUBSUB_SUBSCRIBER_SCOPE, PUBSUB_SUBSCRIBER_SCOPE_DEFAULT);
	const char* topic = celix_properties_get(svcProps,  PUBSUB_SUBSCRIBER_TOPIC, NULL);

	/* TODO: is topic_props==NULL a fatal error such that EP cannot be created? */
	celix_properties_t *topic_props = pubsubEndpoint_getTopicProperties((celix_bundle_t*)bnd, topic, isPublisher);

	const char *pubsubType = isPublisher ? PUBSUB_PUBLISHER_ENDPOINT_TYPE : PUBSUB_SUBSCRIBER_ENDPOINT_TYPE;

	pubsubEndpoint_setFields(ep, fwUUID, scope, topic, pubsubType, NULL, NULL, topic_props);
	if(topic_props != NULL){
		celix_properties_destroy(topic_props); //Can be deleted since setFields invokes properties_copy
	}

	if (!pubsubEndpoint_isValid(ep->properties, true, true)) {
		status = CELIX_ILLEGAL_STATE;
	}

	if (status == CELIX_SUCCESS) {
		*out = ep;
	} else {
		pubsubEndpoint_destroy(ep);
	}

	return status;

}

struct retrieve_topic_properties_data {
	celix_properties_t *props;
	const char *topic;
	bool isPublisher;
};

static void retrieveTopicProperties(void *handle, const celix_bundle_t *bnd) {
	struct retrieve_topic_properties_data *data = handle;
	data->props = pubsubEndpoint_getTopicProperties((bundle_pt)bnd, data->topic, data->isPublisher);
}

celix_status_t pubsubEndpoint_createFromListenerHookInfo(bundle_context_t *ctx, const celix_service_tracker_info_t *info, bool isPublisher, pubsub_endpoint_pt* out) {
	celix_status_t status = CELIX_SUCCESS;

	const char* fwUUID=NULL;
	bundleContext_getProperty(ctx, OSGI_FRAMEWORK_FRAMEWORK_UUID, &fwUUID);

	if( fwUUID==NULL) {
		return CELIX_BUNDLE_EXCEPTION;
	}

	char* topic = NULL;
	char* scope = NULL;
	const char *filterStr = celix_filter_getFilterString(info->filter);
	pubsub_getPubSubInfoFromFilter(filterStr, &topic, &scope);

	if (topic==NULL) {
		free(scope);
		return CELIX_BUNDLE_EXCEPTION;
	}
	if (scope == NULL) {
		scope = strdup(PUBSUB_PUBLISHER_SCOPE_DEFAULT);
	}

	pubsub_endpoint_pt psEp = calloc(1, sizeof(**out));

	struct retrieve_topic_properties_data data;
	data.props = NULL;
	data.isPublisher = isPublisher;
	data.topic = topic;
	celix_bundleContext_useBundle(ctx, info->bundleId, &data, retrieveTopicProperties);

	/* TODO: is topic_props==NULL a fatal error such that EP cannot be created? */
	pubsubEndpoint_setFields(psEp, fwUUID, scope, topic, PUBSUB_PUBLISHER_ENDPOINT_TYPE, NULL, NULL, data.props);
	free(scope);
	free(topic);
	if (data.props != NULL) {
		celix_properties_destroy(data.props); //Can be deleted since setFields invokes properties_copy
	}

	if (!pubsubEndpoint_isValid(psEp->properties, false, false)) {
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

	if(psEp->properties != NULL){
		celix_properties_destroy(psEp->properties);
	}

	free(psEp);
}

bool pubsubEndpoint_equals(pubsub_endpoint_pt psEp1,pubsub_endpoint_pt psEp2){
	if (psEp1 && psEp2) {
		return pubsubEndpoint_equalsWithProperties(psEp1, psEp2->properties);
	} else {
		return false;
	}
}

bool pubsubEndpoint_equalsWithProperties(pubsub_endpoint_pt psEp1, const celix_properties_t *props) {
	if (psEp1->properties && props) {
		int cmp = strcmp(celix_properties_get(psEp1->properties, PUBSUB_ENDPOINT_UUID, "entry1"),
						 celix_properties_get(props, PUBSUB_ENDPOINT_UUID, "entry2"));
		return cmp == 0;
	} else {
		return false;
	}
}

char * pubsubEndpoint_createScopeTopicKey(const char* scope, const char* topic) {
	char *result = NULL;
	asprintf(&result, "%s:%s", scope, topic);

	return result;
}

celix_status_t pubsubEndpoint_createFromProperties(const celix_properties_t *props, pubsub_endpoint_t **out) {
	pubsub_endpoint_t *ep = calloc(1, sizeof(*ep));
	pubsubEndpoint_setFields(ep, NULL, NULL, NULL, NULL, NULL, NULL, props);
	bool valid = pubsubEndpoint_isValid(ep->properties, true, true);
    if (valid) {
        *out = ep;
	} else {
		*out = NULL;
		pubsubEndpoint_destroy(ep);
		return CELIX_BUNDLE_EXCEPTION;
	}
	return CELIX_SUCCESS;
}

static bool checkProp(const celix_properties_t *props, const char *key) {
	const char *val = celix_properties_get(props, key, NULL);
	if (val == NULL) {
		fprintf(stderr, "[Error] Missing mandatory entry for endpoint. Missing key is '%s'\n", key);
	}
	return val != NULL;
}


bool pubsubEndpoint_isValid(const celix_properties_t *props, bool requireAdminType, bool requireSerializerType) {
	bool p1 = checkProp(props, PUBSUB_ENDPOINT_UUID);
	bool p2 = checkProp(props, PUBSUB_ENDPOINT_FRAMEWORK_UUID);
	bool p3 = checkProp(props, PUBSUB_ENDPOINT_TYPE);
	bool p4 = true;
	if (requireAdminType) {
		checkProp(props, PUBSUB_ENDPOINT_ADMIN_TYPE);
	}
	bool p5 = true;
	if (requireSerializerType) {
		checkProp(props, PUBSUB_ENDPOINT_SERIALIZER);
	}
	bool p6 = checkProp(props, PUBSUB_ENDPOINT_TOPIC_NAME);
	bool p7 = checkProp(props, PUBSUB_ENDPOINT_TOPIC_SCOPE);

	return p1 && p2 && p3 && p4 && p5 && p6 && p7;
}

void pubsubEndpoint_setField(pubsub_endpoint_t *ep, const char *key, const char *val) {
    if (ep != NULL) {
        celix_properties_set(ep->properties, key, val);

        ep->topicName = celix_properties_get(ep->properties, PUBSUB_ENDPOINT_TOPIC_NAME, NULL);
        ep->topicScope = celix_properties_get(ep->properties, PUBSUB_ENDPOINT_TOPIC_SCOPE, NULL);
        ep->uuid = celix_properties_get(ep->properties, PUBSUB_ENDPOINT_UUID, NULL);
        ep->frameworkUUid = celix_properties_get(ep->properties, PUBSUB_ENDPOINT_FRAMEWORK_UUID, NULL);
        ep->type = celix_properties_get(ep->properties, PUBSUB_ENDPOINT_TYPE, NULL);
        ep->adminType = celix_properties_get(ep->properties, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
        ep->serializerType = celix_properties_get(ep->properties, PUBSUB_ENDPOINT_SERIALIZER, NULL);
    }
}