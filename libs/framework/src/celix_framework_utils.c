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

#include "celix_framework_utils.h"
#include "celix_framework_utils_private.h"

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dlfcn.h>
#include <assert.h>

#include "bundle_archive.h"
#include "celix_constants.h"
#include "celix_log.h"
#include "celix_properties.h"
#include "celix_file_utils.h"
#include "celix_utils.h"
#include "celix_bundle_context.h"
#include "framework_private.h"

static const char * const FILE_URL_SCHEME = "file://";

static const char * const EMBEDDED_URL_SCHEME = "embedded://";
static const char * const EMBEDDED_BUNDLES_SYMBOL = "celix_embedded_bundles";
static const char * const EMBEDDED_BUNDLE_PREFIX = "celix_embedded_bundle_";
static const char * const EMBEDDED_BUNDLE_START_POSTFIX = "_start";
static const char * const EMBEDDED_BUNDLE_END_POSTFIX = "_end";

#define FW_LOG(level, ...) do {                                                                                                 \
    if (fw) {                                                                                                                   \
        celix_framework_log(fw->logger, (level), __FUNCTION__ , __FILE__, __LINE__, __VA_ARGS__);                               \
    } else {                                                                                                                    \
        celix_framework_log(celix_frameworkLogger_globalLogger(), (level), __FUNCTION__ , __FILE__, __LINE__, __VA_ARGS__);     \
    }                                                                                                                           \
} while(0)

static char* resolveFileBundleUrl(celix_framework_t* fw, const char* bundleLocation, bool silent) {
    char *result = NULL;

    const char *bundlePath = NULL;
    fw_getProperty(fw, CELIX_BUNDLES_PATH_NAME, CELIX_BUNDLES_PATH_DEFAULT, &bundlePath);

    if (!celix_utils_isStringNullOrEmpty(bundleLocation)) {
        if (bundleLocation[0] == '/') {
            //absolute path.
            if (celix_utils_fileExists(bundleLocation)) {
                result = celix_utils_strdup(bundleLocation);
            } else if (!silent) {
                FW_LOG(CELIX_LOG_LEVEL_ERROR, "Bundle location '%s' is not valid, file does not exist.", bundleLocation);
            }
        } else if (celix_utils_fileExists(bundleLocation)) {
            //relative path to existing file.
            result = celix_utils_strdup(bundleLocation);
        } else {
            //relative path to a non-existing file, check the bundle paths.
            char *paths = celix_utils_strdup(bundlePath);
            const char *sep = ":";
            char *savePtr = NULL;

            for (char *path = strtok_r(paths, sep, &savePtr); path != NULL; path = strtok_r(NULL, sep, &savePtr)){
                char *absPath = NULL;
                asprintf(&absPath, "%s/%s", path, bundleLocation);
                if (celix_utils_fileExists(absPath)) {
                    result = absPath;
                    break;
                } else {
                    free(absPath);
                }
            }
            free(paths);

            if (result == NULL && !silent) {
                FW_LOG(CELIX_LOG_LEVEL_ERROR, "Bundle location '%s' is not valid. Relative path does not point to existing file taking into account the cwd and Celix bundle path '%s'.", bundleLocation, bundlePath);
            }
        }
    } else if (!silent) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Invalid bundle empty bundle path.");
    }
    return result;
}

static bool isEmbeddedBundleUrlValid(celix_framework_t *fw, const char* bundleURL, bool silent) {
    bool valid = true;

    char* startSymbol = NULL;
    char* endSymbol = NULL;
    int offset = strlen(EMBEDDED_URL_SCHEME); //offset to remove the EMBEDDED_URL_SCHEME part.
    asprintf(&startSymbol, "%s%s%s", EMBEDDED_BUNDLE_PREFIX, bundleURL+offset, EMBEDDED_BUNDLE_START_POSTFIX);
    asprintf(&endSymbol, "%s%s%s", EMBEDDED_BUNDLE_PREFIX, bundleURL+offset, EMBEDDED_BUNDLE_END_POSTFIX);

    void* main = dlopen(NULL, RTLD_NOW);
    void* start = dlsym(main, startSymbol);
    void* end = dlsym(main, endSymbol);
    dlclose(main);

    if (start == NULL || end == NULL) {
        valid = false;
    }

    if (!valid && !silent) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Invalid bundle URL '%s'. Cannot find start symbol %s and/or end symbol %s. Ensure that the bundle is embedded with the CMake command `celix_target_embed_bundles` or `celix_target_embed_bundle`.", bundleURL, startSymbol, endSymbol);
    }

    free(startSymbol);
    free(endSymbol);

    return valid;
}

static bool extractBundlePath(celix_framework_t *fw, const char* bundlePath, const char* extractPath) {
    FW_LOG(CELIX_LOG_LEVEL_TRACE, "Extracting bundle url `%s` to dir `%s`", bundlePath, extractPath);
    const char* err = NULL;

    char* resolvedPath = resolveFileBundleUrl(fw, bundlePath, false);
    assert(resolvedPath != NULL); //should be caught by celix_framework_utils_isBundleUrlValid

    celix_status_t status = celix_utils_extractZipFile(resolvedPath, extractPath, &err);
    if (status == CELIX_SUCCESS) {
        FW_LOG(CELIX_LOG_LEVEL_TRACE, "Bundle zip `%s` extracted.", resolvedPath);
    } else {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Could not extract bundle zip file `%s` to `%s`: %s", resolvedPath, extractPath, err);
    }
    free(resolvedPath);
    return status == CELIX_SUCCESS;
}

static bool extractBundleEmbedded(celix_framework_t *fw, const char* embeddedBundle, const char* extractPath) {
    FW_LOG(CELIX_LOG_LEVEL_TRACE, "Extracting embedded bundle `%s` to dir `%s`", embeddedBundle, extractPath);
    char* startSymbol = NULL;
    char* endSymbol = NULL;
    asprintf(&startSymbol, "%s%s%s", EMBEDDED_BUNDLE_PREFIX, embeddedBundle, EMBEDDED_BUNDLE_START_POSTFIX);
    asprintf(&endSymbol, "%s%s%s", EMBEDDED_BUNDLE_PREFIX, embeddedBundle, EMBEDDED_BUNDLE_END_POSTFIX);

    void* main = dlopen(NULL, RTLD_NOW);
    void* start = dlsym(main, startSymbol);
    void* end = dlsym(main, endSymbol);
    dlclose(main);

    if (start == NULL || end == NULL) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Cannot extract embedded bundle, could not find symbols `%s` and/or `%s` for embedded bundle `%s`", startSymbol, endSymbol, embeddedBundle);
        free(startSymbol);
        free(endSymbol);
        return false;
    }
    free(startSymbol);
    free(endSymbol);

    const char* err = NULL;
    celix_status_t status = celix_utils_extractZipData(start, end-start, extractPath, &err);
    if (status == CELIX_SUCCESS) {
        FW_LOG(CELIX_LOG_LEVEL_TRACE, "Embedded bundle zip `%s` extracted to `%s`", embeddedBundle, extractPath);
    } else {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Could not extract embedded bundle zip `%s` to `%s`: %s", embeddedBundle, extractPath, err);
    }
    return status == CELIX_SUCCESS;
}

celix_status_t celix_framework_utils_extractBundle(celix_framework_t *fw, const char *bundleURL,  const char* extractPath) {
    if (!celix_framework_utils_isBundleUrlValid(fw, bundleURL, false)) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    char* trimmedUrl = celix_utils_trim(bundleURL);

    bool extracted;
    int fileSchemeLen = strlen(FILE_URL_SCHEME);
    int embeddedSchemeLen = strlen(EMBEDDED_URL_SCHEME);
    if (strncasecmp(FILE_URL_SCHEME, trimmedUrl, fileSchemeLen) == 0) {
        extracted = extractBundlePath(fw, trimmedUrl+fileSchemeLen, extractPath);
    } else if (strncasecmp(EMBEDDED_URL_SCHEME, trimmedUrl, embeddedSchemeLen) == 0) {
        extracted = extractBundleEmbedded(fw, trimmedUrl+embeddedSchemeLen, extractPath);
    } else {
        extracted = extractBundlePath(fw, trimmedUrl, extractPath);
    }

    free(trimmedUrl);
    return extracted ? CELIX_SUCCESS : CELIX_FILE_IO_EXCEPTION;
}

bool celix_framework_utils_isBundleUrlValid(celix_framework_t *fw, const char *bundleURL, bool silent) {
    char* trimmedUrl = celix_utils_trim(bundleURL);

    if (celix_utils_isStringNullOrEmpty(trimmedUrl)) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Invalid NULL or empty bundleURL argument. Provided argument is '%s'", bundleURL);
        free(trimmedUrl);
        return false;
    }

    bool valid;
    if (strncasecmp("file://", trimmedUrl, 7) == 0) {
        char* loc = resolveFileBundleUrl(fw, trimmedUrl+7, silent);
        valid = loc != NULL;
        free(loc);
    } else if (strncasecmp(EMBEDDED_URL_SCHEME, trimmedUrl, 11) == 0) {
        valid = isEmbeddedBundleUrlValid(fw, trimmedUrl, silent);
    } else if (strcasestr(trimmedUrl, "://")) {
        valid = false;
        if (!silent) {
            FW_LOG(CELIX_LOG_LEVEL_ERROR, "Bundle URL '%s' is not a valid url. Scheme is not supported.", bundleURL);
        }
    } else {
        char* loc = resolveFileBundleUrl(fw, trimmedUrl, silent);
        valid = loc != NULL;
        free(loc);
    }
    free(trimmedUrl);

    return valid;
}

celix_array_list_t* celix_framework_utils_listEmbeddedBundles() {
    celix_array_list_t* list = celix_arrayList_create();
    void* main = dlopen(NULL, RTLD_NOW);
    const char** embeddedBundles = dlsym(main, EMBEDDED_BUNDLES_SYMBOL);
    if (embeddedBundles != NULL) {
        char delims[] = ",";
        char *savePtr = NULL;
        char *bundles = celix_utils_strdup(*embeddedBundles);
        for (char *url = strtok_r(bundles, delims, &savePtr); url != NULL; url = strtok_r(NULL, delims, &savePtr)) {
            celix_arrayList_add(list, celix_utils_strdup(url));
        }
        free(bundles);
    }
    dlclose(main);
    return list;
}

size_t celix_framework_utils_installEmbeddedBundles(celix_framework_t* fw, bool autoStart) {
    size_t nrOfBundlesInstalled = 0;
    celix_array_list_t* list = celix_framework_utils_listEmbeddedBundles();
    celix_array_list_t* bundleIds = celix_arrayList_create();
    for (int i = 0; i < celix_arrayList_size(list); ++i) {
        char* url = celix_arrayList_get(list, i);
        long bndId = celix_framework_installBundle(fw, url, false);
        if (bndId > 0) {
            nrOfBundlesInstalled += 1;
            celix_arrayList_addLong(bundleIds, bndId);
        }
        free(url);
    }
    celix_arrayList_destroy(list);

    for (int i = 0; i < celix_arrayList_size(bundleIds) && autoStart; ++i) {
        long bndId = celix_arrayList_getLong(bundleIds, i);
        celix_framework_startBundle(fw, bndId);
    }
    celix_arrayList_destroy(bundleIds);

    return nrOfBundlesInstalled;
}

size_t celix_framework_utils_installBundleSet(celix_framework_t* fw, const char* bundleSet, bool autoStart) {
    size_t installed = 0;
    celix_array_list_t* bundleIds = celix_arrayList_create();
    char delims[] = ",";
    char *savePtr = NULL;
    char *bundles = celix_utils_strdup(bundleSet);
    for (char *url = strtok_r(bundles, delims, &savePtr); url != NULL; url = strtok_r(NULL, delims, &savePtr)) {
        long bndId = celix_framework_installBundle(fw, url, false);
        if (bndId > 0) {
            installed += 1;
            celix_arrayList_addLong(bundleIds, bndId);
        }
    }
    free(bundles);

    for (int i = 0; i < celix_arrayList_size(bundleIds) && autoStart; ++i) {
        long bndId = celix_arrayList_getLong(bundleIds, i);
        celix_framework_startBundle(fw, bndId);
    }
    celix_arrayList_destroy(bundleIds);

    return installed;
}
