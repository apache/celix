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
 * example_endpoint.c
 *
 *  \date       Oct 7, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <jansson.h>

#include "celix_errno.h"

#include "example_endpoint_impl.h"

#include "example_service.h"

celix_status_t exampleEndpoint_create(apr_pool_t *pool, remote_endpoint_t *endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	*endpoint = apr_palloc(pool, sizeof(**endpoint));
	if (!*endpoint) {
		status = CELIX_ENOMEM;
	} else {
		(*endpoint)->service = NULL;
	}


	return status;
}

celix_status_t exampleEndpoint_setService(remote_endpoint_t endpoint, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	endpoint->service = service;
	return status;
}

/**
 * Request: http://host:port/services/{service}/{request}
 */
celix_status_t exampleEndpoint_handleRequest(remote_endpoint_t ep, char *request, char *data, char **reply) {
	celix_status_t status = CELIX_SUCCESS;

	printf("EXAMPLE ENDPOINT: Handle request \"%s\" with data \"%s\"\n", request, data);
	if (strcmp(request, "add") == 0) {
		exampleEndpoint_add(ep, data, reply);
	} else if (strcmp(request, "sub") == 0) {
		exampleEndpoint_sub(ep, data, reply);
	} else if (strcmp(request, "sqrt") == 0) {
		exampleEndpoint_sqrt(ep, data, reply);
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

/**
 * data = { "a" : 1.1, "b" : 2.4 }
 * reply = 3.5
 */
celix_status_t exampleEndpoint_add(remote_endpoint_t ep, char *data, char **reply) {
	celix_status_t status = CELIX_SUCCESS;
	json_error_t jsonError;
	json_t *root;

	root = json_loads(data, 0, &jsonError);
	if (!root) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		double a;
		double b;
		json_unpack(root, "{s:f, s:f}", "a", &a, "b", &b);

		if (ep->service != NULL) {
			double result;
			json_t *resultRoot;
			example_service_t service = ep->service;
			service->add(service->example, a, b, &result);
			resultRoot = json_pack("{s:f}", "result", result);

			char *c = json_dumps(resultRoot, JSON_ENCODE_ANY);
			*reply = c;
		} else {
			printf("EXAMPLE Endpoint: No service available");
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	return status;
}

celix_status_t exampleEndpoint_sub(remote_endpoint_t ep, char *data, char **reply) {
	celix_status_t status = CELIX_SUCCESS;
	json_error_t jsonError;
	json_t *root;

	root = json_loads(data, 0, &jsonError);
	if (!root) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		double a;
		double b;
		json_unpack(root, "{s:f, s:f}", "a", &a, "b", &b);

		if (ep->service != NULL) {
			double result;
			json_t *resultRoot;
			example_service_t service = ep->service;
			service->sub(service->example, a, b, &result);
			resultRoot = json_pack("{s:f}", "result", result);

			char *c = json_dumps(resultRoot, JSON_ENCODE_ANY);
			*reply = c;
		} else {
			printf("EXAMPLE Endpoint: No service available");
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	return status;
}

celix_status_t exampleEndpoint_sqrt(remote_endpoint_t ep, char *data, char **reply) {
	celix_status_t status = CELIX_SUCCESS;
	json_error_t jsonError;
	json_t *root;

	root = json_loads(data, 0, &jsonError);
	if (!root) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		double a;
		json_unpack(root, "{s:f}", "a", &a);

		if (ep->service != NULL) {
			double result;
			json_t *resultRoot;
			example_service_t service = ep->service;
			service->sqrt(service->example, a, &result);
			resultRoot = json_pack("{s:f}", "result", result);

			char *c = json_dumps(resultRoot, JSON_ENCODE_ANY);
			*reply = c;
		} else {
			printf("EXAMPLE Endpoint: No service available");
			status = CELIX_BUNDLE_EXCEPTION;
		}
	}

	return status;
}
