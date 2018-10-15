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

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>

#include "bundle_cache_private.h"
#include "bundle_archive.h"
#include "constants.h"
#include "celix_log.h"
#include "celix_properties.h"

static celix_status_t bundleCache_deleteTree(bundle_cache_pt cache, char * directory, bool root);

static const char* bundleCache_progamName() {
#if defined(__APPLE__) || defined(__FreeBSD__)
	return getprogname();
#elif defined(_GNU_SOURCE)
	return program_invocation_short_name;
#else
	return "";
#endif
}

celix_status_t bundleCache_create(const char *fwUUID, celix_properties_t *configurationMap, bundle_cache_pt *bundle_cache) {
	celix_status_t status;
	bundle_cache_pt cache;

	if (configurationMap == NULL || *bundle_cache != NULL) {
		return CELIX_ILLEGAL_ARGUMENT;
	}

	cache = (bundle_cache_pt) calloc(1, sizeof(*cache));
	if (cache == NULL) {
		status = CELIX_ENOMEM;
	} else {
		const char* cacheDir = celix_properties_get(configurationMap, OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".cache");
		bool useTmpDir = celix_properties_getAsBool(configurationMap, OSGI_FRAMEWORK_STORAGE_USE_TMP_DIR, false);
		cache->configurationMap = configurationMap;
		if (cacheDir == NULL || useTmpDir) {
			//Using /tmp dir for cache, so that multiple frameworks can be launched
			//instead of cacheDir = ".cache";
			const char *pg = bundleCache_progamName();
			if (pg == NULL) {
			    pg = "";
			}
			size_t len = (size_t)snprintf(NULL, 0, "/tmp/celix-cache-%s-%s",pg, fwUUID) + 1;
			char *tmpdir = calloc(len, sizeof(char));
			snprintf(tmpdir, len, "/tmp/celix-cache-%s-%s", pg, fwUUID);

			cache->cacheDir = tmpdir;
			cache->deleteOnDestroy = true;
		} else {
			cache->cacheDir = strdup(cacheDir);
			cache->deleteOnDestroy = false;
		}

		*bundle_cache = cache;
		status = CELIX_SUCCESS;
	}

	framework_logIfError(logger, status, NULL, "Failed to create bundle cache");

	return status;
}

celix_status_t bundleCache_destroy(bundle_cache_pt *cache) {
	if ((*cache)->deleteOnDestroy) {
		bundleCache_delete(*cache);
	}
	free((*cache)->cacheDir);
	free(*cache);
	*cache = NULL;

	return CELIX_SUCCESS;
}

celix_status_t bundleCache_delete(bundle_cache_pt cache) {
	return bundleCache_deleteTree(cache, cache->cacheDir, true);
}

celix_status_t bundleCache_getArchives(bundle_cache_pt cache, array_list_pt *archives) {
	celix_status_t status = CELIX_SUCCESS;

	DIR *dir;
	struct stat st;

	dir = opendir(cache->cacheDir);

	if (dir == NULL && errno == ENOENT) {
		if(mkdir(cache->cacheDir, S_IRWXU) == 0 ){
			dir = opendir(cache->cacheDir);
		}
	}

	if (dir != NULL) {
		array_list_pt list = NULL;
		arrayList_create(&list);

		struct dirent* dent = NULL;

		errno = 0;
		dent = readdir(dir);
		while (errno == 0 && dent != NULL) {
			char archiveRoot[512];

			snprintf(archiveRoot, sizeof(archiveRoot), "%s/%s", cache->cacheDir, dent->d_name);

			if (stat (archiveRoot, &st) == 0) {
				if (S_ISDIR (st.st_mode)
						&& (strcmp((dent->d_name), ".") != 0)
						&& (strcmp((dent->d_name), "..") != 0)
						&& (strncmp(dent->d_name, "bundle", 6) == 0)
						&& (strcmp(dent->d_name, "bundle0") != 0)) {

					bundle_archive_pt archive = NULL;
					status = bundleArchive_recreate(archiveRoot, &archive);
					if (status == CELIX_SUCCESS) {
						arrayList_add(list, archive);
					}
				}
			}

			errno = 0;
			dent = readdir(dir);
		}

		if (errno != 0) {
			fw_log(logger, OSGI_FRAMEWORK_LOG_ERROR, "Error reading dir");
			status = CELIX_FILE_IO_EXCEPTION;
		} else {
			status = CELIX_SUCCESS;
		}

		closedir(dir);

		if (status == CELIX_SUCCESS) {
			*archives = list;
		}
		else{
			int idx = 0;
			for(;idx<arrayList_size(list);idx++){
				bundleArchive_destroy((bundle_archive_pt)arrayList_get(list,idx));
			}
			arrayList_destroy(list);
			*archives = NULL;
		}

	} else {
		status = CELIX_FILE_IO_EXCEPTION;
	}

	framework_logIfError(logger, status, NULL, "Failed to get bundle archives");
	if (status != CELIX_SUCCESS) {
		perror("\t");
	}

	return status;
}

celix_status_t bundleCache_createArchive(bundle_cache_pt cache, long id, const char * location, const char *inputFile, bundle_archive_pt *bundle_archive) {
	celix_status_t status = CELIX_SUCCESS;
	char archiveRoot[512];

	if (cache && location) {
		snprintf(archiveRoot, sizeof(archiveRoot), "%s/bundle%ld",  cache->cacheDir, id);
		status = bundleArchive_create(archiveRoot, id, location, inputFile, bundle_archive);
	}

	framework_logIfError(logger, status, NULL, "Failed to create archive");

	return status;
}

static celix_status_t bundleCache_deleteTree(bundle_cache_pt cache, char * directory, bool root) {
	DIR *dir;
	celix_status_t status = CELIX_SUCCESS;
	struct stat st;

	errno = 0;
	dir = opendir(directory);
	if (dir == NULL) {
		status = CELIX_FILE_IO_EXCEPTION;
	} else {
		struct dirent* dent = NULL;
		dent = readdir(dir);
		while (errno == 0 && dent != NULL) {
			if ((strcmp((dent->d_name), ".") != 0) && (strcmp((dent->d_name), "..") != 0)) {
				char subdir[512];
				snprintf(subdir, sizeof(subdir), "%s/%s", directory, dent->d_name);

				if (stat(subdir, &st) == 0) {
					if (S_ISDIR (st.st_mode)) {
						status = bundleCache_deleteTree(cache, subdir, false);
					} else {
						if (remove(subdir) != 0) {
							status = CELIX_FILE_IO_EXCEPTION;
							break;
						}
					}
				}
			}
			errno = 0;
			dent = readdir(dir);
		}

		if (errno != 0) {
			status = CELIX_FILE_IO_EXCEPTION;
		}
		else if (closedir(dir) != 0) {
			status = CELIX_FILE_IO_EXCEPTION;
		}
		if (status == CELIX_SUCCESS) {
			if (rmdir(directory) != 0) {
				status = CELIX_FILE_IO_EXCEPTION;
			}
		}
	}

    if (!root) {
        //note root dir can be non existing
        framework_logIfError(logger, status, NULL, "Failed to delete tree at dir '%s'", directory);
    }

	return status;
}
