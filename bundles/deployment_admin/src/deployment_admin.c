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
 * deployment_admin.c
 *
 *  \date       Nov 7, 2011
 *  \author    	<a href="mailto:dev@celix.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */

#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <stdint.h>

#include <dirent.h>
#include <sys/types.h>
#include <sys/stat.h>

#include <curl/curl.h>
#include <curl/easy.h>

#include <uuid/uuid.h>

#include "celixbool.h"
#include "deployment_admin.h"
#include "celix_errno.h"
#include "bundle_context.h"
#include "celix_constants.h"
#include "deployment_package.h"
#include "bundle.h"
#include "utils.h"

#include "log.h"
#include "log_store.h"
#include "log_sync.h"

#include "resource_processor.h"
#include "miniunz.h"

#define IDENTIFICATION_ID "deployment_admin_identification"
#define DEFAULT_IDENTIFICATION_ID "celix"

#define ADMIN_URL "deployment_admin_url"
#define DEFAULT_ADMIN_URL "localhost:8080"

#define DEPLOYMENT_CACHE_DIR "deployment_cache_dir"
#define DEPLOYMENT_TAGS "deployment_tags"
// "http://localhost:8080/deployment/"

#define VERSIONS "/versions"

static void* deploymentAdmin_poll(void *deploymentAdmin);
celix_status_t deploymentAdmin_download(deployment_admin_pt admin, char * url, char **inputFile);
size_t deploymentAdmin_writeData(void *ptr, size_t size, size_t nmemb, FILE *stream);
static celix_status_t deploymentAdmin_deleteTree(char * directory);
celix_status_t deploymentAdmin_readVersions(deployment_admin_pt admin, array_list_pt versions);

celix_status_t deploymentAdmin_stopDeploymentPackageBundles(deployment_admin_pt admin, deployment_package_pt target);
celix_status_t deploymentAdmin_updateDeploymentPackageBundles(deployment_admin_pt admin, deployment_package_pt source);
celix_status_t deploymentAdmin_startDeploymentPackageCustomizerBundles(deployment_admin_pt admin, deployment_package_pt source, deployment_package_pt target);
celix_status_t deploymentAdmin_processDeploymentPackageResources(deployment_admin_pt admin, deployment_package_pt source);
celix_status_t deploymentAdmin_dropDeploymentPackageResources(deployment_admin_pt admin, deployment_package_pt source, deployment_package_pt target);
celix_status_t deploymentAdmin_dropDeploymentPackageBundles(deployment_admin_pt admin, deployment_package_pt source, deployment_package_pt target);
celix_status_t deploymentAdmin_startDeploymentPackageBundles(deployment_admin_pt admin, deployment_package_pt source);

static celix_status_t deploymentAdmin_performRequest(deployment_admin_pt admin, char* entry);
static celix_status_t deploymentAdmin_auditEventTargetPropertiesSet(deployment_admin_pt admin);
static celix_status_t deploymentAdmin_auditEventFrameworkStarted(deployment_admin_pt admin);

celix_status_t deploymentAdmin_create(bundle_context_pt context, deployment_admin_pt *admin) {
	celix_status_t status = CELIX_SUCCESS;

	*admin = calloc(1, sizeof(**admin));
	if (!*admin) {
		status = CELIX_ENOMEM;
	} else {
		(*admin)->running = true;
		(*admin)->context = context;
		(*admin)->current = NULL;
		(*admin)->packages = hashMap_create(utils_stringHash, NULL, utils_stringEquals, NULL);
		(*admin)->targetIdentification = NULL;
		(*admin)->pollUrl = NULL;
		(*admin)->auditlogUrl = NULL;

        bundleContext_getProperty(context, IDENTIFICATION_ID, (const char**) &(*admin)->targetIdentification);
        if ((*admin)->targetIdentification == NULL) {
        	(*admin)->targetIdentification = DEFAULT_IDENTIFICATION_ID;
        	fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_INFO, "Identification ID not set, using default '%s'. Set id by using '%s'",
        		DEFAULT_IDENTIFICATION_ID, IDENTIFICATION_ID);
        }

        struct timeval tv;
		gettimeofday(&tv,NULL);
		(*admin)->auditlogId =  tv.tv_sec*(uint64_t)1000000+tv.tv_usec;
        (*admin)->auditlogSeqNr = 0;

		if ((*admin)->targetIdentification == NULL ) {
		    fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "Target name must be set using \"deployment_admin_identification\"");
		} else {
			const char *url = NULL;
			bundleContext_getProperty(context, ADMIN_URL, &url);
			if (url == NULL) {
				url = DEFAULT_ADMIN_URL;
			    fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_INFO, "Server URL is not set, using default '%s'. Set id by using '%s'",
        			DEFAULT_ADMIN_URL, ADMIN_URL);
			}
		
			int pollUrlLength = strlen(url) + strlen((*admin)->targetIdentification) + strlen(VERSIONS) + 13;
			int auditlogUrlLength = strlen(url) + 10;

			char pollUrl[pollUrlLength];
			char auditlogUrl[auditlogUrlLength];

			snprintf(pollUrl, pollUrlLength, "%s/deployment/%s%s", url, (*admin)->targetIdentification, VERSIONS);
			snprintf(auditlogUrl, auditlogUrlLength, "%s/auditlog", url);

			(*admin)->pollUrl = strdup(pollUrl);
			(*admin)->auditlogUrl = strdup(auditlogUrl);

//				log_store_pt store = NULL;
//				log_t *log = NULL;
//				log_sync_pt sync = NULL;
//				logStore_create(subpool, &store);
//				log_create(subpool, store, &log);
//				logSync_create(subpool, (*admin)->targetIdentification, store, &sync);
//
//				log_log(log, 20000, NULL);


			celixThread_create(&(*admin)->poller, NULL, deploymentAdmin_poll, *admin);
		}
	}

	return status;
}



celix_status_t deploymentAdmin_destroy(deployment_admin_pt admin) {
	celix_status_t status = CELIX_SUCCESS;

    admin->running = false;

    celixThread_join(admin->poller, NULL);

	hash_map_iterator_pt iter = hashMapIterator_create(admin->packages);

	while (hashMapIterator_hasNext(iter)) {
		deployment_package_pt target = (deployment_package_pt) hashMapIterator_nextValue(iter);
		deploymentPackage_destroy(target);
	}

	hashMapIterator_destroy(iter);

	hashMap_destroy(admin->packages, false, false);

	if (admin->current != NULL) {
		free(admin->current);
	}

	free(admin->pollUrl);
	free(admin->auditlogUrl);

	free(admin);

	return status;
}


static celix_status_t deploymentAdmin_performRequest(deployment_admin_pt admin, char* entry) {
    celix_status_t status = CELIX_SUCCESS;

    CURL *curl;
    CURLcode res;
    curl = curl_easy_init();

    if (!curl) {
        status = CELIX_BUNDLE_EXCEPTION;

        fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "Error initializing curl.");
    }

    char url[strlen(admin->auditlogUrl)+6];
    sprintf(url, "%s/send", admin->auditlogUrl);

    if (status == CELIX_SUCCESS) {
            curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
            curl_easy_setopt(curl, CURLOPT_URL, url);
            curl_easy_setopt(curl, CURLOPT_POSTFIELDS, entry);
            res = curl_easy_perform(curl);

            if (res != CURLE_OK ) {
                status = CELIX_BUNDLE_EXCEPTION;
                fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "Error sending auditlog, got curl error code %d", res);
            }
    }

    return status;
}

static celix_status_t deploymentAdmin_auditEventTargetPropertiesSet(deployment_admin_pt admin) {
    celix_status_t status = CELIX_SUCCESS;

    const char *tags = NULL;

    bundleContext_getProperty(admin->context, DEPLOYMENT_TAGS, &tags);

    if (tags != NULL) {
        char entry[512];
        int entrySize = 0;

        entrySize = snprintf(entry, 512, "%s,%llu,%u,0,%i,%s\n", admin->targetIdentification, admin->auditlogId, admin->auditlogSeqNr++, DEPLOYMENT_ADMIN_AUDIT_EVENT__TARGETPROPERTIES_SET, tags);

        if (entrySize >= 512) {
            status = CELIX_BUNDLE_EXCEPTION;
        }
        else {
            status = deploymentAdmin_performRequest(admin, entry);
        }
    }

    return status;
}

static celix_status_t deploymentAdmin_auditEventFrameworkStarted(deployment_admin_pt admin) {
    celix_status_t status = CELIX_SUCCESS;

    char entry[512];
    int entrySize = 0;

    entrySize = snprintf(entry, 512, "%s,%llu,%u,0,%i\n", admin->targetIdentification, admin->auditlogId, admin->auditlogSeqNr++, DEPLOYMENT_ADMIN_AUDIT_EVENT__FRAMEWORK_STARTED);

    if (entrySize >= 512) {
        status = CELIX_BUNDLE_EXCEPTION;
    }
    else {
        status = deploymentAdmin_performRequest(admin, entry);
    }

    return status;
}


static void *deploymentAdmin_poll(void *deploymentAdmin) {
	deployment_admin_pt admin = deploymentAdmin;

	/*first poll send framework started audit event, note this will register the target in Apache ACE*/
    deploymentAdmin_auditEventFrameworkStarted(admin);
    deploymentAdmin_auditEventTargetPropertiesSet(admin);

	while (admin->running) {
        int i;

		//poll ace
		array_list_pt versions = NULL;
	    arrayList_create(&versions);

		deploymentAdmin_readVersions(admin, versions);

		char *last = arrayList_get(versions, arrayList_size(versions) - 1);

		if (last != NULL) {
			if (admin->current == NULL || strcmp(last, admin->current) != 0) {
				int length = strlen(admin->pollUrl) + strlen(last) + 2;
				char request[length];

				// TODO
                //      We do not yet support fix packages
                //		Check string length!
                // snprintf(request, length, "%s/%s?current=%s", admin->pollUrl, last, admin->current);
                snprintf(request, length, "%s/%s", admin->pollUrl, last);

				char *inputFilename = NULL;
				celix_status_t status = deploymentAdmin_download(admin ,request, &inputFilename);
				if (status == CELIX_SUCCESS) {
					bundle_pt bundle = NULL;
					bundleContext_getBundle(admin->context, &bundle);
					char *entry = NULL;
					bundle_getEntry(bundle, "/", &entry);

					// Handle file
					char tmpDir[256];
					char uuid[37];
					uuid_t uid;
					uuid_generate(uid);
					uuid_unparse(uid, uuid);
                    snprintf(tmpDir, 256, "%s%s", entry, uuid);
                    if( mkdir(tmpDir, S_IRWXU) == -1){
                        fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "Failed creating directory %s",tmpDir);
                    }

					// TODO: update to use bundle cache DataFile instead of module entries.
					unzip_extractDeploymentPackage(inputFilename, tmpDir);
					int length = strlen(tmpDir) + 22;
					char manifest[length];
					snprintf(manifest, length, "%s/META-INF/MANIFEST.MF", tmpDir);
					manifest_pt mf = NULL;
					manifest_createFromFile(manifest, &mf);
					deployment_package_pt source = NULL;
					deploymentPackage_create(admin->context, mf, &source);
					const char *name = NULL;
					deploymentPackage_getName(source, &name);

					int repoDirLength = strlen(entry) + 5;
					char repoDir[repoDirLength];
					snprintf(repoDir, repoDirLength, "%srepo", entry);
					if( mkdir(repoDir, S_IRWXU) == -1){
						fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "Failed creating directory %s",repoDir);
					}

					int repoCacheLength = strlen(entry) + strlen(name) + 6;
					char repoCache[repoCacheLength];
					snprintf(repoCache, repoCacheLength, "%srepo/%s", entry, name);
					deploymentAdmin_deleteTree(repoCache);
					int stat = rename(tmpDir, repoCache);
					if (stat != 0) {
						fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "No success");
					}

					deployment_package_pt target = hashMap_get(admin->packages, name);
					if (target == NULL) {
//						target = empty package
					}

					deploymentAdmin_stopDeploymentPackageBundles(admin, target);
					deploymentAdmin_updateDeploymentPackageBundles(admin, source);
					deploymentAdmin_startDeploymentPackageCustomizerBundles(admin, source, target);
					deploymentAdmin_processDeploymentPackageResources(admin, source);
					deploymentAdmin_dropDeploymentPackageResources(admin, source, target);
					deploymentAdmin_dropDeploymentPackageBundles(admin, source, target);
					deploymentAdmin_startDeploymentPackageBundles(admin, source);

					deploymentAdmin_deleteTree(repoCache);
					deploymentAdmin_deleteTree(tmpDir);
					if( remove(inputFilename) == -1){
						fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "Remove of %s failed",inputFilename);
					}
					admin->current = strdup(last);
					hashMap_put(admin->packages, (char*)name, source);

                    free(entry);
				}
				if (inputFilename != NULL) {
					free(inputFilename);
				}
			}
		}

		sleep(5);

		for (i = arrayList_size(versions); i > 0; --i) {
		    free(arrayList_remove(versions, 0));
		}

		arrayList_destroy(versions);
	}

	return NULL;
}

struct MemoryStruct {
	char *memory;
	size_t size;
};

size_t deploymentAdmin_parseVersions(void *contents, size_t size, size_t nmemb, void *userp) {
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

celix_status_t deploymentAdmin_readVersions(deployment_admin_pt admin, array_list_pt versions) {
	celix_status_t status = CELIX_SUCCESS;

	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	struct MemoryStruct chunk;
	chunk.memory = calloc(1, sizeof(char));
	chunk.size = 0;
	if (curl) {
	    curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
		curl_easy_setopt(curl, CURLOPT_URL, admin->pollUrl);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, deploymentAdmin_parseVersions);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
		/* always cleanup */
		curl_easy_cleanup(curl);

		char *last;
		char *token = strtok_r(chunk.memory, "\n", &last);
		while (token != NULL) {
			arrayList_add(versions, strdup(token));
			token = strtok_r(NULL, "\n", &last);
		}
	}

    if (chunk.memory) {
        free(chunk.memory);
    }

	return status;
}


celix_status_t deploymentAdmin_download(deployment_admin_pt admin, char * url, char **inputFile) {
	celix_status_t status = CELIX_SUCCESS;
	CURL *curl = NULL;
	CURLcode res = 0;
	curl = curl_easy_init();
	if (curl) {
		const char *dir = NULL;
		bundleContext_getProperty(admin->context, DEPLOYMENT_CACHE_DIR, &dir);
		if (dir != NULL) {
			*inputFile = calloc(1024, sizeof (char));
			snprintf(*inputFile, 1024, "%s/%s", dir, "updateXXXXXX");
		}
		else {
			*inputFile = strdup("updateXXXXXX");
		}
		umask(0011);
        int fd = mkstemp(*inputFile);
        if (fd != -1) {
            FILE *fp = fopen(*inputFile, "wb+");
            if(fp!=NULL){
            	curl_easy_setopt(curl, CURLOPT_NOSIGNAL, 1);
            	curl_easy_setopt(curl, CURLOPT_URL, url);
            	curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, deploymentAdmin_writeData);
            	curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
            	curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
            	//curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
            	//curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, updateCommand_downloadProgress);
            	res = curl_easy_perform(curl);

            	/* always cleanup */
            	curl_easy_cleanup(curl);
            	fclose(fp);
            }
            else{
            	status = CELIX_FILE_IO_EXCEPTION;
            }
        }
        else{
        	status = CELIX_FILE_IO_EXCEPTION;
        }
	}
	else{
		res = CURLE_FAILED_INIT;
	}

	if (res != CURLE_OK) {
		*inputFile[0] = '\0';
		status = CELIX_ILLEGAL_STATE;
	}

	return status;
}

size_t deploymentAdmin_writeData(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}


static celix_status_t deploymentAdmin_deleteTree(char * directory) {
	DIR *dir;
	celix_status_t status = CELIX_SUCCESS;
	dir = opendir(directory);
	if (dir == NULL) {
	    status = CELIX_FILE_IO_EXCEPTION;
	} else {

		struct dirent* dent = NULL;

		errno = 0;
		dent = readdir(dir);
		while (errno == 0 && dent != NULL) {
			if ((strcmp((dent->d_name), ".") != 0) && (strcmp((dent->d_name), "..") != 0)) {
				char subdir[512];
				snprintf(subdir, sizeof(subdir), "%s/%s", directory, dent->d_name);

				if (dent->d_type == DT_DIR) {
					status = deploymentAdmin_deleteTree(subdir);
				} else {
					if (remove(subdir) != 0) {
						status = CELIX_FILE_IO_EXCEPTION;
						break;
					}
				}
			}

			errno = 0;
			dent = readdir(dir);
		}

		if (errno != 0) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else if (closedir(dir) != 0) {
			status = CELIX_FILE_IO_EXCEPTION;
		} else if (rmdir(directory) != 0) {
				status = CELIX_FILE_IO_EXCEPTION;
		}
	}

	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Failed to delete tree");

	return status;
}

celix_status_t deploymentAdmin_stopDeploymentPackageBundles(deployment_admin_pt admin, deployment_package_pt target) {
	celix_status_t status = CELIX_SUCCESS;

	if (target != NULL) {
		array_list_pt infos = NULL;
		deploymentPackage_getBundleInfos(target, &infos);
		int i;
		for (i = 0; i < arrayList_size(infos); i++) {
			bundle_pt bundle = NULL;
			bundle_info_pt info = arrayList_get(infos, i);
			deploymentPackage_getBundle(target, info->symbolicName, &bundle);
			if (bundle != NULL) {
				bundle_stop(bundle);
			} else {
				fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "DEPLOYMENT_ADMIN: Bundle %s not found", info->symbolicName);
			}
		}
		arrayList_destroy(infos);
	}

	return status;
}

celix_status_t deploymentAdmin_updateDeploymentPackageBundles(deployment_admin_pt admin, deployment_package_pt source) {
	celix_status_t status = CELIX_SUCCESS;

	array_list_pt infos = NULL;
	deploymentPackage_getBundleInfos(source, &infos);
	int i;
	for (i = 0; i < arrayList_size(infos); i++) {
		bundle_pt bundle = NULL;
		bundle_info_pt info = arrayList_get(infos, i);

		bundleContext_getBundle(admin->context, &bundle);
		char *entry = NULL;
		bundle_getEntry(bundle, "/", &entry);
		const char *name = NULL;
		deploymentPackage_getName(source, &name);

		int bundlePathLength = strlen(entry) + strlen(name) + strlen(info->path) + 7;
		int bsnLength = strlen(info->symbolicName) + 9;

		char bundlePath[bundlePathLength];
		snprintf(bundlePath, bundlePathLength, "%srepo/%s/%s", entry, name, info->path);

		char bsn[bsnLength];
		snprintf(bsn, bsnLength, "osgi-dp:%s", info->symbolicName);

		bundle_pt updateBundle = NULL;
		deploymentPackage_getBundle(source, info->symbolicName, &updateBundle);
		if (updateBundle != NULL) {
			//printf("Update bundle from: %s\n", bundlePath);
			bundle_update(updateBundle, bundlePath);
		} else {
			//printf("Install bundle from: %s\n", bundlePath);
			bundleContext_installBundle2(admin->context, bsn, bundlePath, &updateBundle);
		}

        free(entry);
	}
	arrayList_destroy(infos);
	return status;
}

celix_status_t deploymentAdmin_startDeploymentPackageCustomizerBundles(deployment_admin_pt admin, deployment_package_pt source, deployment_package_pt target) {
	celix_status_t status = CELIX_SUCCESS;

	array_list_pt bundles = NULL;
	array_list_pt sourceInfos = NULL;

	arrayList_create(&bundles);

	deploymentPackage_getBundleInfos(source, &sourceInfos);
	int i;
	for (i = 0; i < arrayList_size(sourceInfos); i++) {
		bundle_info_pt sourceInfo = arrayList_get(sourceInfos, i);
		if (sourceInfo->customizer) {
			bundle_pt bundle = NULL;
			deploymentPackage_getBundle(source, sourceInfo->symbolicName, &bundle);
			if (bundle != NULL) {
				arrayList_add(bundles, bundle);
			}
		}
	}
	arrayList_destroy(sourceInfos);

	if (target != NULL) {
		array_list_pt targetInfos = NULL;
		deploymentPackage_getBundleInfos(target, &targetInfos);
		for (i = 0; i < arrayList_size(targetInfos); i++) {
			bundle_info_pt targetInfo = arrayList_get(targetInfos, i);
			if (targetInfo->customizer) {
				bundle_pt bundle = NULL;
				deploymentPackage_getBundle(target, targetInfo->symbolicName, &bundle);
				if (bundle != NULL) {
					arrayList_add(bundles, bundle);
				}
			}
		}
		arrayList_destroy(targetInfos);
	}

	for (i = 0; i < arrayList_size(bundles); i++) {
		bundle_pt bundle = arrayList_get(bundles, i);
		bundle_start(bundle);
	}

    arrayList_destroy(bundles);

	return status;
}

celix_status_t deploymentAdmin_processDeploymentPackageResources(deployment_admin_pt admin, deployment_package_pt source) {
	celix_status_t status = CELIX_SUCCESS;

	array_list_pt infos = NULL;
	deploymentPackage_getResourceInfos(source, &infos);
	int i;
	for (i = 0; i < arrayList_size(infos); i++) {
		resource_info_pt info = arrayList_get(infos, i);
		array_list_pt services = NULL;
		int length = strlen(OSGI_FRAMEWORK_SERVICE_PID) + strlen(info->resourceProcessor) + 4;
		char filter[length];

		snprintf(filter, length, "(%s=%s)", OSGI_FRAMEWORK_SERVICE_PID, info->resourceProcessor);

		status = bundleContext_getServiceReferences(admin->context, DEPLOYMENTADMIN_RESOURCE_PROCESSOR_SERVICE, filter, &services);
		if (status == CELIX_SUCCESS) {
			if (services != NULL && arrayList_size(services) > 0) {
				service_reference_pt ref = arrayList_get(services, 0);
				// In Felix a check is done to assure the processor belongs to the deployment package
				// Is this according to spec?
				void *processorP = NULL;
				status = bundleContext_getService(admin->context, ref, &processorP);
				if (status == CELIX_SUCCESS) {
					bundle_pt bundle = NULL;
					char *entry = NULL;
					const char *name = NULL;
					const char *packageName = NULL;
					resource_processor_service_pt processor = processorP;

					bundleContext_getBundle(admin->context, &bundle);
					bundle_getEntry(bundle, "/", &entry);
					deploymentPackage_getName(source, &name);

					int length = strlen(entry) + strlen(name) + strlen(info->path) + 7;
					char resourcePath[length];
					snprintf(resourcePath, length, "%srepo/%s/%s", entry, name, info->path);
					deploymentPackage_getName(source, &packageName);

					processor->begin(processor->processor, (char*)packageName);
					processor->process(processor->processor, info->path, resourcePath);

                    free(entry);
				}
			}
		}

		if(services != NULL) {
			arrayList_destroy(services);
		}
	}

    arrayList_destroy(infos);


	return status;
}

celix_status_t deploymentAdmin_dropDeploymentPackageResources(deployment_admin_pt admin, deployment_package_pt source, deployment_package_pt target) {
    celix_status_t status = CELIX_SUCCESS;

    if (target != NULL) {
        array_list_pt infos = NULL;
        deploymentPackage_getResourceInfos(target, &infos);
        int i;
        for (i = 0; i < arrayList_size(infos); i++) {
            resource_info_pt info = arrayList_get(infos, i);
            resource_info_pt sourceInfo = NULL;
            deploymentPackage_getResourceInfoByPath(source, info->path, &sourceInfo);
            if (sourceInfo == NULL) {
                array_list_pt services = NULL;
                int length = strlen(OSGI_FRAMEWORK_SERVICE_PID) + strlen(info->resourceProcessor) + 4;
                char filter[length];

                snprintf(filter, length, "(%s=%s)", OSGI_FRAMEWORK_SERVICE_PID, info->resourceProcessor);
                status = bundleContext_getServiceReferences(admin->context, DEPLOYMENTADMIN_RESOURCE_PROCESSOR_SERVICE, filter, &services);
                if (status == CELIX_SUCCESS) {
                    if (services != NULL && arrayList_size(services) > 0) {
                        service_reference_pt ref = arrayList_get(services, 0);
                        // In Felix a check is done to assure the processor belongs to the deployment package
                        // Is this according to spec?
                        void *processorP = NULL;
                        status = bundleContext_getService(admin->context, ref, &processorP);
                        if (status == CELIX_SUCCESS) {
                            const char *packageName = NULL;
                            resource_processor_service_pt processor = processorP;

                            deploymentPackage_getName(source, &packageName);
                            processor->begin(processor->processor, (char*)packageName);
                            processor->dropped(processor->processor, info->path);
                        }
                    }
                }

                if (services != NULL) {
                    arrayList_destroy(services);
                }

            }
        }

        arrayList_destroy(infos);
    }

    return status;
}

celix_status_t deploymentAdmin_dropDeploymentPackageBundles(deployment_admin_pt admin, deployment_package_pt source, deployment_package_pt target) {
	celix_status_t status = CELIX_SUCCESS;

	if (target != NULL) {
		array_list_pt targetInfos = NULL;
		deploymentPackage_getBundleInfos(target, &targetInfos);
		int i;
		for (i = 0; i < arrayList_size(targetInfos); i++) {
			bundle_info_pt targetInfo = arrayList_get(targetInfos, i);
			if (!targetInfo->customizer) {
				bundle_info_pt info = NULL;
				deploymentPackage_getBundleInfoByName(source, targetInfo->symbolicName, &info);
				if (info == NULL) {
					bundle_pt bundle = NULL;
					deploymentPackage_getBundle(target, targetInfo->symbolicName, &bundle);
					bundle_uninstall(bundle);
				}
			}
		}
		arrayList_destroy(targetInfos);
	}

	return status;
}

celix_status_t deploymentAdmin_startDeploymentPackageBundles(deployment_admin_pt admin, deployment_package_pt source) {
	celix_status_t status = CELIX_SUCCESS;

	array_list_pt infos = NULL;
	deploymentPackage_getBundleInfos(source, &infos);
	int i;
	for (i = 0; i < arrayList_size(infos); i++) {
		bundle_pt bundle = NULL;
		bundle_info_pt info = arrayList_get(infos, i);
		if (!info->customizer) {
			deploymentPackage_getBundle(source, info->symbolicName, &bundle);
			if (bundle != NULL) {
				bundle_start(bundle);
			} else {
				fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "DEPLOYMENT_ADMIN: Could not start bundle %s", info->symbolicName);
			}
		}
	}
	arrayList_destroy(infos);

	return status;
}
