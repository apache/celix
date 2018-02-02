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


static void pubsubEndpoint_setFields(pubsub_endpoint_pt psEp, const char* fwUUID, const char* scope, const char* topic, long serviceId,const char* endpoint,properties_pt topic_props, bool cloneProps);
static properties_pt pubsubEndpoint_getTopicProperties(bundle_pt bundle, const char *topic, bool isPublisher);

static void pubsubEndpoint_setFields(pubsub_endpoint_pt psEp, const char* fwUUID, const char* scope, const char* topic, long serviceId,const char* endpoint,properties_pt topic_props, bool cloneProps){

	if (psEp->endpoint_props == NULL) {
		psEp->endpoint_props = properties_create();
	}

	char endpointUuid[37];

	uuid_t endpointUid;
	uuid_generate(endpointUid);
	uuid_unparse(endpointUid, endpointUuid);
	properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_ID, endpointUuid);

	if (fwUUID != NULL) {
		properties_set(psEp->endpoint_props, OSGI_FRAMEWORK_FRAMEWORK_UUID, fwUUID);
	}

	if (scope != NULL) {
		properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_SCOPE, scope);
	}

	if (topic != NULL) {
		properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_TOPIC, topic);
	}

	psEp->serviceID = serviceId;

	if(endpoint != NULL) {
		properties_set(psEp->endpoint_props, PUBSUB_ENDPOINT_URL, endpoint);
	}

	if(topic_props != NULL){
		if(cloneProps){
			properties_copy(topic_props, &(psEp->topic_props));
		}
		else{
			psEp->topic_props = topic_props;
		}
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

celix_status_t pubsubEndpoint_create(const char* fwUUID, const char* scope, const char* topic, long serviceId,const char* endpoint,properties_pt topic_props,pubsub_endpoint_pt* psEp){
	celix_status_t status = CELIX_SUCCESS;

	*psEp = calloc(1, sizeof(**psEp));

	pubsubEndpoint_setFields(*psEp, fwUUID, scope, topic, serviceId, endpoint, topic_props, true);

	return status;

}

celix_status_t pubsubEndpoint_clone(pubsub_endpoint_pt in, pubsub_endpoint_pt *out){
	celix_status_t status = CELIX_SUCCESS;

    pubsub_endpoint_pt ep = calloc(1,sizeof(*ep));

	status = properties_copy(in->endpoint_props, &(ep->endpoint_props));

    if (in->topic_props != NULL) {
        status += properties_copy(in->topic_props, &(ep->topic_props));
    }

	ep->serviceID = in->serviceID;
	ep->is_secure = in->is_secure;

    if (status == CELIX_SUCCESS) {
        *out = ep;
    } else {
        pubsubEndpoint_destroy(ep);
    }

	return status;

}

celix_status_t pubsubEndpoint_createFromServiceReference(service_reference_pt reference, pubsub_endpoint_pt* psEp, bool isPublisher){
	celix_status_t status = CELIX_SUCCESS;

	pubsub_endpoint_pt ep = calloc(1,sizeof(*ep));

	bundle_pt bundle = NULL;
	bundle_context_pt ctxt = NULL;
	const char* fwUUID = NULL;
	serviceReference_getBundle(reference,&bundle);
	bundle_getContext(bundle,&ctxt);
	bundleContext_getProperty(ctxt,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);

	const char* scope = NULL;
	serviceReference_getProperty(reference, PUBSUB_SUBSCRIBER_SCOPE,&scope);

	const char* topic = NULL;
	serviceReference_getProperty(reference, PUBSUB_SUBSCRIBER_TOPIC,&topic);

	const char* serviceId = NULL;
	serviceReference_getProperty(reference,(char*)OSGI_FRAMEWORK_SERVICE_ID,&serviceId);

	/* TODO: is topic_props==NULL a fatal error such that EP cannot be created? */
	properties_pt topic_props = pubsubEndpoint_getTopicProperties(bundle, topic, isPublisher);

	pubsubEndpoint_setFields(ep, fwUUID, scope!=NULL?scope:PUBSUB_SUBSCRIBER_SCOPE_DEFAULT, topic, strtol(serviceId,NULL,10), NULL, topic_props, false);

	if (!properties_get(ep->endpoint_props, OSGI_FRAMEWORK_FRAMEWORK_UUID) ||
			!ep->serviceID ||
			!properties_get(ep->endpoint_props, PUBSUB_ENDPOINT_SCOPE) ||
			!properties_get(ep->endpoint_props, PUBSUB_ENDPOINT_TOPIC)) {

		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "PUBSUB_ENDPOINT: incomplete description!.");
		status = CELIX_BUNDLE_EXCEPTION;
		pubsubEndpoint_destroy(ep);
		*psEp = NULL;
	}
	else{
		*psEp = ep;
	}

	return status;

}

celix_status_t pubsubEndpoint_createFromListenerHookInfo(listener_hook_info_pt info,pubsub_endpoint_pt* psEp, bool isPublisher){
	celix_status_t status = CELIX_SUCCESS;

	const char* fwUUID=NULL;
	bundleContext_getProperty(info->context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);

	if(fwUUID==NULL){
		return CELIX_BUNDLE_EXCEPTION;
	}

	char* topic = pubsub_getTopicFromFilter(info->filter);
	if(topic==NULL){
		return CELIX_BUNDLE_EXCEPTION;
	}

	*psEp = calloc(1, sizeof(**psEp));

	char* scope = pubsub_getScopeFromFilter(info->filter);
	if(scope == NULL) {
		scope = strdup(PUBSUB_PUBLISHER_SCOPE_DEFAULT);
	}

	bundle_pt bundle = NULL;
	long bundleId = -1;
	bundleContext_getBundle(info->context,&bundle);

	bundle_getBundleId(bundle,&bundleId);

	properties_pt topic_props = pubsubEndpoint_getTopicProperties(bundle, topic, isPublisher);

	/* TODO: is topic_props==NULL a fatal error such that EP cannot be created? */
	pubsubEndpoint_setFields(*psEp, fwUUID, scope!=NULL?scope:PUBSUB_SUBSCRIBER_SCOPE_DEFAULT, topic, bundleId, NULL, topic_props, false);

	free(topic);
	free(scope);


	return status;
}

celix_status_t pubsubEndpoint_destroy(pubsub_endpoint_pt psEp){

	if(psEp->topic_props != NULL){
		properties_destroy(psEp->topic_props);
	}

	if (psEp->endpoint_props != NULL) {
		properties_destroy(psEp->endpoint_props);
    }

	free(psEp);

	return CELIX_SUCCESS;

}

bool pubsubEndpoint_equals(pubsub_endpoint_pt psEp1,pubsub_endpoint_pt psEp2){

	return ((strcmp(properties_get(psEp1->endpoint_props, OSGI_FRAMEWORK_FRAMEWORK_UUID),properties_get(psEp2->endpoint_props, OSGI_FRAMEWORK_FRAMEWORK_UUID))==0) &&
			(strcmp(properties_get(psEp1->endpoint_props, PUBSUB_ENDPOINT_SCOPE),properties_get(psEp2->endpoint_props, PUBSUB_ENDPOINT_SCOPE))==0) &&
			(strcmp(properties_get(psEp1->endpoint_props, PUBSUB_ENDPOINT_TOPIC),properties_get(psEp2->endpoint_props, PUBSUB_ENDPOINT_TOPIC))==0) &&
			(psEp1->serviceID == psEp2->serviceID) /*&&
			((psEp1->endpoint==NULL && psEp2->endpoint==NULL)||(strcmp(psEp1->endpoint,psEp2->endpoint)==0))*/
	);
}

char *createScopeTopicKey(const char* scope, const char* topic) {
	char *result = NULL;
	asprintf(&result, "%s:%s", scope, topic);

	return result;
}
