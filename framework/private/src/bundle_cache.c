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
 *  Created on: Aug 6, 2010
 *      Author: alexanderb
 */
#ifndef _WIN32
#include <dirent.h>
#endif
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <apr_file_io.h>
#include <apr_strings.h>

#include "bundle_cache.h"
#include "bundle_archive.h"
#include "constants.h"

struct bundleCache {
	PROPERTIES configurationMap;
	char * cacheDir;
	apr_pool_t *mp;
};

static celix_status_t bundleCache_deleteTree(char * directory, apr_pool_t *mp);
static apr_status_t bundleCache_destroy(void *cacheP);

celix_status_t bundleCache_create(PROPERTIES configurationMap, apr_pool_t *mp, BUNDLE_CACHE *bundle_cache) {
    celix_status_t status;
    BUNDLE_CACHE cache;

	cache = (BUNDLE_CACHE) apr_palloc(mp, (sizeof(*cache)));
    if (cache == NULL) {
        status = CELIX_ENOMEM;
    } else {
    	apr_pool_pre_cleanup_register(mp, cache, bundleCache_destroy);

		if (configurationMap != NULL && mp != NULL && *bundle_cache == NULL) {
            char * cacheDir = properties_get(configurationMap, (char *) FRAMEWORK_STORAGE);
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

	return status;
}

apr_status_t bundleCache_destroy(void *cacheP) {
	BUNDLE_CACHE cache = cacheP;
    properties_destroy(cache->configurationMap);
    return CELIX_SUCCESS;
}

celix_status_t bundleCache_delete(BUNDLE_CACHE cache) {
	return bundleCache_deleteTree(cache->cacheDir, cache->mp);
}

celix_status_t bundleCache_getArchives(BUNDLE_CACHE cache, apr_pool_t *pool, ARRAY_LIST *archives) {
	celix_status_t status = CELIX_SUCCESS;

	apr_dir_t *dir;
	apr_status_t aprStatus = apr_dir_open(&dir, cache->cacheDir, pool);

	if (aprStatus == APR_ENOENT) {
		apr_dir_make(cache->cacheDir, APR_UREAD|APR_UWRITE|APR_UEXECUTE, cache->mp);
		aprStatus = apr_dir_open(&dir, cache->cacheDir, pool);
	}

	if (aprStatus == APR_SUCCESS) {
        ARRAY_LIST list = NULL;
		apr_finfo_t dp;
        arrayList_create(pool, &list);
        
        while ((apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir)) == APR_SUCCESS) {
            char * archiveRoot = (char *)malloc(strlen(cache->cacheDir) + strlen(dp.name) + 2);
            strcpy(archiveRoot, cache->cacheDir);
            strcat(archiveRoot, "/");
            strcat(archiveRoot, dp.name);

            if (dp.filetype == APR_DIR
                    && (strcmp((dp.name), ".") != 0)
                    && (strcmp((dp.name), "..") != 0)
                    && (strncmp(dp.name, "bundle", 6) == 0)
                    && (strcmp(dp.name, "bundle0") != 0)) {

                BUNDLE_ARCHIVE archive = NULL;
                status = bundleArchive_recreate(apr_pstrdup(cache->mp, archiveRoot), cache->mp, &archive);
                if (status == CELIX_SUCCESS) {
                    arrayList_add(list, archive);
                }
            }
			free(archiveRoot);
        }

        apr_dir_close(dir);

        *archives = list;

        status = CELIX_SUCCESS;
	} else {
	    status = CELIX_FILE_IO_EXCEPTION;
	}

	return status;
}

celix_status_t bundleCache_createArchive(BUNDLE_CACHE cache, apr_pool_t *bundlePool, long id, char * location, char *inputFile, BUNDLE_ARCHIVE *bundle_archive) {
    celix_status_t status;
	char archiveRoot[256];
    BUNDLE_ARCHIVE archive;

	if (cache && location && bundlePool) {
        sprintf(archiveRoot, "%s/bundle%ld",  cache->cacheDir, id);

        status = bundleArchive_create(apr_pstrdup(cache->mp, archiveRoot), id, location, inputFile, bundlePool, bundle_archive);
	}

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

	return status;
}
