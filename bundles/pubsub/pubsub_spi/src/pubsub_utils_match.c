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
#include <pubsub_endpoint.h>
#include <pubsub_serializer.h>

#include "service_reference.h"

#include "pubsub_admin.h"

#include "pubsub_utils.h"
#include "constants.h"

static double getPSAScore(const char *requested_admin, const char *request_qos, const char *adminType, double sampleScore, double controlScore, double defaultScore) {
	double score;
	if (requested_admin != NULL && strncmp(requested_admin, adminType, strlen(adminType)) == 0) {
		/* We got precise specification on the pubsub_admin we want */
		//Full match
		score = PUBSUB_ADMIN_FULL_MATCH_SCORE;
	} else if (requested_admin != NULL) {
		//admin type requested, but no match -> do not select this psa
		score = PUBSUB_ADMIN_NO_MATCH_SCORE;
	} else if (request_qos != NULL && strncmp(request_qos, PUBSUB_UTILS_QOS_TYPE_SAMPLE, strlen(PUBSUB_UTILS_QOS_TYPE_SAMPLE)) == 0) {
		//qos match
		score = sampleScore;
	} else if (request_qos != NULL && strncmp(request_qos, PUBSUB_UTILS_QOS_TYPE_CONTROL, strlen(PUBSUB_UTILS_QOS_TYPE_CONTROL)) == 0) {
		//qos match
		score = controlScore;
	} else if (request_qos != NULL) {
		//note unsupported qos -> defaultScore
		score = defaultScore;
	} else {
		//default match
		score = defaultScore;
	}
	return score;
}

struct psa_serializer_selection_data {
	const char *requested_serializer;
	long matchingSvcId;
};

void psa_serializer_selection_callback(void *handle, void *svc __attribute__((unused)), const celix_properties_t *props) {
	struct psa_serializer_selection_data *data = handle;
	const char *serType = celix_properties_get(props, PUBSUB_SERIALIZER_TYPE_KEY, NULL);
	if (serType == NULL) {
		fprintf(stderr, "Warning found serializer without mandatory serializer type key (%s)\n", PUBSUB_SERIALIZER_TYPE_KEY);
	} else {
		if (strncmp(data->requested_serializer, serType, 1024 * 1024) == 0) {
			data->matchingSvcId = celix_properties_getAsLong(props, OSGI_FRAMEWORK_SERVICE_ID, -1L);
		}
	}
}

static long getPSASerializer(celix_bundle_context_t *ctx, const char *requested_serializer) {
	long svcId;

	if (requested_serializer != NULL) {
		struct psa_serializer_selection_data data;
		data.requested_serializer = requested_serializer;
		data.matchingSvcId = -1L;

		celix_service_use_options_t opts = CELIX_EMPTY_SERVICE_USE_OPTIONS;
		opts.filter.serviceName = PUBSUB_SERIALIZER_SERVICE_NAME;
		opts.filter.ignoreServiceLanguage = true;
		opts.callbackHandle = &data;
		opts.useWithProperties = psa_serializer_selection_callback;
		celix_bundleContext_useServicesWithOptions(ctx, &opts);
		svcId = data.matchingSvcId;
	} else {
		celix_service_filter_options_t opts = CELIX_EMPTY_SERVICE_FILTER_OPTIONS;
		opts.serviceName = PUBSUB_SERIALIZER_SERVICE_NAME;
		opts.ignoreServiceLanguage = true;

		//note findService will automatically return the highest ranking service id
		svcId = celix_bundleContext_findServiceWithOptions(ctx, &opts);
	}

	return svcId;
}

double pubsub_utils_matchPublisher(
		celix_bundle_context_t *ctx,
		long bundleId,
		const char *filter,
		const char *adminType,
		double sampleScore,
		double controlScore,
		double defaultScore,
		long *outSerializerSvcId) {

	celix_properties_t *ep = pubsubEndpoint_createFromPublisherTrackerInfo(ctx, bundleId, filter);
	const char *requested_admin 		= NULL;
	const char *requested_qos			= NULL;
	if (ep != NULL) {
		requested_admin = celix_properties_get(ep, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
		requested_qos = celix_properties_get(ep, PUBSUB_UTILS_QOS_ATTRIBUTE_KEY, NULL);
	}

	double score = getPSAScore(requested_admin, requested_qos, adminType, sampleScore, controlScore, defaultScore);

	const char *requested_serializer = celix_properties_get(ep, PUBSUB_ENDPOINT_SERIALIZER, NULL);
	long serializerSvcId = getPSASerializer(ctx, requested_serializer);

	if (serializerSvcId < 0) {
		score = PUBSUB_ADMIN_NO_MATCH_SCORE; //no serializer, no match
	}

//	printf("Score publisher service for psa type %s is %f\n", adminType, score);

	if (outSerializerSvcId != NULL) {
		*outSerializerSvcId = serializerSvcId;
	}

	if (ep != NULL) {
		celix_properties_destroy(ep); //TODO improve function to that tmp endpoint is not needed -> parse filter
	}

	return score;
}

typedef struct pubsub_match_retrieve_topic_properties_data {
	const char *topic;
	bool isPublisher;

	celix_properties_t *outEndpoint;
} pubsub_get_topic_properties_data_t;

static void getTopicPropertiesCallback(void *handle, const celix_bundle_t *bnd) {
	pubsub_get_topic_properties_data_t *data = handle;
	data->outEndpoint = pubsub_utils_getTopicProperties(bnd, data->topic, data->isPublisher);
}

double pubsub_utils_matchSubscriber(
		celix_bundle_context_t *ctx,
		const long svcProviderBundleId,
		const celix_properties_t *svcProperties,
		const char *adminType,
		double sampleScore,
		double controlScore,
		double defaultScore,
		long *outSerializerSvcId) {

	pubsub_get_topic_properties_data_t data;
	data.isPublisher = false;
	data.topic = celix_properties_get(svcProperties, PUBSUB_SUBSCRIBER_TOPIC, NULL);
	data.outEndpoint = NULL;
	celix_bundleContext_useBundle(ctx, svcProviderBundleId, &data, getTopicPropertiesCallback);

	celix_properties_t *ep = data.outEndpoint;
	const char *requested_admin 		= NULL;
	const char *requested_qos			= NULL;
	const char *requested_serializer 	= NULL;
	if (ep != NULL) {
		requested_admin = celix_properties_get(ep, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
		requested_qos = celix_properties_get(ep, PUBSUB_UTILS_QOS_ATTRIBUTE_KEY, NULL);
		requested_serializer = celix_properties_get(ep, PUBSUB_ENDPOINT_SERIALIZER, NULL);
	}

	double score = getPSAScore(requested_admin, requested_qos, adminType, sampleScore, controlScore, defaultScore);

	long serializerSvcId = getPSASerializer(ctx, requested_serializer);
	if (serializerSvcId < 0) {
		score = PUBSUB_ADMIN_NO_MATCH_SCORE; //no serializer, no match
	}

	if (outSerializerSvcId != NULL) {
		*outSerializerSvcId = serializerSvcId;
	}

	if (ep != NULL) {
		celix_properties_destroy(ep);
	}

	return score;
}

bool pubsub_utils_matchEndpoint(
		celix_bundle_context_t *ctx,
		const celix_properties_t *ep,
		const char *adminType,
		long *outSerializerSvcId) {

	bool psaMatch = false;
	const char *configured_admin = celix_properties_get(ep, PUBSUB_ENDPOINT_ADMIN_TYPE, NULL);
	if (configured_admin != NULL) {
		psaMatch = strncmp(configured_admin, adminType, strlen(adminType)) == 0;
	}

	bool serMatch = false;
	long serializerSvcId = -1L;
	if (psaMatch) {
		const char *configured_serializer = celix_properties_get(ep, PUBSUB_ENDPOINT_SERIALIZER, NULL);
		serializerSvcId = getPSASerializer(ctx, configured_serializer);
		serMatch = serializerSvcId >= 0;
	}

	bool match = psaMatch && serMatch;
//	printf("Match for endpoint for psa type %s is %s\n", adminType, match ? "true" : "false");

	if (outSerializerSvcId != NULL) {
		*outSerializerSvcId = serializerSvcId;
	}

	return match;
}
