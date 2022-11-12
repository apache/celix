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

#ifndef CELIX_FILE_UTILS_H
#define CELIX_FILE_UTILS_H

#include <stdbool.h>
#include "celix_errno.h"

#ifdef __cplusplus
extern "C" {
#endif

/**
 * @brief Returns whether a file at path exists.
 */
bool celix_utils_fileExists(const char* path);

/**
 * @brief Returns whether a file at path exists and is a directory.
 */
bool celix_utils_directoryExists(const char* path);

/**
 * @brief Create dir and if needed subdirs.
 * @param path The directory to create.
 * @param failIfPresent Whether to fail if the directory already exists.
 * @param errorOut An optional error output argument. If an error occurs this will point to a (static) error message.
 * @return CELIX_SUCCESS if the directory was successfully created.
 */
celix_status_t celix_utils_createDirectory(const char* path, bool failIfPresent, const char** errorOut);

/**
 * @brief Recursively delete the directory at path.
 *
 * @param path A directory path. Directory should exist.
 * @param errorOut An optional error output argument. If an error occurs this will point to a (static) error message.
 * @return CELIX_SUCCESS if the directory was found and successfully deleted.
 */
celix_status_t celix_utils_deleteDirectory(const char* path, const char** errorOut);

/**
 * @brief Extract the zip file to the target dir.
 *
 * Will create the targetDir if it does not already exist.
 *
 * @param zipPath The path to the zip file.
 * @param extractToDir The path where the zip file will be extracted.
 * @param errorOut An optional error output argument. If an error occurs this will point to a (static) error message.
 * @return CELIX_SUCCESS if the zip file was extracted successfully.
 */
celix_status_t celix_utils_extractZipFile(const char* zipPath, const char* extractToDir, const char** errorOut);


/**
 * @brief Extract the zip data to the target dir.
 *
 * Will create the targetDir if it does not already exist.
 *
 * @param zipData pointer to the beginning of the zip data
 * @param zipDataLen the size which at least fits the zip date.
 * @param extractToDir The path where the zip data will be extracted.
 * @param errorOut An optional error output argument. If an error occurs this will point to a (static) error message.
 * @return CELIX_SUCCESS if the zip data was extracted successfully.
 */
celix_status_t celix_utils_extractZipData(const void *zipData, size_t zipDataSize, const char* extractToDir, const char** errorOut);

#ifdef __cplusplus
}
#endif

#endif //CELIX_FILE_UTILS_H
