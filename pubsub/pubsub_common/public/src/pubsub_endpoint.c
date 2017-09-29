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

#include "celix_errno.h"
#include "celix_log.h"

#include "pubsub_common.h"
#include "pubsub_endpoint.h"
#include "constants.h"

#include "pubsub_utils.h"


static void pubsubEndpoint_setFields(pubsub_endpoint_pt psEp, const char* fwUUID, const char* scope, const char* topic, long serviceId,const char* endpoint,properties_pt topic_props, bool cloneProps);
static properties_pt pubsubEndpoint_getTopicProperties(bundle_pt bundle, const char *topic, bool isPublisher);

static void pubsubEndpoint_setFields(pubsub_endpoint_pt psEp, const char* fwUUID, const char* scope, const char* topic, long serviceId,const char* endpoint,properties_pt topic_props, bool cloneProps){

	if (fwUUID != NULL) {
		psEp->frameworkUUID = strdup(fwUUID);
	}

	if (scope != NULL) {
		psEp->scope = strdup(scope);
	}

	if (topic != NULL) {
		psEp->topic = strdup(topic);
	}

	psEp->serviceID = serviceId;

	if(endpoint != NULL) {
		psEp->endpoint = strdup(endpoint);
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

celix_status_t pubsubEndpoint_create(const char* fwUUID, const char* scope, const char* topic, long serviceId,const char* endpoint,properties_pt topic_props,pubsub_endpoint_pt* psEp){
	celix_status_t status = CELIX_SUCCESS;

	*psEp = calloc(1, sizeof(**psEp));

	pubsubEndpoint_setFields(*psEp, fwUUID, scope, topic, serviceId, endpoint, topic_props, true);

	return status;

}

celix_status_t pubsubEndpoint_clone(pubsub_endpoint_pt in, pubsub_endpoint_pt *out){
	celix_status_t status = CELIX_SUCCESS;

	*out = calloc(1,sizeof(**out));

	pubsubEndpoint_setFields(*out, in->frameworkUUID, in->scope, in->topic, in->serviceID, in->endpoint, in->topic_props, true);

	return status;

}

celix_status_t pubsubEndpoint_createFromServiceReference(service_reference_pt reference, pubsub_endpoint_pt* psEp, bool isPublisher){
	celix_status_t status = CELIX_SUCCESS;

	*psEp = calloc(1,sizeof(**psEp));

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

	pubsubEndpoint_setFields(*psEp, fwUUID, scope!=NULL?scope:PUBSUB_SUBSCRIBER_SCOPE_DEFAULT, topic, strtol(serviceId,NULL,10), NULL, topic_props, false);

	if (!(*psEp)->frameworkUUID || !(*psEp)->serviceID || !(*psEp)->scope || !(*psEp)->topic) {
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "PUBSUB_ENDPOINT: incomplete description!.");
		status = CELIX_BUNDLE_EXCEPTION;
	}

	return status;

}

celix_status_t pubsubEndpoint_createFromListenerHookInfo(listener_hook_info_pt info,pubsub_endpoint_pt* psEp, bool isPublisher){
	celix_status_t status = CELIX_SUCCESS;

	const char* fwUUID=NULL;
	bundleContext_getProperty(info->context,OSGI_FRAMEWORK_FRAMEWORK_UUID,&fwUUID);

	char* topic = pubsub_getTopicFromFilter(info->filter);
	if(topic==NULL || fwUUID==NULL){
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

	if(psEp->frameworkUUID!=NULL){
		free(psEp->frameworkUUID);
		psEp->frameworkUUID = NULL;
	}

	if(psEp->scope!=NULL){
		free(psEp->scope);
		psEp->scope = NULL;
	}

	if(psEp->topic!=NULL){
		free(psEp->topic);
		psEp->topic = NULL;
	}

	if(psEp->endpoint!=NULL){
		free(psEp->endpoint);
		psEp->endpoint = NULL;
	}

	if(psEp->topic_props != NULL){
		properties_destroy(psEp->topic_props);
	}

	free(psEp);

	return CELIX_SUCCESS;

}

bool pubsubEndpoint_equals(pubsub_endpoint_pt psEp1,pubsub_endpoint_pt psEp2){

	return ((strcmp(psEp1->frameworkUUID,psEp2->frameworkUUID)==0) &&
			(strcmp(psEp1->scope,psEp2->scope)==0) &&
			(strcmp(psEp1->topic,psEp2->topic)==0) &&
			(psEp1->serviceID == psEp2->serviceID) /*&&
			((psEp1->endpoint==NULL && psEp2->endpoint==NULL)||(strcmp(psEp1->endpoint,psEp2->endpoint)==0))*/
	);
}

char *createScopeTopicKey(const char* scope, const char* topic) {
	char *result = NULL;
	asprintf(&result, "%s:%s", scope, topic);

	return result;
}
