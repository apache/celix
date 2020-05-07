/*
 * Licensed to the Apache Software Foundation (ASF) under one
 * or more contributor license agreements.  See the NOTICE file
 * distributed with this work for additional information
 * regarding copyright ownership.  The ASF licenses this file
 * to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance
 * with the License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing,
 * software distributed under the License is distributed on an
 * "AS IS" BASIS, WITHOUT WARRANTIES OR CONDITIONS OF ANY
 *  KIND, either express or implied.  See the License for the
 * specific language governing permissions and limitations
 * under the License.
 */
/**
 * log_sync.c
 *
 *  \date       Apr 19, 2012
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include "celix_errno.h"
#include "celix_log.h"
#include "celixbool.h"

#include "celix_threads.h"

#include "log_sync.h"
#include "log_event.h"

struct log_sync {
	log_store_pt logStore;

	char *targetId;
	bool running;

	celix_thread_t syncTask;
};

struct log_descriptor {
	char *targetId;
	unsigned long logId;
	unsigned long low;
	unsigned long high;
};

typedef struct log_descriptor *log_descriptor_pt;

celix_status_t logSync_queryLog(log_sync_pt logSync, char *targetId, long logId, char **queryReply);
static size_t logSync_readQuery(void *contents, size_t size, size_t nmemb, void *userp);
static void *logSync_synchronize(void *logSyncP);

celix_status_t logSync_create(char *targetId, log_store_pt store, log_sync_pt *logSync) {
	celix_status_t status = CELIX_SUCCESS;

	*logSync = calloc(1, sizeof(**logSync));
	if (!*logSync) {
		status = CELIX_ENOMEM;
	} else {
		(*logSync)->logStore = store;
		(*logSync)->targetId = targetId;
		(*logSync)->syncTask = celix_thread_default;
		(*logSync)->running = true;

		celixThread_create(&(*logSync)->syncTask, NULL, logSync_synchronize, *logSync);
	}

	return status;
}

celix_status_t logSync_parseLogDescriptor(log_sync_pt logSync, char *descriptorString, log_descriptor_pt *descriptor) {
	celix_status_t status = CELIX_SUCCESS;

	fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_DEBUG, "Descriptor: %s", descriptorString);
	char *last = NULL;
	char *targetId = strtok_r(descriptorString, ",", &last);
	char *logIdStr = strtok_r(NULL, ",", &last);
	long logId = 0;
	if (logIdStr != NULL) {
		logId = atol(logIdStr);
	}
	char *range = strtok_r(NULL, ",", &last);
	fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_DEBUG, "Range: %s", range);

	long low = 0;
	long high = 0;
	if (range != NULL) {
		char *rangeToken = NULL;
		low = atol(strtok_r(range, "-", &rangeToken));
		high = atol(strtok_r(NULL, "-", &rangeToken));
	}

	*descriptor = calloc(1, sizeof(**descriptor));
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

static void *logSync_synchronize(void *logSyncP) {
	log_sync_pt logSync = logSyncP;

	while (logSync->running) {

		//query current log
		// http://localhost:8080/auditlog/query?tid=targetid&logid=logid
		char *logDescriptorString = NULL;
		unsigned long id = 0;
		logStore_getLogId(logSync->logStore, &id);
		logSync_queryLog(logSync, logSync->targetId, id, &logDescriptorString);
		log_descriptor_pt descriptor = NULL;
		logSync_parseLogDescriptor(logSync, logDescriptorString, &descriptor);

		long highest = 0;
		logStore_getHighestId(logSync->logStore, &highest);

		if (highest >= 0) {
			int i;
			for (i = descriptor->high + 1; i <= highest; i++) {
				array_list_pt events = NULL;
				logStore_getEvents(logSync->logStore, &events);
			}
		}

		if(descriptor!=NULL){
			free(descriptor);
		}

		sleep(10);
	}


	celixThread_exit(NULL);
	return NULL;
}

struct MemoryStruct {
	char *memory;
	size_t size;
};

celix_status_t logSync_queryLog(log_sync_pt logSync, char *targetId, long logId, char **queryReply) {
	// http://localhost:8080/auditlog/query?tid=targetid&logid=logid
	celix_status_t status = CELIX_SUCCESS;
	int length = strlen(targetId) + 60;
	char query[length];
	snprintf(query, length, "http://localhost:8080/auditlog/query?tid=%s&logid=1", targetId);

	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	struct MemoryStruct chunk;
	chunk.memory = calloc(1, sizeof(char));
	chunk.size = 0;
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_URL, query);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, logSync_readQuery);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
		fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "Error: %d", res);
		/* always cleanup */
		curl_easy_cleanup(curl);

		*queryReply = strdup(chunk.memory);
	}

	return status;
}

static size_t logSync_readQuery(void *contents, size_t size, size_t nmemb, void *userp) {
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)userp;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
	if (mem->memory == NULL) {
		/* out of memory! */
		fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "not enough memory (realloc returned NULL)");
		exit(EXIT_FAILURE);
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}
