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
 * calculator_endpoint_impl.c
 *
 *  \date       Oct 7, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <jansson.h>
#include <string.h>

#include "celix_errno.h"

#include "calculator_endpoint_impl.h"

celix_status_t calculatorEndpoint_create(remote_endpoint_pt *endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	*endpoint = calloc(1, sizeof(**endpoint));
	if (!*endpoint) {
		status = CELIX_ENOMEM;
	} else {
		(*endpoint)->service = NULL;
	}

	return status;
}

celix_status_t calculatorEndpoint_destroy(remote_endpoint_pt *endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	free(*endpoint);
	*endpoint = NULL;

	return status;
}

celix_status_t calculatorEndpoint_setService(remote_endpoint_pt endpoint, void *service) {
	celix_status_t status = CELIX_SUCCESS;
	endpoint->service = service;
	return status;
}

/**
 * Request: http://host:port/services/{service}
 */
celix_status_t calculatorEndpoint_handleRequest(remote_endpoint_pt endpoint, char *data, char **reply) {
	celix_status_t status = CELIX_SUCCESS;
	json_error_t jsonError;
    json_t *root = json_loads(data, 0, &jsonError);
    const char *sig;
    json_unpack(root, "{s:s}", "m", &sig);

	printf("CALCULATOR_ENDPOINT: Handle request \"%s\" with data \"%s\"\n", sig, data);
	if (strcmp(sig, "add(DD)D") == 0) {
		calculatorEndpoint_add(endpoint, data, reply);
	} else if (strcmp(sig, "sub(DD)D") == 0) {
		calculatorEndpoint_sub(endpoint, data, reply);
	} else if (strcmp(sig, "sqrt(D)D") == 0) {
		calculatorEndpoint_sqrt(endpoint, data, reply);
	} else {
		status = CELIX_ILLEGAL_ARGUMENT;
	}

	json_decref(root);

	return status;
}

celix_status_t calculatorEndpoint_add(remote_endpoint_pt endpoint, char *data, char **reply) {
	celix_status_t status = CELIX_SUCCESS;
	json_error_t jsonError;
	json_t *root;

	root = json_loads(data, 0, &jsonError);
	if (!root) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		double a;
		double b;
		json_unpack(root, "{s:[ff]}", "a", &a, &b);

		if (endpoint->service != NULL) {
			double result;
			json_t *resultRoot;
			calculator_service_pt service = endpoint->service;
			service->add(service->calculator, a, b, &result);
			resultRoot = json_pack("{s:f}", "r", result);

			char *c = json_dumps(resultRoot, 0);
			*reply = c;

			json_decref(resultRoot);
		} else {
			printf("CALCULATOR_ENDPOINT: No service available");
			status = CELIX_BUNDLE_EXCEPTION;
		}
		json_decref(root);
	}

	return status;
}

celix_status_t calculatorEndpoint_sub(remote_endpoint_pt endpoint, char *data, char **reply) {
	celix_status_t status = CELIX_SUCCESS;
	json_error_t jsonError;
	json_t *root;

	root = json_loads(data, 0, &jsonError);
	if (!root) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		double a;
		double b;
		json_unpack(root, "{s:[ff]}", "a", &a, &b);

		if (endpoint->service != NULL) {
			double result;
			json_t *resultRoot;
			calculator_service_pt service = endpoint->service;
			service->sub(service->calculator, a, b, &result);
			resultRoot = json_pack("{s:f}", "r", result);

			char *c = json_dumps(resultRoot, JSON_ENCODE_ANY);
			*reply = c;

			json_decref(resultRoot);
		} else {
			printf("CALCULATOR_ENDPOINT: No service available");
			status = CELIX_BUNDLE_EXCEPTION;
		}
		json_decref(root);
	}

	return status;
}

celix_status_t calculatorEndpoint_sqrt(remote_endpoint_pt endpoint, char *data, char **reply) {
	celix_status_t status = CELIX_SUCCESS;
	json_error_t jsonError;
	json_t *root;

	root = json_loads(data, 0, &jsonError);
	if (!root) {
		status = CELIX_ILLEGAL_ARGUMENT;
	} else {
		double a;
		json_unpack(root, "{s:[f]}", "a", &a);

		if (endpoint->service != NULL) {
			double result;
			json_t *resultRoot;
			calculator_service_pt service = endpoint->service;
			service->sqrt(service->calculator, a, &result);
			resultRoot = json_pack("{s:f}", "r", result);

			char *c = json_dumps(resultRoot, JSON_ENCODE_ANY);
			*reply = c;

			json_decref(resultRoot);
		} else {
			printf("CALCULATOR_ENDPOINT: No service available");
			status = CELIX_BUNDLE_EXCEPTION;
		}
		json_decref(root);
	}

	return status;
}
