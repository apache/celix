/*
 * deployment_admin.c
 *
 *  Created on: Nov 7, 2011
 *      Author: alexander
 */
#include <stdlib.h>
#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <curl/curl.h>
#include <curl/types.h>
#include <curl/easy.h>
#include <apr_strings.h>

#include "deployment_admin.h"
#include "celix_errno.h"
#include "bundle_context.h"
#include "constants.h"
#include "deployment_package.h"
#include "bundle.h"
#include "utils.h"

#include "resource_processor.h"

#define IDENTIFICATION_ID "deployment_admin.identification"
#define DISCOVERY_URL "deployment_admin.url"
// "http://localhost:8080/deployment/"

#define VERSIONS "/versions"

static void *APR_THREAD_FUNC deploymentAdmin_poll(apr_thread_t *thd, void *deploymentAdmin);
celix_status_t deploymentAdmin_download(char * url, char **inputFile);
size_t deploymentAdmin_writeData(void *ptr, size_t size, size_t nmemb, FILE *stream);
static celix_status_t deploymentAdmin_deleteTree(char * directory, apr_pool_t *mp);
celix_status_t deploymentAdmin_readVersions(deployment_admin_t admin, ARRAY_LIST *versions);

celix_status_t deploymentAdmin_stopDeploymentPackageBundles(deployment_admin_t admin, deployment_package_t target);
celix_status_t deploymentAdmin_updateDeploymentPackageBundles(deployment_admin_t admin, deployment_package_t source);
celix_status_t deploymentAdmin_startDeploymentPackageCustomizerBundles(deployment_admin_t admin, deployment_package_t source, deployment_package_t target);
celix_status_t deploymentAdmin_processDeploymentPackageResources(deployment_admin_t admin, deployment_package_t source);
celix_status_t deploymentAdmin_dropDeploymentPackageResources(deployment_admin_t admin, deployment_package_t source, deployment_package_t target);
celix_status_t deploymentAdmin_dropDeploymentPackageBundles(deployment_admin_t admin, deployment_package_t source, deployment_package_t target);
celix_status_t deploymentAdmin_startDeploymentPackageBundles(deployment_admin_t admin, deployment_package_t source);

celix_status_t deploymentAdmin_create(apr_pool_t *pool, BUNDLE_CONTEXT context, deployment_admin_t *admin) {
	celix_status_t status = CELIX_SUCCESS;
	apr_pool_t *subpool;
	apr_pool_create(&subpool, pool);

	*admin = apr_palloc(subpool, sizeof(**admin));
	if (!*admin) {
		status = CELIX_ENOMEM;
	} else {
		(*admin)->pool = subpool;
		(*admin)->running = true;
		(*admin)->context = context;
		(*admin)->current = NULL;
		(*admin)->packages = hashMap_create(string_hash, NULL, string_equals, NULL);
		(*admin)->targetIdentification = NULL;
		(*admin)->pollUrl = NULL;
        bundleContext_getProperty(context, IDENTIFICATION_ID, &(*admin)->targetIdentification);
		if ((*admin)->targetIdentification == NULL ) {
			printf("Target name must be set using \"deployment_admin.identification\"\n");
			status = CELIX_ILLEGAL_ARGUMENT;
		} else {
			char *url = NULL;
			bundleContext_getProperty(context, DISCOVERY_URL, &url);
			if (url == NULL) {
				printf("URL must be set using \"deployment_admin.url\"\n");
				status = CELIX_ILLEGAL_ARGUMENT;
			} else {
				(*admin)->pollUrl = apr_pstrcat(subpool, url, (*admin)->targetIdentification, VERSIONS, NULL);

				apr_thread_create(&(*admin)->poller, NULL, deploymentAdmin_poll, *admin, subpool);
			}
		}
	}

	return status;
}

static void *APR_THREAD_FUNC deploymentAdmin_poll(apr_thread_t *thd, void *deploymentAdmin) {
	deployment_admin_t admin = deploymentAdmin;

	while (admin->running) {
		//poll ace
		printf("Read version\n");
		ARRAY_LIST versions = NULL;
		deploymentAdmin_readVersions(admin, &versions);

		char *last = arrayList_get(versions, arrayList_size(versions) - 1);

		if (last != NULL) {
			if (admin->current == NULL || strcmp(last, admin->current) > 0) {
				printf("install version: %s\n", last);
				char *request = NULL;
				if (admin->current == NULL) {
					request = apr_pstrcat(admin->pool, admin->pollUrl, "/", last, NULL);
				} else {
					// We do not yet support fix packages
					//request = apr_pstrcat(admin->pool, VERSIONS, "/", last, "?current=", admin->current, NULL);
					request = apr_pstrcat(admin->pool, admin->pollUrl, "/", last, NULL);
				}
				printf("Request: %s\n", request);

				char inputFile[MAXNAMLEN];
				inputFile[0] = '\0';
				char *test = inputFile;
				celix_status_t status = deploymentAdmin_download(request, &test);
				if (status == CELIX_SUCCESS) {
					// Handle file
					printf("Handle file\n");
					char tmpDir[MAXNAMLEN];
					tmpDir[0] = '\0';
					tmpnam(tmpDir);

					apr_dir_make(tmpDir, APR_UREAD|APR_UWRITE|APR_UEXECUTE, admin->pool);

					// TODO: update to use bundle cache DataFile instead of module entries.
					printf("Extract %s t0 %s\n", test, tmpDir);
					extractBundle(test, tmpDir);
					char *manifest = apr_pstrcat(admin->pool, tmpDir, "/META-INF/MANIFEST.MF", NULL);
					MANIFEST mf = NULL;
					manifest_createFromFile(admin->pool, manifest, &mf);
					deployment_package_t source = NULL;
					deploymentPackage_create(admin->pool, admin->context, mf, &source);
					char *name = NULL;
					deploymentPackage_getName(source, &name);

					BUNDLE bundle = NULL;
					bundleContext_getBundle(admin->context, &bundle);
					char *entry = NULL;
					bundle_getEntry(bundle, "/", admin->pool, &entry);
					char *repoDir = apr_pstrcat(admin->pool, entry, "repo", NULL);
					apr_dir_make(repoDir, APR_UREAD|APR_UWRITE|APR_UEXECUTE, admin->pool);
					char *repoCache = apr_pstrcat(admin->pool, entry, "repo/", name, NULL);
					printf("CAche: %s\n", repoCache);
					deploymentAdmin_deleteTree(repoCache, admin->pool);
					apr_status_t stat = apr_file_rename(tmpDir, repoCache, admin->pool);
					if (stat != APR_SUCCESS) {
						printf("No success\n");
					}

					deployment_package_t target = hashMap_get(admin->packages, name);
					if (target == NULL) {
	//					target = empty package
					}

					deploymentAdmin_stopDeploymentPackageBundles(admin, target);
					deploymentAdmin_updateDeploymentPackageBundles(admin, source);
					deploymentAdmin_startDeploymentPackageCustomizerBundles(admin, source, target);
					deploymentAdmin_processDeploymentPackageResources(admin, source);
					deploymentAdmin_dropDeploymentPackageResources(admin, source, target);
					deploymentAdmin_dropDeploymentPackageBundles(admin, source, target);
					deploymentAdmin_startDeploymentPackageBundles(admin, source);

					deploymentAdmin_deleteTree(repoCache, admin->pool);
					deploymentAdmin_deleteTree(tmpDir, admin->pool);
					remove(test);
					admin->current = strdup(last);
					hashMap_put(admin->packages, name, source);
				}
			}
		}
		sleep(5);
	}

	apr_thread_exit(thd, APR_SUCCESS);
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
	printf("not enough memory (realloc returned NULL)\n");
	exit(EXIT_FAILURE);
	}

	memcpy(&(mem->memory[mem->size]), contents, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}

celix_status_t deploymentAdmin_readVersions(deployment_admin_t admin, ARRAY_LIST *versions) {
	celix_status_t status = CELIX_SUCCESS;
	arrayList_create(admin->pool, versions);
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	struct MemoryStruct chunk;
	chunk.memory = calloc(1, sizeof(char));
	chunk.size = 0;
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, admin->pollUrl);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, deploymentAdmin_parseVersions);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, &chunk);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
		printf("Error: %d\n", res);
		/* always cleanup */
		curl_easy_cleanup(curl);

		char *last;
		char *token = apr_strtok(chunk.memory, "\n", &last);
		while (token != NULL) {
			printf("Version: %s\n", token);
			arrayList_add(*versions, apr_pstrdup(admin->pool, token));
			token = apr_strtok(NULL, "\n", &last);
		}
	}



	return status;
}


celix_status_t deploymentAdmin_download(char * url, char **inputFile) {
	celix_status_t status = CELIX_SUCCESS;
	CURL *curl;
	CURLcode res;
	curl = curl_easy_init();
	if (curl) {
		tmpnam(*inputFile);
		printf("Temp file: %s\n", *inputFile);
		FILE *fp = fopen(*inputFile, "wb+");
		curl_easy_setopt(curl, CURLOPT_URL, url);
		curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, deploymentAdmin_writeData);
		curl_easy_setopt(curl, CURLOPT_WRITEDATA, fp);
		curl_easy_setopt(curl, CURLOPT_FAILONERROR, true);
		//curl_easy_setopt(curl, CURLOPT_NOPROGRESS, 0);
		//curl_easy_setopt(curl, CURLOPT_PROGRESSFUNCTION, updateCommand_downloadProgress);
		res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			status = CELIX_BUNDLE_EXCEPTION;
		}
		printf("Error: %d\n", res);
		/* always cleanup */
		curl_easy_cleanup(curl);
		fclose(fp);
	}
	if (res != CURLE_OK) {
		*inputFile[0] = '\0';
		status = CELIX_ILLEGAL_STATE;
	} else {
		status = CELIX_SUCCESS;
	}

	return status;
}

size_t deploymentAdmin_writeData(void *ptr, size_t size, size_t nmemb, FILE *stream) {
    size_t written = fwrite(ptr, size, nmemb, stream);
    return written;
}

static celix_status_t deploymentAdmin_deleteTree(char * directory, apr_pool_t *mp) {
    celix_status_t status = CELIX_SUCCESS;
	apr_dir_t *dir;

	if (directory && mp) {
        if (apr_dir_open(&dir, directory, mp) == APR_SUCCESS) {
            apr_finfo_t dp;
            while ((apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir)) == APR_SUCCESS) {
                if ((strcmp((dp.name), ".") != 0) && (strcmp((dp.name), "..") != 0)) {
                    char subdir[strlen(directory) + strlen(dp.name) + 2];
                    strcpy(subdir, directory);
                    strcat(subdir, "/");
                    strcat(subdir, dp.name);

                    if (dp.filetype == APR_DIR) {
                    	deploymentAdmin_deleteTree(subdir, mp);
                    } else {
                        remove(subdir);
                    }
                }
            }
            remove(directory);
        } else {
            status = CELIX_FILE_IO_EXCEPTION;
        }
	} else {
	    status = CELIX_ILLEGAL_ARGUMENT;
	}

	return status;
}

celix_status_t deploymentAdmin_stopDeploymentPackageBundles(deployment_admin_t admin, deployment_package_t target) {
	celix_status_t status = CELIX_SUCCESS;

	if (target != NULL) {
		ARRAY_LIST infos = NULL;
		deploymentPackage_getBundleInfos(target, &infos);
		int i;
		for (i = 0; i < arrayList_size(infos); i++) {
			BUNDLE bundle = NULL;
			bundle_info_t info = arrayList_get(infos, i);
			deploymentPackage_getBundle(target, info->symbolicName, &bundle);
			if (bundle != NULL) {
				bundle_stop(bundle, 0);
			} else {
				printf("DEPLOYMENT_ADMIN: Bundle %s not found\n", info->symbolicName);
			}
		}
	}

	return status;
}

celix_status_t deploymentAdmin_updateDeploymentPackageBundles(deployment_admin_t admin, deployment_package_t source) {
	celix_status_t status = CELIX_SUCCESS;

	ARRAY_LIST infos = NULL;
	deploymentPackage_getBundleInfos(source, &infos);
	int i;
	for (i = 0; i < arrayList_size(infos); i++) {
		BUNDLE bundle = NULL;
		bundle_info_t info = arrayList_get(infos, i);

		bundleContext_getBundle(admin->context, &bundle);
		char *entry = NULL;
		bundle_getEntry(bundle, "/", admin->pool, &entry);
		char *name = NULL;
		deploymentPackage_getName(source, &name);
		char *bundlePath = apr_pstrcat(admin->pool, entry, "repo/", name, "/", info->path, NULL);
		char *bsn = apr_pstrcat(admin->pool, "osgi-dp:", info->symbolicName, NULL);

		BUNDLE updateBundle = NULL;
		deploymentPackage_getBundle(source, info->symbolicName, &updateBundle);
		if (updateBundle != NULL) {
			printf("Update bundle from: %s\n", bundlePath);
			bundle_update(updateBundle, bundlePath);
		} else {
			printf("Install bundle from: %s\n", bundlePath);
			bundleContext_installBundle2(admin->context, bsn, bundlePath, &updateBundle);
		}
	}

	return status;
}

celix_status_t deploymentAdmin_startDeploymentPackageCustomizerBundles(deployment_admin_t admin, deployment_package_t source, deployment_package_t target) {
	celix_status_t status = CELIX_SUCCESS;

	apr_pool_t *tmpPool = NULL;
	ARRAY_LIST bundles = NULL;
	ARRAY_LIST sourceInfos = NULL;

	apr_pool_create(&tmpPool, admin->pool);
	arrayList_create(tmpPool, &bundles);

	deploymentPackage_getBundleInfos(source, &sourceInfos);
	int i;
	for (i = 0; i < arrayList_size(sourceInfos); i++) {
		bundle_info_t sourceInfo = arrayList_get(sourceInfos, i);
		if (sourceInfo->customizer) {
			BUNDLE bundle = NULL;
			deploymentPackage_getBundle(source, sourceInfo->symbolicName, &bundle);
			if (bundle != NULL) {
				arrayList_add(bundles, bundle);
			}
		}
	}

	if (target != NULL) {
		ARRAY_LIST targetInfos = NULL;
		deploymentPackage_getBundleInfos(target, &targetInfos);
		for (i = 0; i < arrayList_size(targetInfos); i++) {
			bundle_info_t targetInfo = arrayList_get(targetInfos, i);
			if (targetInfo->customizer) {
				BUNDLE bundle = NULL;
				deploymentPackage_getBundle(target, targetInfo->symbolicName, &bundle);
				if (bundle != NULL) {
					arrayList_add(bundles, bundle);
				}
			}
		}
	}

	for (i = 0; i < arrayList_size(bundles); i++) {
		BUNDLE bundle = arrayList_get(bundles, i);
		bundle_start(bundle, 0);
	}

	apr_pool_destroy(tmpPool);

	return status;
}

celix_status_t deploymentAdmin_processDeploymentPackageResources(deployment_admin_t admin, deployment_package_t source) {
	celix_status_t status = CELIX_SUCCESS;

	ARRAY_LIST infos = NULL;
	deploymentPackage_getResourceInfos(source, &infos);
	int i;
	for (i = 0; i < arrayList_size(infos); i++) {
		resource_info_t info = arrayList_get(infos, i);
		apr_pool_t *tmpPool = NULL;
		ARRAY_LIST services = NULL;
		char *filter = NULL;

		printf("Process resource: %s with %s\n", info->path, info->resourceProcessor);

		apr_pool_create(&tmpPool, admin->pool);
		filter = apr_pstrcat(tmpPool, "(", SERVICE_PID, "=", info->resourceProcessor, ")", NULL);

		status = bundleContext_getServiceReferences(admin->context, RESOURCE_PROCESSOR_SERVICE, filter, &services);
		if (status == CELIX_SUCCESS) {
			printf("REFS\n");
			if (services != NULL && arrayList_size(services) > 0) {
				printf("REFS22\n");
				SERVICE_REFERENCE ref = arrayList_get(services, 0);
				// In Felix a check is done to assure the processor belongs to the deployment package
				// Is this according to spec?
				void *processorP = NULL;
				status = bundleContext_getService(admin->context, ref, &processorP);
				if (status == CELIX_SUCCESS) {
					BUNDLE bundle = NULL;
					char *entry = NULL;
					char *name = NULL;
					char *packageName = NULL;
					resource_processor_service_t processor = processorP;

					bundleContext_getBundle(admin->context, &bundle);
					bundle_getEntry(bundle, "/", admin->pool, &entry);
					deploymentPackage_getName(source, &name);

					char *resourcePath = apr_pstrcat(admin->pool, entry, "repo/", name, "/", info->path, NULL);
					deploymentPackage_getName(source, &packageName);

					processor->begin(processor->processor, packageName);
					processor->process(processor->processor, info->path, resourcePath);
				}
			}
		}


	}

	return status;
}

celix_status_t deploymentAdmin_dropDeploymentPackageResources(deployment_admin_t admin, deployment_package_t source, deployment_package_t target) {
	celix_status_t status = CELIX_SUCCESS;

	if (target != NULL) {
		ARRAY_LIST infos = NULL;
		deploymentPackage_getResourceInfos(target, &infos);
		int i;
		for (i = 0; i < arrayList_size(infos); i++) {
			resource_info_t info = arrayList_get(infos, i);
			resource_info_t sourceInfo = NULL;
			deploymentPackage_getResourceInfoByPath(source, info->path, &sourceInfo);
			if (sourceInfo == NULL) {
				apr_pool_t *tmpPool = NULL;
				ARRAY_LIST services = NULL;
				char *filter = NULL;

				printf("Drop resource: %s with %s\n", info->path, info->resourceProcessor);

				apr_pool_create(&tmpPool, admin->pool);
				filter = apr_pstrcat(tmpPool, "(", SERVICE_PID, "=", info->resourceProcessor, ")", NULL);

				status = bundleContext_getServiceReferences(admin->context, RESOURCE_PROCESSOR_SERVICE, filter, &services);
				if (status == CELIX_SUCCESS) {
					if (services != NULL && arrayList_size(services) > 0) {
						SERVICE_REFERENCE ref = arrayList_get(services, 0);
						// In Felix a check is done to assure the processor belongs to the deployment package
						// Is this according to spec?
						void *processorP = NULL;
						status = bundleContext_getService(admin->context, ref, &processorP);
						if (status == CELIX_SUCCESS) {
							BUNDLE bundle = NULL;
							char *packageName = NULL;
							resource_processor_service_t processor = processorP;

							deploymentPackage_getName(source, &packageName);
							processor->begin(processor->processor, packageName);
							processor->dropped(processor->processor, info->path);
						}
					}
				}

			}
		}
	}

	return status;
}

celix_status_t deploymentAdmin_dropDeploymentPackageBundles(deployment_admin_t admin, deployment_package_t source, deployment_package_t target) {
	celix_status_t status = CELIX_SUCCESS;

	if (target != NULL) {
		ARRAY_LIST targetInfos = NULL;
		deploymentPackage_getBundleInfos(target, &targetInfos);
		int i;
		for (i = 0; i < arrayList_size(targetInfos); i++) {
			bundle_info_t targetInfo = arrayList_get(targetInfos, i);
			if (!targetInfo->customizer) {
				bundle_info_t info = NULL;
				deploymentPackage_getBundleInfoByName(source, targetInfo->symbolicName, &info);
				if (info == NULL) {
					BUNDLE bundle = NULL;
					deploymentPackage_getBundle(target, targetInfo->symbolicName, &bundle);
					bundle_uninstall(bundle);
				}
			}
		}
	}

	return status;
}

celix_status_t deploymentAdmin_startDeploymentPackageBundles(deployment_admin_t admin, deployment_package_t source) {
	celix_status_t status = CELIX_SUCCESS;

	ARRAY_LIST infos = NULL;
	deploymentPackage_getBundleInfos(source, &infos);
	int i;
	for (i = 0; i < arrayList_size(infos); i++) {
		BUNDLE bundle = NULL;
		bundle_info_t info = arrayList_get(infos, i);
		if (!info->customizer) {
			deploymentPackage_getBundle(source, info->symbolicName, &bundle);
			if (bundle != NULL) {
				bundle_start(bundle, 0);
			} else {
				printf("DEPLOYMENT_ADMIN: Could not start bundle %s\n", info->symbolicName);
			}
		}
	}

	return status;
}
