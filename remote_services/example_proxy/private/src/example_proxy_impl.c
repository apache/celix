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

#include <curl/curl.h>

#include <string.h>
#include <apr_strings.h>

#include <stddef.h>

#include "celix_errno.h"

#include "example_proxy_impl.h"

struct post {
	const char *readptr;
	int size;
};

struct get {
	char *writeptr;
	int size;
};

celix_status_t exampleProxy_postRequest(example_t example, char *url, struct post post, struct get *get);
static size_t read_callback(void *ptr, size_t size, size_t nmemb, void *userp);
static size_t exampleProxy_write(void *contents, size_t size, size_t nmemb, void *userp);

celix_status_t exampleProxy_create(apr_pool_t *pool, example_t *endpoint) {
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

celix_status_t exampleProxy_add(example_t example, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;

	if (example->endpoint != NULL) {
		printf("CALCULATOR_PROXY: URL: %s\n", example->endpoint->id);
		char *url = apr_pstrcat(example->pool, example->endpoint->id, "/add", NULL);

		json_t *root;
		root = json_pack("{s:f, s:f}", "a", a, "b", b);

		struct post post;
		char *data = json_dumps(root, 0);
		post.readptr = data;
		post.size = strlen(data);

		struct get get;
		get.size = 0;
		get.writeptr = malloc(1);

		status = exampleProxy_postRequest(example, url, post, &get);
		if (status == CELIX_SUCCESS) {
			json_error_t jsonError;
			json_t *reply = json_loads(get.writeptr, 0, &jsonError);
			json_unpack(reply, "{s:f}", "result", result);
		}
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available");
	}

	return status;
}

celix_status_t exampleProxy_sub(example_t example, double a, double b, double *result) {
	celix_status_t status = CELIX_SUCCESS;
	if (example->endpoint != NULL) {
		printf("CALCULATOR_PROXY: URL: %s\n", example->endpoint->id);
		char *url = apr_pstrcat(example->pool, example->endpoint->id, "/sub", NULL);

		json_t *root;
		root = json_pack("{s:f, s:f}", "a", a, "b", b);

		struct post post;
		char *data = json_dumps(root, 0);
		post.readptr = data;
		post.size = strlen(data);

		struct get get;
		get.size = 0;
		get.writeptr = malloc(1);

		status = exampleProxy_postRequest(example, url, post, &get);
		if (status == CELIX_SUCCESS) {
			json_error_t jsonError;
			json_t *reply = json_loads(get.writeptr, 0, &jsonError);
			json_unpack(reply, "{s:f}", "result", result);
		}
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available");
	}

	return status;
}

celix_status_t exampleProxy_sqrt(example_t example, double a, double *result) {
	celix_status_t status = CELIX_SUCCESS;
	if (example->endpoint != NULL) {
		printf("CALCULATOR_PROXY: URL: %s\n", example->endpoint->id);
		char *url = apr_pstrcat(example->pool, example->endpoint->id, "/sqrt", NULL);

		json_t *root;
		root = json_pack("{s:f}", "a", a);

		struct post post;
		char *data = json_dumps(root, 0);
		post.readptr = data;
		post.size = strlen(data);

		struct get get;
		get.size = 0;
		get.writeptr = malloc(1);

		status = exampleProxy_postRequest(example, url, post, &get);
		if (status == CELIX_SUCCESS) {
			json_error_t jsonError;
			json_t *reply = json_loads(get.writeptr, 0, &jsonError);
			json_unpack(reply, "{s:f}", "result", result);
		}
	} else {
		printf("CALCULATOR_PROXY: No endpoint information available");
	}

	return status;
}

celix_status_t exampleProxy_postRequest(example_t example, char *url, struct post post, struct get *get) {
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
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, exampleProxy_write);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, (void *)get);
		curl_easy_setopt(curl, CURLOPT_POSTFIELDSIZE, (curl_off_t)post.size);
		res = curl_easy_perform(curl);
		curl_easy_cleanup(curl);

		printf("CALCULATOR_PROXY: Data read: \"%s\"\n", get->writeptr);

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

static size_t exampleProxy_write(void *contents, size_t size, size_t nmemb, void *userp) {
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

celix_status_t exampleProxy_setEndpointDescription(void *proxy, endpoint_description_t endpoint) {
	celix_status_t status = CELIX_SUCCESS;

	example_t example = proxy;
	example->endpoint = endpoint;

	return status;
}
