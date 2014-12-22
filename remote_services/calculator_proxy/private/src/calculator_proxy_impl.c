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
#include <stddef.h>

#include "celix_errno.h"
#include "array_list.h"
#include "calculator_proxy_impl.h"

/* Allows the use of Jansson < 2.3 */
#ifndef JSON_DECODE_ANY
#define JSON_DECODE_ANY 0
#endif

celix_status_t calculatorProxy_create(bundle_context_pt context, calculator_pt *calculator)  {
	celix_status_t status = CELIX_SUCCESS;
	*calculator = calloc(1, sizeof(**calculator));
	if (!*calculator) {
		status = CELIX_ENOMEM;
	} else {
		(*calculator)->context = context;
		(*calculator)->endpoint = NULL;
		(*calculator)->sendToCallback=NULL;
		(*calculator)->sendToHandler=NULL;
	}

	return status;
}


celix_status_t calculatorProxy_destroy(calculator_pt *calculator)  {
	celix_status_t status = CELIX_SUCCESS;

	free(*calculator);
	*calculator = NULL;

	return status;
}

// { "m": "" "a":["arg1", "arg2"] }
celix_status_t calculatorProxy_add(calculator_pt calculator, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;

	if (calculator->endpoint != NULL) {
		json_t *root;
		root = json_pack("{s:s, s:[ff]}", "m", "add(DD)D", "a", a, b);

		char *data = json_dumps(root, 0);
		char *reply = NULL;
		int replyStatus = 0;

		calculator->sendToCallback(calculator->sendToHandler, calculator->endpoint, data, &reply, &replyStatus);

		if (status == CELIX_SUCCESS) {
		    printf("Handle reply: %s\n", reply);
			json_error_t error;
			json_t *js_reply = json_loads(reply, 0, &error);
			if (js_reply) {
				json_unpack(js_reply, "{s:f}", "r", result);
				json_decref(js_reply);
			} else {
				printf("PROXY: got error '%s' for '%s'\n", error.text, reply);
				status = CELIX_BUNDLE_EXCEPTION;
			}
		}
		json_decref(root);

		free(data);
		free(reply);
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available\n");
		status = CELIX_BUNDLE_EXCEPTION;
	}

	return status;
}

celix_status_t calculatorProxy_sub(calculator_pt calculator, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;
	if (calculator->endpoint != NULL) {
		json_t *root;
		root = json_pack("{s:s, s:[ff]}", "m", "sub(DD)D", "a", a, b);

		char *data = json_dumps(root, 0);
		char *reply = NULL;
		int replyStatus = 0;

		calculator->sendToCallback(calculator->sendToHandler, calculator->endpoint, data, &reply, &replyStatus);

		if (status == CELIX_SUCCESS) {
			json_error_t error;
			json_t *js_reply = json_loads(reply, 0, &error);
			if (js_reply) {
			    json_unpack(js_reply, "{s:f}", "r", result);
			    json_decref(js_reply);
			} else {
				printf("PROXY: got error '%s' for '%s'\n", error.text, reply);
				status = CELIX_BUNDLE_EXCEPTION;
			}
		}

		json_decref(root);

		free(data);
		free(reply);
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available\n");
		status = CELIX_BUNDLE_EXCEPTION;
	}

	return status;
}

celix_status_t calculatorProxy_sqrt(calculator_pt calculator, double a, double *result) {
	celix_status_t status = CELIX_SUCCESS;
	if (calculator->endpoint != NULL) {
		json_t *root;
		root = json_pack("{s:s, s:[f]}", "m", "sqrt(D)D", "a", a);

		char *data = json_dumps(root, 0);
		char *reply = NULL;
		int replyStatus;

		calculator->sendToCallback(calculator->sendToHandler, calculator->endpoint, data, &reply, &replyStatus);

		if (status == CELIX_SUCCESS) {
			json_error_t error;
			json_t *js_reply = json_loads(reply, JSON_DECODE_ANY, &error);
			if (js_reply) {
			    json_unpack(js_reply, "{s:f}", "r", result);
			    json_decref(js_reply);
			} else {
				printf("PROXY: got error '%s' for '%s'\n", error.text, reply);
				status = CELIX_BUNDLE_EXCEPTION;
			}
		}

		json_decref(root);

		free(data);
		free(reply);
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available\n");
		status = CELIX_BUNDLE_EXCEPTION;
	}

	return status;
}
