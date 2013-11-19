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

#include <curl/curl.h>

#include <string.h>
#include <apr_strings.h>

#include <stddef.h>

#include "celix_errno.h"

#include "calculator_proxy_impl.h"

struct post {
	const char *readptr;
	int size;
};

struct get {
	char *writeptr;
	int size;
};

celix_status_t calculatorProxy_postRequest(calculator_pt calculator, char *url, struct post post, struct get *get);
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);
static size_t calculatorProxy_write(void *contents, size_t size, size_t nmemb, void *userp);

celix_status_t calculatorProxy_create(apr_pool_t *pool, calculator_pt *endpoint) {
	celix_status_t status = CELIX_SUCCESS;
	*endpoint = apr_palloc(pool, sizeof(**endpoint));
	if (!*endpoint) {
		status = CELIX_ENOMEM;
	} else {
		(*endpoint)->pool = pool;
		(*endpoint)->endpoint = NULL;
	}

	return status;
}

celix_status_t calculatorProxy_add(calculator_pt calculator, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;

	if (calculator->endpoint != NULL) {
		char *serviceUrl = properties_get(calculator->endpoint->properties, ".ars.alias");
		printf("CALCULATOR_PROXY: URL: %s\n", serviceUrl);
		char *url = apr_pstrcat(calculator->pool, serviceUrl, "/add", NULL);

		json_t *root;
		root = json_pack("{s:f, s:f}", "arg0", a, "arg1", b);

		struct post post;
		char *data = json_dumps(root, 0);
		post.readptr = data;
		post.size = strlen(data);

		struct get get;
		get.size = 0;
		get.writeptr = malloc(1);

		status = calculatorProxy_postRequest(calculator, url, post, &get);
		if (status == CELIX_SUCCESS) {
			json_error_t jsonError;
			json_t *reply = json_loads(get.writeptr, 0, &jsonError);
			json_unpack(reply, "[f]", result);
		}
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available\n");
	}

	return status;
}

celix_status_t calculatorProxy_sub(calculator_pt calculator, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;
	if (calculator->endpoint != NULL) {
	    char *serviceUrl = properties_get(calculator->endpoint->properties, ".ars.alias");
		printf("CALCULATOR_PROXY: URL: %s\n", serviceUrl);
		char *url = apr_pstrcat(calculator->pool, serviceUrl, "/sub", NULL);

		json_t *root;
		root = json_pack("{s:f, s:f}", "arg0", a, "arg1", b);

		struct post post;
		char *data = json_dumps(root, 0);
		post.readptr = data;
		post.size = strlen(data);

		struct get get;
		get.size = 0;
		get.writeptr = malloc(1);

		status = calculatorProxy_postRequest(calculator, url, post, &get);
		if (status == CELIX_SUCCESS) {
			json_error_t jsonError;
			json_t *reply = json_loads(get.writeptr, 0, &jsonError);
			json_unpack(reply, "[f]", result);
		}
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available\n");
	}

	return status;
}

celix_status_t calculatorProxy_sqrt(calculator_pt calculator, double a, double *result) {
	celix_status_t status = CELIX_SUCCESS;
	if (calculator->endpoint != NULL) {
	    char *serviceUrl = properties_get(calculator->endpoint->properties, ".ars.alias");
		printf("CALCULATOR_PROXY: URL: %s\n", serviceUrl);
		char *url = apr_pstrcat(calculator->pool, serviceUrl, "/sqrt", NULL);

		json_t *root;
		root = json_pack("{s:f}", "arg0", a);

		struct post post;
		char *data = json_dumps(root, 0);
		post.readptr = data;
		post.size = strlen(data);

		struct get get;
		get.size = 0;
		get.writeptr = malloc(1);

		status = calculatorProxy_postRequest(calculator, url, post, &get);
		if (status == CELIX_SUCCESS) {
			json_error_t jsonError;
			json_t *reply = json_loads(get.writeptr, 0, &jsonError);
			json_unpack(reply, "[f]", result);
		}
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available\n");
	}

	return status;
}

celix_status_t calculatorProxy_postRequest(calculator_pt calculator, char *url, struct post post, struct get *get) {
	celix_status_t status = CELIX_SUCCESS;
	CURL *curl;
	CURLcode res;

	curl = curl_easy_init();
	if(!curl) {
		status = CELIX_ILLEGAL_STATE;
	} else {
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_POST, 1L);
		curl_easy_setopt(curl, CURLOPT_READFUNCTION, read_callback);
		curl_easy_setopt(curl, CURLOPT_READDATA, &post);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, calculatorProxy_write);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)get);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (curl_off_t)post.size);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		printf("CALCULATOR_PROXY: Data read: \"%s\" %d\n", get->writeptr, res);

	}
	return status;
}

static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp) {
	struct post *post = userp;

	if (post->size) {
		*(char *) ptr = post->readptr[0];
		post->readptr++;
		post->size--;
		return 1;
	}

	return 0;
}

static size_t calculatorProxy_write(void *contents, size_t size, size_t nmemb, void *userp) {
  size_t realsize = size * nmemb;
  struct get *mem = (struct get *)userp;

  mem->writeptr = realloc(mem->writeptr, mem->size + realsize + 1);
  if (mem->writeptr == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }

  memcpy(&(mem->writeptr[mem->size]), contents, realsize);
  mem->size += realsize;
  mem->writeptr[mem->size] = 0;

  return realsize;
}

celix_status_t calculatorProxy_setEndpointDescription(void *proxy, endpoint_description_pt endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	calculator_pt calculator = proxy;
	calculator->endpoint = endpoint;

	return status;
}
