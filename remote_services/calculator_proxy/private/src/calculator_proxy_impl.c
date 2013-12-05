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
 * calculator_proxy_impl.c
 *
 *  \date       Oct 7, 2011
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <jansson.h>

#include <string.h>
#include <apr_strings.h>

#include <stddef.h>

#include "celix_errno.h"

#include "calculator_proxy_impl.h"

celix_status_t calculatorProxy_create(apr_pool_t *pool, calculator_pt *calculator)  {
	celix_status_t status = CELIX_SUCCESS;
	*calculator = apr_palloc(pool, sizeof(**calculator));
	if (!*calculator) {
		status = CELIX_ENOMEM;
	} else {
		(*calculator)->pool = pool;
		(*calculator)->endpoint = NULL;
		(*calculator)->sendToCallback=NULL;
		(*calculator)->sendToHandler=NULL;
	}

	return status;
}

celix_status_t calculatorProxy_add(calculator_pt calculator, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;

	if (calculator->endpoint != NULL) {
		json_t *root;
		root = json_pack("{s:f, s:f}", "arg0", a, "arg1", b);

		char *data = json_dumps(root, 0);
		char *reply = calloc(128, sizeof(char));
		int replyStatus = 0;

		calculator->sendToCallback(calculator->sendToHandler, calculator->endpoint, "add", data, &reply, &replyStatus);

		if (status == CELIX_SUCCESS) {
			json_error_t jsonError;
			json_t *js_reply = json_loads(reply, 0, &jsonError);
            json_unpack(js_reply, "[f]", result);
		}

	} else {
		printf("CALCULATOR_PROXY: No endpoint information available\n");
	}

	return status;
}

celix_status_t calculatorProxy_sub(calculator_pt calculator, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;
	if (calculator->endpoint != NULL) {
		json_t *root;
		root = json_pack("{s:f, s:f}", "arg0", a, "arg1", b);

		char *data = json_dumps(root, 0);
		char *reply = calloc(128, sizeof(char));
		int replyStatus = 0;

		calculator->sendToCallback(calculator->sendToHandler, calculator->endpoint, "sub", data, &reply, &replyStatus);

		if (status == CELIX_SUCCESS) {
			json_error_t jsonError;
			json_t *js_reply = json_loads(reply, 0, &jsonError);
			json_unpack(js_reply, "[f]", result);
		}
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available\n");
	}

	return status;
}

celix_status_t calculatorProxy_sqrt(calculator_pt calculator, double a, double *result) {
	celix_status_t status = CELIX_SUCCESS;
	if (calculator->endpoint != NULL) {
		json_t *root;
		root = json_pack("{s:f}", "arg0", a);

		char *data = json_dumps(root, 0);
		char *reply = calloc(128, sizeof(char));
		int replyStatus;

		calculator->sendToCallback(calculator->sendToHandler, calculator->endpoint, "sqrt", data, &reply, &replyStatus);

		if (status == CELIX_SUCCESS) {
			json_error_t jsonError;
			json_t *js_reply = json_loads(reply, 0, &jsonError);
			json_unpack(js_reply, "[f]", result);
		}
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available\n");
	}

	return status;
}

celix_status_t calculatorProxy_setEndpointDescription(void *proxy, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	calculator_pt calculator = proxy;
	calculator->endpoint = endpoint;

	return status;
}


celix_status_t calculatorProxy_setHandler(void *proxy, void *handler) {
	celix_status_t status = CELIX_SUCCESS;

	calculator_pt calculator = proxy;
	calculator->sendToHandler = handler;

	return status;
}


celix_status_t calculatorProxy_setCallback(void *proxy, sendToHandle callback) {
	celix_status_t status = CELIX_SUCCESS;

	calculator_pt calculator = proxy;
	calculator->sendToCallback = callback;

	return status;
}
