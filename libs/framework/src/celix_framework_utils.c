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
#include "celix_bundle_context.h"
#include "framework_private.h"

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

static bool extractBundlePath(celix_framework_t *fw, const char* bundlePath, const char* extractPath) {
    const char* err = NULL;
    FW_LOG(CELIX_LOG_LEVEL_DEBUG, "Extracting bundle url `%s` to dir `%s`", bundlePath, extractPath);
    celix_status_t status = celix_utils_extractZipFile(bundlePath, extractPath, &err);
    if (status == CELIX_SUCCESS) {
        FW_LOG(CELIX_LOG_LEVEL_TRACE, "Bundle zip `%s` extracted to `%s`", bundlePath, extractPath);
    } else {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Could not extract bundle zip `%s` to `%s`: %s", bundlePath, extractPath, err);
    }
    return status == CELIX_SUCCESS;
}

static bool extractBundleEmbedded(celix_framework_t *fw, const char* embeddedBundle, const char* extractPath) {
    FW_LOG(CELIX_LOG_LEVEL_DEBUG, "Extracting embedded bundle `%s` to dir `%s`", embeddedBundle, extractPath);

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
    celix_status_t status = celix_utils_extractZipData(start, end-start, extractPath, &err);
    if (status == CELIX_SUCCESS) {
        FW_LOG(CELIX_LOG_LEVEL_TRACE, "Embedded bundle zip `%s` extracted to `%s`", embeddedBundle, extractPath);
    } else {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Could not extract embedded bundle zip `%s` to `%s`: %s", embeddedBundle, extractPath, err);
    }
    return status == CELIX_SUCCESS;
}

celix_status_t celix_framework_utils_extractBundle(celix_framework_t *fw, const char *bundleURL,  const char* extractPath) {
    if (celix_utils_isStringNullOrEmpty(bundleURL)) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Invalid NULL or empty bundleURL argument");
        return CELIX_ILLEGAL_ARGUMENT;
    }

    char* trimmedUrl = celix_utils_trim(bundleURL);

    bool extracted;
    if (strncasecmp("file://", bundleURL, 7) == 0) {
        extracted = extractBundlePath(fw, trimmedUrl+7, extractPath); //note +7 to remove the file:// part.
    } else if (strncasecmp("embedded://", bundleURL, 11) == 0) {
        extracted = extractBundleEmbedded(fw, trimmedUrl+11, extractPath); //note +11 to remove the embedded:// part.
    } else if (strcasestr(bundleURL, "://")) {
        FW_LOG(CELIX_LOG_LEVEL_ERROR, "Cannot extract bundle url '%s', because url type is not supported. Supported types are file:// or embedded://.", bundleURL);
        extracted = false;
    } else {
        extracted = extractBundlePath(fw, trimmedUrl, extractPath);
    }

    free(trimmedUrl);
    return extracted ? CELIX_SUCCESS : CELIX_FILE_IO_EXCEPTION;
}