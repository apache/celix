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
 * bundle_cache.c
 *
 *  \date       Aug 6, 2010
 *  \author    	<a href="mailto:celix-dev@incubator.apache.org">Apache Celix Project Team</a>
 *  \copyright	Apache License, Version 2.0
 */
#ifndef _WIN32
#include <dirent.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <apr_file_io.h>
#include <apr_strings.h>

#include "bundle_cache_private.h"
#include "bundle_archive.h"
#include "constants.h"
#include "celix_log.h"

static celix_status_t bundleCache_deleteTree(char * directory, apr_pool_t *mp);
static apr_status_t bundleCache_destroy(void *cacheP);

celix_status_t bundleCache_create(properties_pt configurationMap, apr_pool_t *mp, bundle_cache_pt *bundle_cache) {
    celix_status_t status;
    bundle_cache_pt cache;

	cache = (bundle_cache_pt) apr_palloc(mp, (sizeof(*cache)));
    if (cache == NULL) {
        status = CELIX_ENOMEM;
    } else {
    	apr_pool_pre_cleanup_register(mp, cache, bundleCache_destroy);

		if (configurationMap != NULL && mp != NULL && *bundle_cache == NULL) {
            char * cacheDir = properties_get(configurationMap, (char *) OSGI_FRAMEWORK_FRAMEWORK_STORAGE);
			cache->configurationMap = configurationMap;
            if (cacheDir == NULL) {
                cacheDir = ".cache";
            }
            cache->cacheDir = cacheDir;
            cache->mp = mp;

            *bundle_cache = cache;
            status = CELIX_SUCCESS;
        } else {
            status = CELIX_ILLEGAL_ARGUMENT;
        }
    }

    framework_logIfError(status, NULL, "Failed to create bundle cache");

	return status;
}

apr_status_t bundleCache_destroy(void *cacheP) {
	bundle_cache_pt cache = (bundle_cache_pt) cacheP;
    properties_destroy(cache->configurationMap);
    return CELIX_SUCCESS;
}

celix_status_t bundleCache_delete(bundle_cache_pt cache) {
	return bundleCache_deleteTree(cache->cacheDir, cache->mp);
}

celix_status_t bundleCache_getArchives(bundle_cache_pt cache, apr_pool_t *pool, array_list_pt *archives) {
	celix_status_t status = CELIX_SUCCESS;

	apr_dir_t *dir;
	apr_status_t aprStatus = apr_dir_open(&dir, cache->cacheDir, pool);

	if (APR_STATUS_IS_ENOENT(aprStatus)) {
		apr_dir_make(cache->cacheDir, APR_UREAD|APR_UWRITE|APR_UEXECUTE, cache->mp);
		aprStatus = apr_dir_open(&dir, cache->cacheDir, pool);
	}

	if (aprStatus == APR_SUCCESS) {
        array_list_pt list = NULL;
		apr_finfo_t dp;
        arrayList_create(&list);
        
        while ((apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir)) == APR_SUCCESS) {
        	apr_pool_t *subpool = NULL;
			char *archiveRoot = NULL;

        	apr_pool_create(&subpool, pool);
        	archiveRoot = (char *)apr_palloc(subpool, strlen(cache->cacheDir) + strlen(dp.name) + 2);

            strcpy(archiveRoot, cache->cacheDir);
            strcat(archiveRoot, "/");
            strcat(archiveRoot, dp.name);

            if (dp.filetype == APR_DIR
                    && (strcmp((dp.name), ".") != 0)
                    && (strcmp((dp.name), "..") != 0)
                    && (strncmp(dp.name, "bundle", 6) == 0)
                    && (strcmp(dp.name, "bundle0") != 0)) {

                bundle_archive_pt archive = NULL;
                status = bundleArchive_recreate(apr_pstrdup(cache->mp, archiveRoot), cache->mp, &archive);
                if (status == CELIX_SUCCESS) {
                    arrayList_add(list, archive);
                }
            }
			apr_pool_destroy(subpool);
        }

        apr_dir_close(dir);

        *archives = list;

        status = CELIX_SUCCESS;
	} else {
	    status = CELIX_FILE_IO_EXCEPTION;
	}

	framework_logIfError(status, NULL, "Failed to get bundle archives");

	return status;
}

celix_status_t bundleCache_createArchive(bundle_cache_pt cache, apr_pool_t *bundlePool, long id, char * location, char *inputFile, bundle_archive_pt *bundle_archive) {
	celix_status_t status = CELIX_SUCCESS;
	char *archiveRoot = NULL;

	if (cache && location && bundlePool) {
		archiveRoot = apr_psprintf(bundlePool, "%s/bundle%ld",  cache->cacheDir, id);
        status = bundleArchive_create(archiveRoot, id, location, inputFile, bundlePool, bundle_archive);
	}

	framework_logIfError(status, NULL, "Failed to create archive");

	return status;
}

static celix_status_t bundleCache_deleteTree(char * directory, apr_pool_t *mp) {
    celix_status_t status = CELIX_SUCCESS;
	apr_dir_t *dir;

	if (directory && mp) {
        if (apr_dir_open(&dir, directory, mp) == APR_SUCCESS) {
            apr_finfo_t dp;
            while ((apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir)) == APR_SUCCESS) {
                if ((strcmp((dp.name), ".") != 0) && (strcmp((dp.name), "..") != 0)) {
                    char *subdir = (char *) apr_palloc(mp, sizeof(*subdir) * (strlen(directory) + strlen(dp.name) + 2));
                    strcpy(subdir, directory);
                    strcat(subdir, "/");
                    strcat(subdir, dp.name);

                    if (dp.filetype == APR_DIR) {
                        bundleCache_deleteTree(subdir, mp);
                    } else {
                    	apr_file_remove(subdir, mp);
                    }
                }
            }
            apr_dir_close(dir);
            apr_dir_remove(directory, mp);
        } else {
            status = CELIX_FILE_IO_EXCEPTION;
        }
	} else {
	    status = CELIX_ILLEGAL_ARGUMENT;
	}

	framework_logIfError(status, NULL, "Failed to delete tree");

	return status;
}
