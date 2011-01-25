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
#include <dirent.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
//#include <sys/stat.h>

#include <apr-1/apr_file_io.h>

#include "bundle_cache.h"
#include "bundle_archive.h"
#include "headers.h"
#include "constants.h"

struct bundleCache {
	PROPERTIES configurationMap;
	char * cacheDir;
	apr_pool_t *mp;
};

void bundleCache_deleteTree(char * directory, apr_pool_t *mp);

BUNDLE_CACHE bundleCache_create(PROPERTIES configurationMap, apr_pool_t *mp) {
	BUNDLE_CACHE cache = (BUNDLE_CACHE) malloc(sizeof(*cache));

	cache->configurationMap = configurationMap;
	char * cacheDir = properties_get(configurationMap, (char *) FRAMEWORK_STORAGE);
	if (cacheDir == NULL) {
		cacheDir = ".cache";
	}
	cache->cacheDir = cacheDir;
	cache->mp = mp;

	return cache;
}

void bundleCache_delete(BUNDLE_CACHE cache) {
	bundleCache_deleteTree(cache->cacheDir, cache->mp);
}

void bundleCache_deleteTree(char * directory, apr_pool_t *mp) {
	apr_dir_t *dir;
	apr_dir_open(&dir, directory, mp);
	apr_finfo_t dp;
	while (apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir)) {

	//DIR * dir = opendir(directory);
//	struct dirent * dp;
//	while ((dp = readdir(dir))) {
		if ((strcmp((dp.name), ".") != 0) && (strcmp((dp.name), "..") != 0)) {
			char subdir[strlen(directory) + strlen(dp.name) + 2];
			strcpy(subdir, directory);
			strcat(subdir, "/");
			strcat(subdir, dp.name);

//			struct stat s;
//			stat(dp->d_name, &s);
			if (dp.filetype = APR_DIR) {
//			if (S_ISDIR(s.st_mode)) {
//			if (dp->d_type == DT_DIR) {
				bundleCache_deleteTree(subdir, mp);
			} else {
				remove(subdir);
			}
		}
	}
	remove(directory);
}

ARRAY_LIST bundleCache_getArchives(BUNDLE_CACHE cache) {
	apr_dir_t *dir;
	apr_status_t status = apr_dir_open(&dir, cache->cacheDir, cache->mp);

	//DIR * dir = opendir(cache->cacheDir);
	if (status == APR_ENOENT) {
		apr_dir_make(cache->cacheDir, APR_UREAD|APR_UWRITE|APR_UEXECUTE, cache->mp);
		status = apr_dir_open(&dir, cache->cacheDir, cache->mp);
	}

	ARRAY_LIST list = arrayList_create();
	apr_finfo_t dp;
	//apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir);
	while ((apr_dir_read(&dp, APR_FINFO_DIRENT|APR_FINFO_TYPE, dir))) {
	//struct dirent * dp;
	//while ((dp = readdir(dir))) {
		char archiveRoot[strlen(cache->cacheDir) + strlen(dp.name) + 2];
		strcpy(archiveRoot, cache->cacheDir);
		strcat(archiveRoot, "/");
		strcat(archiveRoot, dp.name);

		if (dp.filetype == APR_DIR
//		struct stat s;
//		stat(archiveRoot, &s);
//		if (S_ISDIR(s.st_mode)
//		if (dp->d_type == DT_DIR
				&& (strcmp((dp.name), ".") != 0)
				&& (strcmp((dp.name), "..") != 0)
				&& (strncmp(dp.name, "bundle", 6) == 0)
				&& (strcmp(dp.name, "bundle0") != 0)) {

			BUNDLE_ARCHIVE archive = bundleArchive_recreate(strdup(archiveRoot), cache->mp);
			arrayList_add(list, archive);
		}
	}

	return list;
}

BUNDLE_ARCHIVE bundleCache_createArchive(BUNDLE_CACHE cache, long id, char * location) {
	char archiveRoot[256];
	sprintf(archiveRoot, "%s/bundle%ld",  cache->cacheDir, id);

	return bundleArchive_create(strdup(archiveRoot), id, location, cache->mp);
}
