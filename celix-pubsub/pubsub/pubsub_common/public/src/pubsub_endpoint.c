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
#include "subscriber.h"

celix_status_t pubsubEndpoint_create(const char* fwUUID, const char* scope, const char* topic, long serviceId, const char* endpoint, pubsub_endpoint_pt* psEp) {
    celix_status_t status = CELIX_SUCCESS;
    *psEp = calloc(1, sizeof(**psEp));

    if (fwUUID != NULL) {
        (*psEp)->frameworkUUID = strdup(fwUUID);
    }

    if (scope != NULL) {
        (*psEp)->scope = strdup(scope);
    }

    if (topic != NULL) {
        (*psEp)->topic = strdup(topic);
    }

    (*psEp)->serviceID = serviceId;

    if (endpoint != NULL) {
        (*psEp)->endpoint = strdup(endpoint);
    }

    return status;

}

celix_status_t pubsubEndpoint_createFromServiceReference(service_reference_pt reference, pubsub_endpoint_pt* psEp){
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


	if(fwUUID!=NULL){
		(*psEp)->frameworkUUID=strdup(fwUUID);
	}

	if(scope!=NULL){
		(*psEp)->scope=strdup(scope);
	} else {
	    (*psEp)->scope=strdup(PUBSUB_SUBSCRIBER_SCOPE_DEFAULT);
	}

	if(topic!=NULL){
		(*psEp)->topic=strdup(topic);
	}

	if(serviceId!=NULL){
		(*psEp)->serviceID = strtol(serviceId,NULL,10);
	}

	if (!(*psEp)->frameworkUUID || !(*psEp)->serviceID || !(*psEp)->scope || !(*psEp)->topic) {
		fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "PUBSUB_ENDPOINT: incomplete description!.");
		status = CELIX_BUNDLE_EXCEPTION;
	}

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
