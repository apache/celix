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

#include "celix_bundle_cache.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <dirent.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/errno.h>
#include <dlfcn.h>

#include "bundle_archive.h"
#include "celix_constants.h"
#include "celix_log.h"
#include "celix_properties.h"
#include "celix_file_utils.h"
#include "celix_utils.h"

static const char * const EMBEDDED_BUNDLE_PREFIX = "celix_embedded_bundle_";
static const char * const EMBEDDED_BUNDLE_START_POSTFIX = "_start";
static const char * const EMBEDDED_BUNDLE_END_POSTFIX = "_end";

#define FW_LOG(level, ...) \
    celix_framework_log(celix_frameworkLogger_globalLogger(), (level), __FUNCTION__ , __FILE__, __LINE__, __VA_ARGS__)

struct celix_bundle_cache {
    char* cacheDir;
    bool deleteOnDestroy;
};

static const char* bundleCache_progamName() {
#if defined(__APPLE__) || defined(__FreeBSD__)
	return getprogname();
#elif defined(_GNU_SOURCE)
	return program_invocation_short_name;
#else
	return "";
#endif
}

celix_status_t celix_bundleCache_create(const char *fwUUID, const celix_properties_t* configurationMap, celix_bundle_cache_t **out) {
    if (configurationMap == NULL) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Cannot create cache with a NULL configurationMap");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    celix_bundle_cache_t* cache = calloc(1, sizeof(*cache));

    const char* cacheDir = celix_properties_get(configurationMap, OSGI_FRAMEWORK_FRAMEWORK_STORAGE, ".cache");
    bool useTmpDir = celix_properties_getAsBool(configurationMap, OSGI_FRAMEWORK_STORAGE_USE_TMP_DIR, false);
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

    *out = cache;
	return CELIX_SUCCESS;
}

celix_status_t celix_bundleCache_destroy(celix_bundle_cache_t* cache) {
    celix_status_t status = CELIX_SUCCESS;
	if (cache->deleteOnDestroy) {
        status = celix_bundleCache_delete(cache);
	}
	free(cache->cacheDir);
	free(cache);
	return status;
}

celix_status_t celix_bundleCache_delete(celix_bundle_cache_t* cache) {
    const char* err = NULL;
    celix_status_t status = celix_utils_deleteDirectory(cache->cacheDir, &err);
    if (err != NULL) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Cannot delete cache dir at %s: %s", cache->cacheDir, err);
    }
    return status;
}

celix_status_t celix_bundleCache_getArchives(celix_bundle_cache_t* cache, array_list_pt *archives) {
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
			fw_log(celix_frameworkLogger_globalLogger(), CELIX_LOG_LEVEL_ERROR, "Error reading dir");
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

	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Failed to get bundle archives");
	if (status != CELIX_SUCCESS) {
		perror("\t");
	}

	return status;
}

celix_status_t celix_bundleCache_createArchive(celix_bundle_cache_t* cache, long id, const char * location, const char *inputFile, bundle_archive_pt *archive) {
	celix_status_t status = CELIX_SUCCESS;
	char archiveRoot[512];

	if (cache && location) {
		snprintf(archiveRoot, sizeof(archiveRoot), "%s/bundle%ld",  cache->cacheDir, id);
		status = bundleArchive_create(archiveRoot, id, location, inputFile, archive);
	}

	framework_logIfError(celix_frameworkLogger_globalLogger(), status, NULL, "Failed to create archive");

	return status;
}

static bool extractBundlePath(const char* bundlePath, const char* bundleCache) {
    const char* err = NULL;
    FW_LOG(CELIX_LOG_LEVEL_DEBUG, "Extracting bundle url `%s` to bundle cache `%s`", bundlePath, bundleCache);
    celix_status_t status = celix_utils_extractZipFile(bundlePath, bundleCache, &err);
    if (status == CELIX_SUCCESS) {
        FW_LOG(CELIX_LOG_LEVEL_TRACE, "Bundle zip `%s` extracted to `%s`", bundlePath, bundleCache);
    } else {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Could not extract bundle zip `%s` to `%s`: %s", bundlePath, bundleCache, err);
    }
    return status == CELIX_SUCCESS;
}

static bool extractBundleEmbedded(const char* embeddedBundle, const char* bundleCache) {
    FW_LOG(CELIX_LOG_LEVEL_DEBUG, "Extracting embedded bundle `%s` to bundle cache `%s`", embeddedBundle, bundleCache);

    /* TBD, maybe still needed for osx
    unsigned long dataSize;
    void* data;
    data = getsectdata("bundles", embeddedSymbol, &dataSize);

    if (data == NULL) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Cannot extract embedded bundle, could not find sectdata in executable `%s` for segname `%s` and sectname `%s`", getprogname(), "bundles", embeddedSymbol);
        return false;
    }
    intptr_t slide = _dyld_get_image_vmaddr_slide(0);
    data = (void*)(slide + (intptr_t)data);
    const char* err = NULL;
    celix_status_t status = celix_utils_extractZipData(data, (size_t)dataSize, bundleCache, &err);
    if (status == CELIX_SUCCESS) {
        FW_LOG(CELIX_LOG_LEVEL_TRACE, "Embedded bundle zip `%s` extracted to `%s`", embeddedSymbol, bundleCache);
    } else {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Could not extract embedded bundle zip `%s` to `%s`: %s", embeddedSymbol, bundleCache, err);
    }
    return status == CELIX_SUCCESS;
    */

    char* startSymbol = NULL;
    char* endSymbol = NULL;
    asprintf(&startSymbol, "%s%s%s", EMBEDDED_BUNDLE_PREFIX, embeddedBundle, EMBEDDED_BUNDLE_START_POSTFIX);
    asprintf(&endSymbol, "%s%s%s", EMBEDDED_BUNDLE_PREFIX, embeddedBundle, EMBEDDED_BUNDLE_END_POSTFIX);

    void* main = dlopen(NULL, RTLD_NOW);
    void* start = dlsym(main, startSymbol);
    void* end = dlsym(main, endSymbol);

    if (start == NULL || end == NULL) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Cannot extract embedded bundle, could not find symbols `%s` and/or `%s` for embedded bundle `%s`", startSymbol, endSymbol, embeddedBundle);
        free(startSymbol);
        free(endSymbol);
        return false;
    }
    free(startSymbol);
    free(endSymbol);

    const char* err = NULL;
    celix_status_t status = celix_utils_extractZipData(start, end-start, bundleCache, &err);
    if (status == CELIX_SUCCESS) {
        FW_LOG(CELIX_LOG_LEVEL_TRACE, "Embedded bundle zip `%s` extracted to `%s`", embeddedBundle, bundleCache);
    } else {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Could not extract embedded bundle zip `%s` to `%s`: %s", embeddedBundle, bundleCache, err);
    }
    return status == CELIX_SUCCESS;
}

char* celix_bundleCache_extractBundle(celix_bundle_cache_t* cache, const char* bundleCacheDir, const char* bundleURL) {
    char* result = NULL;

    if (celix_utils_isStringNullOrEmpty(bundleURL)) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Invalid NULL or empty bundleURL argument");
        return result;
    }

    char* trimmedUrl = celix_utils_trim(bundleURL);
    asprintf(&result, "%s/%s", cache->cacheDir, bundleCacheDir);

    bool extracted;
    if (strncasecmp("file://", bundleURL, 7) == 0) {
        extracted = extractBundlePath(trimmedUrl+7, result); //note +7 to remove the file:// part.
    } else if (strncasecmp("embedded://", bundleURL, 11) == 0) {
        extracted = extractBundleEmbedded(trimmedUrl+11, result); //note +11 to remove the embedded:// part.
    } else if (strcasestr(bundleURL, "://")) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Cannot extract bundle url '%s', because url type is not supported. Supported types are file:// or embedded://.", bundleURL);
        extracted = false;
    } else {
        extracted = extractBundlePath(trimmedUrl, result);
    }

    free(trimmedUrl);
    if (!extracted) {
        free(result);
        return NULL;
    }
    return result;
}