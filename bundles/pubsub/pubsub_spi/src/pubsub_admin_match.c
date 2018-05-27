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


#include <string.h>
#include <limits.h>

#include "service_reference.h"

#include "pubsub_admin.h"

#include "pubsub_admin_match.h"
#include "constants.h"

/*
 * Match can be called by
 * a) a local registered pubsub_subscriber service
 * b) a local opened service tracker for a pubsub_publisher service
 * c) a remote found publisher endpoint
 * Note subscribers are not (yet) dicovered remotely
 */
celix_status_t pubsub_admin_match(
		pubsub_endpoint_pt endpoint,
		const char *pubsub_admin_type,
		const char *frameworkUuid,
		double sampleScore,
		double controlScore,
		double defaultScore,
		array_list_pt serializerList,
		double *out) {

	celix_status_t status = CELIX_SUCCESS;
	double score = 0;

	const char *endpointFrameworkUuid		= NULL;
	const char *endpointAdminType 			= NULL;

	const char *requested_admin_type 		= NULL;
	const char *requested_qos_type			= NULL;

	if (endpoint->endpoint_props != NULL) {
		endpointFrameworkUuid = properties_get(endpoint->endpoint_props, PUBSUB_ENDPOINT_FRAMEWORK_UUID);
		endpointAdminType = properties_get(endpoint->endpoint_props, PUBSUB_ENDPOINT_ADMIN_TYPE);
	}
	if (endpoint->topic_props != NULL) {
		requested_admin_type = properties_get(endpoint->topic_props, PUBSUB_ADMIN_TYPE_KEY);
		requested_qos_type = properties_get(endpoint->topic_props, QOS_ATTRIBUTE_KEY);
	}

	if (endpointFrameworkUuid != NULL && frameworkUuid != NULL && strncmp(frameworkUuid, endpointFrameworkUuid, 128) == 0) {
		//match for local subscriber or publisher

		/* Analyze the pubsub_admin */
		if (requested_admin_type != NULL) { /* We got precise specification on the pubsub_admin we want */
			if (strncmp(requested_admin_type, pubsub_admin_type, strlen(pubsub_admin_type)) == 0) { //Full match
				score = PUBSUB_ADMIN_FULL_MATCH_SCORE;
			}
		} else if (requested_qos_type != NULL) { /* We got QoS specification that will determine the selected PSA */
			if (strncmp(requested_qos_type, QOS_TYPE_SAMPLE, strlen(QOS_TYPE_SAMPLE)) == 0) {
				score = sampleScore;
			} else if (strncmp(requested_qos_type, QOS_TYPE_CONTROL, strlen(QOS_TYPE_CONTROL)) == 0) {
				score += controlScore;
			} else {
				printf("Unknown QoS type '%s'\n", requested_qos_type);
				status = CELIX_ILLEGAL_ARGUMENT;
			}
		} else { /* We got no specification: fallback to default score */
			score = defaultScore;
		}

		//NOTE serializer influence the score if a specific serializer is configured and not available.
		//get best serializer. This is based on service raking or requested serializer. In the case of a request NULL is return if not request match is found.
		service_reference_pt serSvcRef = NULL;
		pubsub_admin_get_best_serializer(endpoint->topic_props, serializerList, &serSvcRef);
		const char *serType = NULL; //for printing info
		if (serSvcRef == NULL) {
			score = 0;
		} else {
			serviceReference_getProperty(serSvcRef, PUBSUB_SERIALIZER_TYPE_KEY, &serType);
		}

		printf("Score for psa type %s is %f. Serializer used is '%s'\n", pubsub_admin_type, score, serType);
	} else {
		//remote publisher. score will be 0 or 100. nothing else.
		//TODO FIXME remote publisher should go through a different process. Currently it is confusing what to match
		if (endpointAdminType == NULL) {
			score = 0;

//			const char *key = NULL;
//			printf("Endpoint properties:\n");
//			PROPERTIES_FOR_EACH(endpoint->endpoint_props, key) {
//				printf("\t%s=%s\n", key, properties_get(endpoint->endpoint_props, key));
//			}

			fprintf(stderr, "WARNING PSA MATCH: remote publisher has no type. The key '%s' must be specified\n", PUBSUB_ENDPOINT_ADMIN_TYPE);
		} else  {
			score = strncmp(endpointAdminType, pubsub_admin_type, 1024) == 0 ? 100 : 0;
		}
		printf("Score for psa type %s is %f. Publisher is remote\n", pubsub_admin_type, score);
	}


	*out = score;

	return status;
}

celix_status_t pubsub_admin_get_best_serializer(properties_pt endpoint_props, array_list_pt serializerList, service_reference_pt *out){
	celix_status_t status = CELIX_SUCCESS;
	int i;
	const char *requested_serializer_type = NULL;

	if (endpoint_props != NULL){
		requested_serializer_type = properties_get(endpoint_props,PUBSUB_SERIALIZER_TYPE_KEY);
	}

	service_reference_pt svcRef = NULL;
    service_reference_pt best = NULL;
    long hightestRanking = LONG_MIN;

    if (requested_serializer_type != NULL) {
        for (i = 0; i < arrayList_size(serializerList); ++i) {
            svcRef = (service_reference_pt) arrayList_get(serializerList, 0);
			const char* currentSerType = NULL;
            serviceReference_getProperty(svcRef, PUBSUB_SERIALIZER_TYPE_KEY, &currentSerType);
            if (currentSerType != NULL && strncmp(requested_serializer_type, currentSerType, 128) == 0) {
                best = svcRef;
                break;
            }
        }
    } else {
        //no specific serializer request -> search for highest ranking serializer service
        for (i = 0; i < arrayList_size(serializerList); ++i) {
            svcRef = (service_reference_pt)arrayList_get(serializerList,0);
            const char *service_ranking_str  = NULL;
			const char* currentSerType = NULL;
            serviceReference_getProperty(svcRef, OSGI_FRAMEWORK_SERVICE_RANKING, &service_ranking_str);
			serviceReference_getProperty(svcRef, PUBSUB_SERIALIZER_TYPE_KEY, &currentSerType);
            long svcRanking = service_ranking_str == NULL ? LONG_MIN : strtol(service_ranking_str, NULL, 10);
            if (best == NULL || (svcRanking > hightestRanking && currentSerType != NULL)) {
                best = svcRef;
                hightestRanking = svcRanking;
            }
			if (currentSerType == NULL) {
				fprintf(stderr, "Invalid pubsub_serializer service. Must have a property '%s'\n", PUBSUB_SERIALIZER_TYPE_KEY);
			}
        }
    }

	*out = best;

    return status;
}