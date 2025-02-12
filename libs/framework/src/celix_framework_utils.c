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

#include <assert.h>
#include <dlfcn.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#include "celix_bundle_archive.h"
#include "celix_bundle_context.h"
#include "celix_constants.h"
#include "celix_file_utils.h"
#include "celix_log.h"
#include "celix_properties.h"
#include "celix_utils.h"
#include "framework_private.h"

#define FILE_URL_SCHEME "file://"

#define FW_LOG(level, ...) do {                                                                                                 \
    if (fw) {                                                                                                                   \
        celix_framework_log(fw->logger, (level), __FUNCTION__ , __FILE__, __LINE__, __VA_ARGS__);                               \
    } else {                                                                                                                    \
        celix_framework_log(celix_frameworkLogger_globalLogger(), (level), __FUNCTION__ , __FILE__, __LINE__, __VA_ARGS__);     \
    }                                                                                                                           \
} while(0)

/**
 * @brief Resolves path to bundle. Uses the provided pathBuffer if is big enough, otherwise allocates a new buffer.
 * @note The returned pathBuffer can be part of the provide buffer, when done the result must be freed by calling
 * celix_utils_freeStringIfNotEqual.
 */
static char* celix_framework_utils_resolveFileBundleUrl(char* pathBuffer, size_t pathBufferSize, celix_framework_t* fw, const char* bundleLocation, bool silent) {
    char *result = NULL;
    if (celix_utils_isStringNullOrEmpty(bundleLocation)) {
        if (!silent) {
            FW_LOG(CELIX_LOG_LEVEL_ERROR, "Invalid bundle empty bundle path.");
        }
        return NULL;
    }

    const char *bundlePath = celix_framework_getConfigProperty(fw, CELIX_BUNDLES_PATH_NAME, CELIX_BUNDLES_PATH_DEFAULT, NULL);
    if (celix_utils_fileExists(bundleLocation)) {
        //absolute/relative path to existing file.
        result = celix_utils_writeOrCreateString(pathBuffer, pathBufferSize, "%s", bundleLocation);
    } else if (bundleLocation[0] != '/'){
        errno = 0;
        //relative path to a non-existing file, check the bundle paths.
        char *paths = celix_utils_strdup(bundlePath);
        const char *sep = ":";
        char *savePtr = NULL;
        if (paths == NULL) {
            goto error_out;
        }

        for (char *path = strtok_r(paths, sep, &savePtr); path != NULL; path = strtok_r(NULL, sep, &savePtr)){
            char* resolvedPath = celix_utils_writeOrCreateString(pathBuffer, pathBufferSize, "%s/%s", path, bundleLocation);
            if (celix_utils_fileExists(resolvedPath)) {
                result = resolvedPath;
                break;
            } else {
                celix_utils_freeStringIfNotEqual(pathBuffer, resolvedPath);
            }
        }
        free(paths);
    }
error_out:
    if (result == NULL && !silent) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Failed(%s) to resolve bundle location '%s', taking into account the cwd and Celix bundle path '%s'.",
               strerror(errno), bundleLocation, bundlePath);
    }
    return result;
}

static bool celix_framework_utils_isBundlePathNewerThan(celix_framework_t *fw, const char* bundlePath, const struct timespec* time) {
    struct timespec bundleModTime;
    char pathBuffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    char* resolvedPath = celix_framework_utils_resolveFileBundleUrl(pathBuffer, sizeof(pathBuffer), fw, bundlePath, true);
    celix_status_t status = celix_utils_getLastModified(resolvedPath, &bundleModTime);
    if (status != CELIX_SUCCESS) {
        fw_logCode(fw->logger, CELIX_LOG_LEVEL_ERROR, status, "Cannot get last modified time for bundle %s", resolvedPath);
        return false;
    }
    double diff = celix_difftime(&bundleModTime, time);
    celix_utils_freeStringIfNotEqual(pathBuffer, resolvedPath);
    return diff < 0.0;
}

bool celix_framework_utils_isBundleUrlNewerThan(celix_framework_t* fw, const char* bundleURL, const struct timespec* time) {
    if (time == NULL) {
        return true;
    }

    char* trimmedUrl = celix_utils_trim(bundleURL);
    bool newer;
    size_t fileSchemeLen = sizeof(FILE_URL_SCHEME)-1;
    if (strncasecmp(FILE_URL_SCHEME, trimmedUrl, fileSchemeLen) == 0) {
        newer = celix_framework_utils_isBundlePathNewerThan(fw, trimmedUrl + fileSchemeLen, time); //skip the file:// part
    } else {
        newer = celix_framework_utils_isBundlePathNewerThan(fw, trimmedUrl, time);
    }
    free(trimmedUrl);

    return newer;
}

static celix_status_t celix_framework_utils_extractBundlePath(celix_framework_t *fw, const char* bundlePath, const char* extractPath) {
    FW_LOG(CELIX_LOG_LEVEL_TRACE, "Extracting bundle url `%s` to dir `%s`", bundlePath, extractPath);
    const char* err = NULL;

    char buffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    char* resolvedPath = celix_framework_utils_resolveFileBundleUrl(buffer, sizeof(buffer), fw, bundlePath, false);
    if (resolvedPath == NULL) {
        //other errors should be caught by celix_framework_utils_isBundleUrlValid
        return CELIX_ENOMEM;
    }
    celix_status_t status = CELIX_SUCCESS;
    if (celix_utils_directoryExists(resolvedPath)) {
        char *abs = realpath(resolvedPath, NULL);
        if (abs == NULL) {
            status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
            err = "Could not get real path for bundle";
        }
        if (status == CELIX_SUCCESS) {
            if(symlink(abs, extractPath) == -1) {
                status = CELIX_ERROR_MAKE(CELIX_FACILITY_CERRNO,errno);
                err = "Could not add symbolic link";
            }
        }
        free(abs);
    } else {
        status = celix_utils_extractZipFile(resolvedPath, extractPath, &err);
    }
    framework_logIfError(fw->logger, status, err, "Could not extract bundle zip file `%s` to `%s`", resolvedPath, extractPath);
    celix_utils_freeStringIfNotEqual(buffer, resolvedPath);
    return status;
}

celix_status_t celix_framework_utils_extractBundle(celix_framework_t *fw, const char *bundleURL,  const char* extractPath) {
    if (!celix_framework_utils_isBundleUrlValid(fw, bundleURL, false)) {
        return CELIX_ILLEGAL_ARGUMENT;
    }
    char* trimmedUrl = celix_utils_trim(bundleURL);

    celix_status_t status;
    size_t fileSchemeLen = sizeof(FILE_URL_SCHEME)-1;
    if (strncasecmp(FILE_URL_SCHEME, trimmedUrl, fileSchemeLen) == 0) {
        status = celix_framework_utils_extractBundlePath(fw, trimmedUrl + fileSchemeLen, extractPath);
    } else {
        status = celix_framework_utils_extractBundlePath(fw, trimmedUrl, extractPath);
    }

    free(trimmedUrl);
    return status;
}

bool celix_framework_utils_isBundleUrlValid(celix_framework_t *fw, const char *bundleURL, bool silent) {
    char* trimmedUrl = celix_utils_trim(bundleURL);

    if (celix_utils_isStringNullOrEmpty(trimmedUrl)) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Invalid NULL or empty bundleURL argument. Provided argument is '%s'", bundleURL);
        free(trimmedUrl);
        return false;
    }

    bool valid;
    char buffer[CELIX_DEFAULT_STRING_CREATE_BUFFER_SIZE];
    size_t fileSchemeLen = sizeof(FILE_URL_SCHEME)-1;
    if (strncasecmp(FILE_URL_SCHEME, trimmedUrl, fileSchemeLen) == 0) {
        char* loc = celix_framework_utils_resolveFileBundleUrl(buffer, sizeof(buffer), fw, trimmedUrl + fileSchemeLen, silent);
        valid = loc != NULL;
        celix_utils_freeStringIfNotEqual(buffer, loc);
    } else if (strcasestr(trimmedUrl, "://")) {
        valid = false;
        if (!silent) {
            FW_LOG(CELIX_LOG_LEVEL_ERROR, "Bundle URL '%s' is not a valid url. Scheme is not supported.", bundleURL);
        }
    } else {
        char* loc = celix_framework_utils_resolveFileBundleUrl(buffer, sizeof(buffer), fw, trimmedUrl, silent);
        valid = loc != NULL;
        celix_utils_freeStringIfNotEqual(buffer, loc);
    }
    free(trimmedUrl);

    return valid;
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


celix_status_t celix_framework_utils_createBundleArchivesCache(celix_framework_t* fw) {
    bool useTmp = celix_framework_getConfigPropertyAsBool(fw,
                                                          CELIX_FRAMEWORK_CACHE_USE_TMP_DIR,
                                                          CELIX_FRAMEWORK_CACHE_USE_TMP_DIR_DEFAULT,
                                                          NULL);
    if (useTmp) {
        fw_log(fw->logger, CELIX_LOG_LEVEL_ERROR, "Creating a bundle cache in combination with using a tmp dir for bundle cache is not supported.");
        return CELIX_ILLEGAL_STATE;
    }
    return celix_bundleCache_createBundleArchivesCache(fw, true);
}