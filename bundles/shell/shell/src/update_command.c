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


#include <stdlib.h>
#include <string.h>
#include <curl/curl.h>
#include <sys/stat.h>

#include "celix_utils.h"
#include "celix_array_list.h"
#include "celix_bundle_context.h"

celix_status_t updateCommand_download(bundle_context_pt context, char * url, char **inputFile);
size_t updateCommand_writeData(void *ptr, size_t size, size_t nmemb, FILE *stream);

bool updateCommand_execute(void *handle, const char *const_line, FILE *outStream, FILE *errStream) {
	bundle_context_pt context = handle;
    bundle_pt bundle = NULL;
	char delims[] = " ";
	char * sub = NULL;
	char *save_ptr = NULL;
	char *line = celix_utils_strdup(const_line);

	sub = strtok_r(line, delims, &save_ptr);
	sub = strtok_r(NULL, delims, &save_ptr);

	bool updateSucceeded = false;
	if (sub == NULL) {
		fprintf(errStream, "Incorrect number of arguments.\n");
	} else {
		long id = atol(sub);
		celix_status_t ret = bundleContext_getBundleById(context, id, &bundle);
		if (ret==CELIX_SUCCESS && bundle!=NULL) {
			char inputFile[256];
			sub = strtok_r(NULL, delims, &save_ptr);
			inputFile[0] = '\0';
			if (sub != NULL) {
				char *test = inputFile;
				printf("URL: %s\n", sub);

				if (updateCommand_download(context, sub, &test) == CELIX_SUCCESS) {
					printf("Update bundle with stream\n");
					celix_status_t status = bundle_update(bundle, inputFile);
                    updateSucceeded = status == CELIX_SUCCESS;
				} else {
					fprintf(errStream, "Unable to download from %s\n", sub);
				}
			} else {
				bundle_update(bundle, NULL);
			}
		} else {
			fprintf(errStream, "Bundle id is invalid.\n");
		}
	}
	free(line);
	return updateSucceeded;
}

celix_status_t updateCommand_download(bundle_context_pt context, char * url, char **inputFile) {
	CURL *curl = NULL;
	CURLcode res = CURLE_FILE_COULDNT_READ_FILE;
	curl = curl_easy_init();
	if (curl) {
		FILE *fp = NULL;
		snprintf(*inputFile, 13,"updateXXXXXX");
		umask(0011);
		int fd = mkstemp(*inputFile);
		if (fd) {
		    fp = fopen(*inputFile, "wb+");
		    if(fp!=NULL){
		    	printf("Temp file: %s\n", *inputFile);
		    	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		    	curl_easy_setopt(curl, CURLOPT_URL, url);
		    	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, updateCommand_writeData);
		    	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		    	//curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
		    	//curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, updateCommand_downloadProgress);
		    	res = curl_easy_perform(curl);
		    	fclose(fp);
		    }
		    /* always cleanup */
		    curl_easy_cleanup(curl);
		    if(fp==NULL){
		    	return CELIX_FILE_IO_EXCEPTION;
		    }
		}
	}
	if (res != CURLE_OK) {
		printf("Error: %d\n", res);
		*inputFile[0] = '\0';
		return CELIX_ILLEGAL_STATE;
	} else {
		return CELIX_SUCCESS;
	}
}

size_t updateCommand_writeData(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}
