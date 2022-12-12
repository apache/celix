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

#ifndef CELIX_FRAMEWORK_UTILS_PRIVATE_H_
#define CELIX_FRAMEWORK_UTILS_PRIVATE_H_


#include "celix_framework_utils.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief extracts a bundle for the given cache.
 * @param fw Optional Celix framework (used for logging).
 *           If NULL the result of celix_frameworkLogger_globalLogger() will be used for logging.
 * @param extractPath The path to extract the bundle to.
 * @param bundleURL The bundle url. Which must be the following:
 *  - prefixed with file:// -> url is a file path.
 *  - prefixed with embedded:// -> url is a symbol for the bundle embedded in the current executable.
 *  - *:// -> not supported
 *  - no :// -> assuming that the url is a file path (same as with a file:// prefix)
 * @return CELIX_SUCCESS is the bundle was correctly extracted.
 */
celix_status_t celix_framework_utils_extractBundle(celix_framework_t *fw, const char *bundleURL,  const char* extractPath);

/**
 * @brief Checks whether the provided bundle url is valid.
 *
 * An invalid bundle url is:
 *  - A bundle url with an invalid url scheme (only file:// or embedded:// is supported)
 *  - A file bundle url where the file does not exists.
 *  - A embedded bundle url where the required symbol cannot be found.
 *
 *  If a bundle url is invalid, this function will print - on error level - why the url is invalid.
 *
 * @param fw Optional Celix framework (used for logging).
 *           If NULL the result of celix_frameworkLogger_globalLogger() will be used for logging.
 * @param bundleURL A bundle url to check.
 * @param silent If true, this function will not write error logs when the bundle url is not valid.
 * @return Whether the bundle url is valid.
 */
bool celix_framework_utils_isBundleUrlValid(celix_framework_t *fw, const char *bundleURL, bool silent);

#ifdef __cplusplus
}
#endif

#endif /* CELIX_FRAMEWORK_UTILS_PRIVATE_H_ */
