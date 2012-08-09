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
 * log_sync.c
 *
 *  \date       Apr 19, 2012
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <apr_general.h>
#include <apr_thread_proc.h>
#include <apr_strings.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include "celix_errno.h"
#include "celixbool.h"

#include "log_sync.h"
#include "log_event.h"

struct log_sync {
	apr_pool_t *pool;
	log_store_t logStore;

	char *targetId;
	bool running;

	apr_thread_t *syncTask;
};

struct log_descriptor {
	char *targetId;
	unsigned long logId;
	unsigned long low;
	unsigned long high;
};

typedef struct log_descriptor *log_descriptor_t;

celix_status_t logSync_queryLog(log_sync_t logSync, char *targetId, long logId, char **queryReply);
static size_t logSync_readQeury(void *contents, size_t size, size_t nmemb, void *userp);
static void *APR_THREAD_FUNC logSync_synchronize(apr_thread_t *thd, void *logSyncP);

celix_status_t logSync_create(apr_pool_t *pool, char *targetId, log_store_t store, log_sync_t *logSync) {
	celix_status_t status = CELIX_SUCCESS;

	*logSync = apr_palloc(pool, sizeof(**logSync));
	if (!*logSync) {
		status = CELIX_ENOMEM;
	} else {
		(*logSync)->pool = pool;
		(*logSync)->logStore = store;
		(*logSync)->targetId = targetId;
		(*logSync)->syncTask = NULL;
		(*logSync)->running = true;

		apr_thread_create(&(*logSync)->syncTask, NULL, logSync_synchronize, *logSync, pool);
	}

	return status;
}

celix_status_t logSync_parseLogDescriptor(log_sync_t logSync, char *descriptorString, log_descriptor_t *descriptor) {
	celix_status_t status = CELIX_SUCCESS;

	printf("Descriptor: %s\n", descriptorString);
	char *last = NULL;
	char *targetId = apr_strtok(descriptorString, ",", &last);
	char *logIdStr = apr_strtok(NULL, ",", &last);
	long logId = 0;
	if (logIdStr != NULL) {
		logId = atol(logIdStr);
	}
	char *range = apr_strtok(NULL, ",", &last);
	printf("Range: %s\n", range);

	long low = 0;
	long high = 0;
	if (range != NULL) {
		char *rangeToken = NULL;
		low = atol(apr_strtok(range, "-", &rangeToken));
		high = atol(apr_strtok(NULL, "-", &rangeToken));
	}

	*descriptor = apr_palloc(logSync->pool, sizeof(**descriptor));
	if (!*descriptor) {
		status = CELIX_ENOMEM;
	} else {
		(*descriptor)->targetId = targetId;
		(*descriptor)->logId = logId;
		(*descriptor)->low = low;
		(*descriptor)->high = high;
	}

	return status;
}

static void *APR_THREAD_FUNC logSync_synchronize(apr_thread_t *thd, void *logSyncP) {
	log_sync_t logSync = logSyncP;

		while (logSync->running) {

		//query current log
		// http://localhost:8080/auditlog/query?tid=targetid&logid=logid
		char *logDescriptorString = NULL;
		unsigned long id = 0;
		logStore_getLogId(logSync->logStore, &id);
		logSync_queryLog(logSync, logSync->targetId, id, &logDescriptorString);
		log_descriptor_t descriptor = NULL;
		logSync_parseLogDescriptor(logSync, logDescriptorString, &descriptor);

		long highest = 0;
		logStore_getHighestId(logSync->logStore, &highest);

//		printf("Highest local: %ld\n", highest);
//		printf("Highest remote: %ld\n", descriptor->high);

		if (highest >= 0) {
			int i;
			for (i = descriptor->high + 1; i <= highest; i++) {
				ARRAY_LIST events = NULL;
				logStore_getEvents(logSync->logStore, &events);
				log_event_t event = arrayList_get(events, i);
//				printf("Event id: %ld\n", event->id);


			}
		}
		sleep(10);
	}


	apr_thread_exit(thd, APR_SUCCESS);
	return NULL;
}

struct MemoryStruct {
	char *memory;
	size_t size;
};

celix_status_t logSync_queryLog(log_sync_t logSync, char *targetId, long logId, char **queryReply) {
	// http://localhost:8080/auditlog/query?tid=targetid&logid=logid
	celix_status_t status = CELIX_SUCCESS;

	char *query = apr_pstrcat(logSync->pool, "http://localhost:8080/auditlog/query?tid=", targetId, "&logid=1", NULL);

	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	struct MemoryStruct chunk;
	chunk.memory = calloc(1, sizeof(char));
	chunk.size = 0;
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, query);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, logSync_readQeury);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
		printf("Error: %d\n", res);
		/* always cleanup */
		curl_easy_cleanup(curl);

		*queryReply = apr_pstrdup(logSync->pool, chunk.memory);
	}

	return status;
}

static size_t logSync_readQeury(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
	/* out of memory! */
	printf("not enough memory (realloc returned NULL)\n");
	exit(EXIT_FAILURE);
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}
