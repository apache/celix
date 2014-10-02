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
#include "array_list.h"
#include "calculator_proxy_impl.h"

/* Allows the use of Jansson < 2.3 */
#ifndef JSON_DECODE_ANY
#define JSON_DECODE_ANY 0
#endif

celix_status_t calculatorProxy_setEndpointDescription(void *proxy, endpoint_description_pt endpoint);
celix_status_t calculatorProxy_setHandler(void *proxy, void *handler);
celix_status_t calculatorProxy_setCallback(void *proxy, sendToHandle callback);


celix_status_t calculatorProxy_create(apr_pool_t *pool, bundle_context_pt context, calculator_pt *calculator)  {
	celix_status_t status = CELIX_SUCCESS;
	*calculator = apr_palloc(pool, sizeof(**calculator));
	if (!*calculator) {
		status = CELIX_ENOMEM;
	} else {
		(*calculator)->pool = pool;
		(*calculator)->context = context;
		(*calculator)->endpoint = NULL;
		(*calculator)->sendToCallback=NULL;
		(*calculator)->sendToHandler=NULL;
	}

	return status;
}

// { "m": "" "a":["arg1", "arg2"] }
celix_status_t calculatorProxy_add(calculator_pt calculator, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;

	if (calculator->endpoint != NULL) {
		json_t *root;
		root = json_pack("{s:s, s:[ff]}", "m", "add(DD)D", "a", a, b);

		char *data = json_dumps(root, 0);
		char *reply = malloc(256);
		int replyStatus = 0;

		calculator->sendToCallback(calculator->sendToHandler, calculator->endpoint, data, &reply, &replyStatus);

		if (status == CELIX_SUCCESS) {
		    printf("Handle reply: %s\n", reply);
			json_error_t error;
			json_t *js_reply = json_loads(reply, 0, &error);
			if (js_reply) {
				json_unpack(js_reply, "{s:f}", "r", result);
			} else {
				printf("PROXY: got error '%s' for '%s'\n", error.text, reply);
				status = CELIX_BUNDLE_EXCEPTION;
			}
		}

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
		char *reply = malloc(128);
		int replyStatus = 0;

		calculator->sendToCallback(calculator->sendToHandler, calculator->endpoint, data, &reply, &replyStatus);

		if (status == CELIX_SUCCESS) {
			json_error_t error;
			json_t *js_reply = json_loads(reply, 0, &error);
			if (js_reply) {
			    json_unpack(js_reply, "{s:f}", "r", result);
			} else {
				printf("PROXY: got error '%s' for '%s'\n", error.text, reply);
				status = CELIX_BUNDLE_EXCEPTION;
			}
		}

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
		char *reply = malloc(128);
		int replyStatus;

		calculator->sendToCallback(calculator->sendToHandler, calculator->endpoint, data, &reply, &replyStatus);

		if (status == CELIX_SUCCESS) {
			json_error_t error;
			json_t *js_reply = json_loads(reply, JSON_DECODE_ANY, &error);
			if (js_reply) {
			    json_unpack(js_reply, "{s:f}", "r", result);
			} else {
				printf("PROXY: got error '%s' for '%s'\n", error.text, reply);
				status = CELIX_BUNDLE_EXCEPTION;
			}
		}

		free(data);
		free(reply);
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available\n");
		status = CELIX_BUNDLE_EXCEPTION;
	}

	return status;
}



celix_status_t calculatorProxy_registerProxyService(void* proxyFactoryService, endpoint_description_pt endpointDescription, void* rsa, sendToHandle sendToCallback) {
	celix_status_t status = CELIX_SUCCESS;

	remote_proxy_factory_service_pt calculatorProxyFactoryService = (remote_proxy_factory_service_pt) proxyFactoryService;
	calculator_pt calculator = NULL;
	calculator_service_pt calculatorService = NULL;

	calculatorProxy_create(calculatorProxyFactoryService->pool, calculatorProxyFactoryService->context, &calculator);
	calculatorService = apr_palloc(calculatorProxyFactoryService->pool, sizeof(*calculatorService));
	calculatorService->calculator = calculator;
	calculatorService->add = calculatorProxy_add;
	calculatorService->sub = calculatorProxy_sub;
	calculatorService->sqrt = calculatorProxy_sqrt;

	properties_pt srvcProps = properties_create();
	properties_set(srvcProps, (char *) "proxy.interface", (char *) CALCULATOR_SERVICE);
	properties_set(srvcProps, (char *) "endpoint.framework.uuid", (char *) endpointDescription->frameworkUUID);

	service_registration_pt proxyReg = NULL;

	calculatorProxy_setEndpointDescription(calculator, endpointDescription);
	calculatorProxy_setHandler(calculator, rsa);
	calculatorProxy_setCallback(calculator, sendToCallback);

	if (bundleContext_registerService(calculatorProxyFactoryService->context, CALCULATOR_SERVICE, calculatorService, srvcProps, &proxyReg) != CELIX_SUCCESS)
	{
		printf("CALCULATOR_PROXY: error while registering calculator service\n");
	}

	hashMap_put(calculatorProxyFactoryService->proxy_registrations, endpointDescription, proxyReg);


	return status;
}


celix_status_t calculatorProxy_unregisterProxyService(void* proxyFactoryService, endpoint_description_pt endpointDescription) {
	celix_status_t status = CELIX_SUCCESS;

	remote_proxy_factory_service_pt calculatorProxyFactoryService = (remote_proxy_factory_service_pt) proxyFactoryService;
	service_registration_pt proxyReg = hashMap_get(calculatorProxyFactoryService->proxy_registrations, endpointDescription);

	if (proxyReg != NULL)
	{
		serviceRegistration_unregister(proxyReg);
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

